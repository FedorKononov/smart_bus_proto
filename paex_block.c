#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <portaudio.h>
#include <speex/speex.h>
#include <fcntl.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SAMPLE_RATE  16000
#define FRAMES_PER_BUFFER 320
#define NUM_CHANNELS    1
#define NUM_SECONDS     2
#define NOISE_THRESHOLD 21000
#define QUIT_HOLDING_TIME 40
#define SPEEX_FRAME_LEN 110

/* Select sample format. */

#define PA_SAMPLE_TYPE  paInt16
#define SAMPLE_SIZE 2
#define SAMPLE_SILENCE  0
#define CLEAR(a) memset((a), 0,  FRAMES_PER_BUFFER * NUM_CHANNELS * SAMPLE_SIZE)

#define PORT "80" // the port client will be connecting to 

#define MAXDATASIZERESP 400 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

struct chunk {
	short *data;
	struct chunk *next;
};


int main(int argc, char **argv)
{
	FILE *rec_file;

	PaStreamParameters inputParameters;
	PaStream *stream = NULL;
	PaError err;

	char *sampleBlock, *sent_buffer;
	int i, j, sent_buffer_len;
	int numBytes;
	char val;
	double average;
	int silence_state = 0;
	struct chunk *buffer_head;
	struct chunk *buffer;
	struct chunk *tmp_buffer;

	int sockfd, sock_numbytes;  
    char resp_buf[MAXDATASIZERESP];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char *query;
    char *tpl = "POST /speech-api/v1/recognize?xjerr=1&pfilter=1&client=chromium&lang=ru-RU HTTP/1.1\r\nHost: www.google.com\r\nUser-Agent: fedor\r\nConnection: close\r\nContent-Type: audio/x-speex-with-header-byte; rate=16000\r\nContent-Length: %d\r\n\r\n";

	// speex
	char cbits[SPEEX_FRAME_LEN + 1];
	int nbBytes, qualty, vbr;
	/*Holds the state of the encoder*/
	void *spex_state;
	/*Holds bits so they can be read and written to by the Speex routines*/
	SpeexBits spex_bits;

	// Creating socket to google
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo("www.google.com", PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	freeaddrinfo(servinfo); // all done with this structure

	rec_file = fopen("record", "w");

	// speex_bits_init() does not initialize all of the |bits| struct.
	memset(&spex_bits, 0, sizeof(spex_bits));

	// Initialization of the structure that holds the bits
	speex_bits_init(&spex_bits);

	//Create a new encoder state in widewband mode
	spex_state = speex_encoder_init(&speex_wb_mode);

	//Set the quality to 8 (15 kbps)
	qualty = 8;
	speex_encoder_ctl(spex_state, SPEEX_SET_QUALITY, &qualty);

	vbr = 1;
	speex_encoder_ctl(spex_state, SPEEX_SET_VBR, &vbr);

	numBytes = FRAMES_PER_BUFFER * NUM_CHANNELS * SAMPLE_SIZE ;
	sampleBlock = (char *) malloc(numBytes);

	if (sampleBlock == NULL)
	{
		printf("Could not allocate record array.\n");
		exit(1);
	}

	CLEAR(sampleBlock);

	err = Pa_Initialize();
	if (err != paNoError) goto error;

	inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
	inputParameters.channelCount = NUM_CHANNELS;
	inputParameters.sampleFormat = PA_SAMPLE_TYPE;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency ;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	err = Pa_OpenStream(
		&stream,
		&inputParameters,
		NULL,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		paClipOff, /* we won't output out of range samples so don't bother clipping them */
		NULL, /* no callback, use blocking API */
		NULL /* no callback, so no callback userData */
	); 

	if (err != paNoError) goto error;

	err = Pa_StartStream(stream);
	if (err != paNoError) goto error;

	while (1)
	{
		Pa_ReadStream(stream, sampleBlock, FRAMES_PER_BUFFER);
		//fwrite(sampleBlock, 2, numBytes/2, stdout);fflush(stdout);

		// получаем среднее
		average = 0.0;
		for (j=0; j<FRAMES_PER_BUFFER*2; j++)
		{
			val = sampleBlock[j];
			if (val < 0) val = -val; // ABS
			average += val;
		}
		
		// если поймана речь начинам писать буфер и активируем счетчик тишины для прекрашения записи если в эфире будет тишина чере некоторое время
		if (average > NOISE_THRESHOLD)
		{
			if (silence_state == 0)
			{
				printf("record start\n");
				silence_state = 1;

				// инициализируем голову буфера
				buffer_head = malloc(sizeof(struct chunk));
				buffer = buffer_head;
			} else
				silence_state = 2;
			
		}

		if (silence_state > 0)
		{
			if (silence_state > 1)
			{
				// инициаллизируем следующий чанк
				buffer->next = malloc(sizeof(struct chunk));
				buffer = buffer->next;
			}

			printf("sample average = %lf\n", average);

			// пишем чанк в буфер
			buffer->data = (short *) malloc(numBytes);
			memcpy(buffer->data, sampleBlock, numBytes);
			buffer->next = 0;

			// тишину мы тоже пишим но увеличиваем счетчик
			if (average < NOISE_THRESHOLD)
				silence_state++;
		

			if (silence_state > QUIT_HOLDING_TIME)
			{
				printf("record stop\n");

				// подготавливаем буфер для суммы всех фреймов
				sent_buffer = (char *) malloc(numBytes * silence_state);
				sent_buffer_len = 0;

				// деактивируем счетчик тишины
				silence_state = 0;

				// чистим буфер
				buffer = buffer_head;
				while (buffer != NULL)
				{
					fwrite(buffer->data, 2, numBytes/2, rec_file);

					/*Flush all the bits in the struct so we can encode a new frame*/
					speex_bits_reset(&spex_bits);

					/*Encode the frame*/
					speex_encode_int(spex_state, buffer->data, &spex_bits);

					/*Copy the bits to an array of char that can be written*/
					nbBytes = speex_bits_write(&spex_bits, cbits, SPEEX_FRAME_LEN);

					// write header
					//fwrite(&nbBytes, 1, 1, rec_file);
					memcpy(sent_buffer + sent_buffer_len, &nbBytes, 1);

					// write the compressed data
					//fwrite(cbits, 1, nbBytes, rec_file);
					memcpy(sent_buffer + sent_buffer_len + 1, cbits, nbBytes);

					sent_buffer_len = sent_buffer_len + nbBytes + 1;

					tmp_buffer = buffer;
					buffer = buffer->next;
					free(tmp_buffer);
				}
				buffer_head = NULL;

				// отправляем
				query = (char *)malloc(strlen(tpl));

				sprintf(query, tpl, sent_buffer_len);

				// send http query
				if ((sock_numbytes=send(sockfd, query, strlen(query), 0)) == -1) {
					perror("sendto");
					exit(1);
				}
				// send buffer
				if ((sock_numbytes=send(sockfd, sent_buffer, sent_buffer_len, 0)) == -1) {
					perror("sendto");
					exit(1);
				}

				while ((sock_numbytes = recv(sockfd, resp_buf, MAXDATASIZERESP-1, 0)) > 0) {
					resp_buf[sock_numbytes] = '\0';

					printf("%s\n", resp_buf);
				}
				fclose(rec_file);
				//fwrite(sent_buffer, 1, sent_buffer_len, stdout);
				return 1;
			}
		}
	}

	err = Pa_StopStream(stream);
	if(err != paNoError) goto error;

	Pa_CloseStream(stream);

	free(sampleBlock);

	Pa_Terminate();

	close(sockfd);

	/*Destroy the encoder state*/
	speex_encoder_destroy(spex_state);
	/*Destroy the bit-packing struct*/
	speex_bits_destroy(&spex_bits);
	return 0;

	error:
		if (stream)
		{
			Pa_AbortStream(stream);
			Pa_CloseStream(stream);
		}

		free(sampleBlock);
		Pa_Terminate();

		close(sockfd);

		/*Destroy the encoder state*/
		speex_encoder_destroy(spex_state);
		/*Destroy the bit-packing struct*/
		speex_bits_destroy(&spex_bits);

		fprintf(stderr, "An error occured while using the portaudio stream\n");
		fprintf(stderr, "Error number: %d\n", err );
		fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
		return -1;
}