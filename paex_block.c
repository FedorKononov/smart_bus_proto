
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "portaudio.h"

#define SAMPLE_RATE  (16000)
#define FRAMES_PER_BUFFER (320)
#define NUM_CHANNELS    (1)
#define NUM_SECONDS     (2)
#define NOISE_THRESHOLD (21000)
#define QUIT_HOLDING_TIME (85)

/* Select sample format. */

#define PA_SAMPLE_TYPE  paInt16
#define SAMPLE_SIZE (2)
#define SAMPLE_SILENCE  (0)
#define CLEAR(a) memset((a), 0,  FRAMES_PER_BUFFER * NUM_CHANNELS * SAMPLE_SIZE)

struct chunk {
  char *data;
  struct chunk *next;
};

int main(int argc, char **argv)
{
	PaStreamParameters inputParameters;
	PaStream *stream = NULL;
	PaError err;

	char *sampleBlock;
	int i, j;
	int numBytes;
	char val;
	double average;
	int silence_state = 0;
	struct chunk *buffer_head;
	struct chunk *buffer;
	struct chunk *tmp_buffer;

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
				//printf("record start\n");
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

			//printf("sample average = %lf\n", average);
		
			// кодируем на лету

			// пишем чанк в буфер
			buffer->data = (char *) malloc(numBytes);
			memcpy(buffer->data, sampleBlock, numBytes);
			buffer->next = 0;

			// тишину мы тоже пишим но увеличиваем счетчик
			if (average < NOISE_THRESHOLD)
				silence_state++;
		

			if (silence_state > QUIT_HOLDING_TIME)
			{
				//printf("record stop\n");

				// деактивируем счетчик тишины
				silence_state = 0;

				// отправляем

				// чистим буфер
				buffer = buffer_head;
				while (buffer != NULL)
				{
					fwrite(buffer->data, 2, numBytes/2, stdout);fflush(stdout);
					tmp_buffer = buffer;
					buffer = buffer->next;
					free(tmp_buffer);
				}
				buffer_head = NULL;
			}
		}
	}

	err = Pa_StopStream(stream);
	if(err != paNoError) goto error;

	Pa_CloseStream(stream);

	free(sampleBlock);

	Pa_Terminate();
	return 0;

	error:
		if (stream)
		{
			Pa_AbortStream(stream);
			Pa_CloseStream(stream);
		}

		free(sampleBlock);
		Pa_Terminate();
		fprintf(stderr, "An error occured while using the portaudio stream\n");
		fprintf(stderr, "Error number: %d\n", err );
		fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
		return -1;
}