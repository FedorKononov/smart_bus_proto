
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "portaudio.h"

#define SAMPLE_RATE  (16000)
#define FRAMES_PER_BUFFER (320)
#define NUM_CHANNELS    (1)
#define NUM_SECONDS     (2)

/* Select sample format. */

#define PA_SAMPLE_TYPE  paInt16
#define SAMPLE_SIZE (2)
#define SAMPLE_SILENCE  (0)
#define CLEAR(a) memset((a), 0,  FRAMES_PER_BUFFER * NUM_CHANNELS * SAMPLE_SIZE)
#define PRINTF_S_FORMAT "%d"


int main(void)
{
	PaStreamParameters inputParameters;
	PaStream *stream = NULL;
	PaError err;

	char *sampleBlock;
	int i;
	int numBytes;

	numBytes = FRAMES_PER_BUFFER * NUM_CHANNELS * SAMPLE_SIZE ;
	sampleBlock = (char *) malloc( numBytes );

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

		fwrite(sampleBlock, 2, numBytes/2, stdout);fflush(stdout);
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