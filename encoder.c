#include <speex/speex.h>
#include <string.h>
#include <stdio.h>

/*The frame size in hardcoded for this sample code but it doesn't have to be*/
#define FRAME_SIZE 320
#define SPEEX_FRAME_LEN 110
int main(int argc, char **argv)
{
   char *inFile;
   FILE *fin;
   short in[FRAME_SIZE];

   char cbits[SPEEX_FRAME_LEN + 1];
   int nbBytes;
   /*Holds the state of the encoder*/
   void *state;
   /*Holds bits so they can be read and written to by the Speex routines*/
   SpeexBits bits;
   int i, tmp, vbr, samples_per_frame;
   
   inFile = argv[1];
   fin = fopen(inFile, "r");

   // speex_bits_init() does not initialize all of the |bits| struct.
   memset(&bits, 0, sizeof(bits));

   // Initialization of the structure that holds the bits
   speex_bits_init(&bits);

   /*Create a new encoder state in widewband mode*/
   state = speex_encoder_init(&speex_wb_mode);

   /*Set the quality to 8 (15 kbps)*/
   tmp = 8;
   speex_encoder_ctl(state, SPEEX_SET_QUALITY, &tmp);

   speex_encoder_ctl(state, SPEEX_GET_FRAME_SIZE, &samples_per_frame);
   //printf("max frame size: %d\n", samples_per_frame);

   vbr = 1;
   speex_encoder_ctl(state, SPEEX_SET_VBR, &vbr);

   while (1)
   {
      /*Read a 16 bits/sample audio frame*/
      fread(in, 2, samples_per_frame, fin);

      if (feof(fin))
         break;
      
      /*Flush all the bits in the struct so we can encode a new frame*/
      speex_bits_reset(&bits);

      /*Encode the frame*/
      speex_encode_int(state, in, &bits);

      /*Copy the bits to an array of char that can be written*/
      nbBytes = speex_bits_write(&bits, cbits, SPEEX_FRAME_LEN);

      /*Write the size of the frame first. This is what sampledec expects but
       it's likely to be different in your own application*/
      //fwrite(&nbBytes, sizeof(int), 1, stdout);

      fwrite(&nbBytes, 1, 1, stdout);

      /*Write the compressed data*/
      fwrite(cbits, 1, nbBytes, stdout);
   }
   
   /*Destroy the encoder state*/
   speex_encoder_destroy(state);
   /*Destroy the bit-packing struct*/
   speex_bits_destroy(&bits);
   fclose(fin);
   return 0;
}