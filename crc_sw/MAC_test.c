/**********************************************************/
/*	ECE695R, FALL 2016, HOMEWORK #4			  */
/*	File: MAC_test.c				  */
/**********************************************************/

/* This is the reference program for performing
 * the compute-intensive steps of the IEEE 802.11
 * MAC protocol: WEP encryption and CRC computation.
 *
 *  The steps are:
 *  1. The ICV (CRC of the payload) is calculated and appended
 *     to the frame.
 *  2. The payload and ICV are encrypted using WEP.
 *  3. The FCS (CRC of the entire frame) is calculated
 *     and appended to the frame.
 */

//Header files
#include <stdio.h>
#include "system.h"
#include "crc_table.h"
#include "test_frames.h"
#include "misc.h"


// Function declarations
void sbox_setup ( unsigned char seed[] );
unsigned char get_wep_byte ( void );
void wep_encrypt ( int plaintext_len, unsigned char plaintext[], unsigned char ciphertext[], unsigned char seed[] );
unsigned long get_crc( int no_of_bytes, unsigned char data[] );


// Global Variable declarations
static unsigned char sbox[256];
static unsigned char wep_index1;
static unsigned char wep_index2;

const char* framesize[4] = {"16B", "64B", "256B", "1024B"};

/***************************************************/
/* Initialize the S-Box used in WEP encryption     */
/***************************************************/
void sbox_setup(unsigned char seed[])
{
  unsigned char index, temp;
  short counter;

  // Initialize index variables
  index = 0;
  wep_index1 = 0;
  wep_index2 = 0;

  for (counter = 0; counter < 256; counter++)
    sbox[counter] = (unsigned char)counter;

  for (counter = 0; counter < 256; counter++)
    {
      // Calculate index
      index = (index + sbox[counter] + seed[counter % 8]) % 256;

      // Swap bytes
      temp = sbox[counter];
      sbox[counter] = sbox[index];
      sbox[index] = temp;
    }
}

/***************************************************/
/* Get a Byte of the key stream - this is just     */
/* XOR-ed with the plaintext                       */
/***************************************************/
unsigned char get_wep_byte(void)
{
  unsigned char index, temp;

  wep_index1 = (wep_index1 + 1) % 256;
  wep_index2 = (wep_index2 + sbox[wep_index1]) % 256;
  temp = sbox[wep_index1];
  sbox[wep_index1] = sbox[wep_index2];
  sbox[wep_index2] = temp;
  index = (sbox[wep_index1] + sbox[wep_index2]) % 256;

  return sbox[index];
}

/***************************************************/
/* Perform WEP encryption of the given plaintext   */
/* using the given seed, using the RC4 algorithm   */
/***************************************************/
void wep_encrypt(int plaintext_len, unsigned char plaintext[], unsigned char ciphertext[], unsigned char seed[])
{
  int i;

  sbox_setup(seed);

  for (i = 0; i < plaintext_len; i++)
    {
      ciphertext[i] = plaintext[i] ^ get_wep_byte();
    }
}

/***************************************************/
/* Compute the CRC of an array of Bytes            */
/***************************************************/
//SW Basic Design
// This is the reference Software implementation of CRC computation, invoked by setting mode variable to 1.
unsigned long get_crc(int no_of_bytes, unsigned char data[])
{
  unsigned long crc_temp = 0xFFFFFFFF;
  int i, index;

	//Performance Counter
	PERF_BEGIN(PERF_CNTR_BASE, 1);

  index = ((int)(crc_temp >> 24)) & 0xFF;
  crc_temp = (crc_temp << 8) ^ crc_table[index];
#ifdef DebugPrint
  printf ("CRC 0x%08x \n", crc_temp);
#endif

  for (i = 0; i < no_of_bytes; i++)
    {
      index = ((int)(crc_temp >> 24) ^ data[i]) & 0xFF;
      crc_temp = (crc_temp << 8) ^ crc_table[index];
#ifdef DebugPrint
      printf ("CRC 0x%08x \n", crc_temp);
#endif
    }
  crc_temp = crc_temp ^ 0xFFFFFFFF;
#ifdef DebugPrint
  printf ("CRC 0x%08x \n", crc_temp);
#endif

	//Performance Counter
	PERF_END(PERF_CNTR_BASE, 1);

  return crc_temp;
}



/***************************************************/
/*  Main function - runs through the test frames,  */
/*       performs the required processing one each */
/*       one, and prints out the final frame       */
/***************************************************/
int main()
{

  unsigned char frame[MAX_FRAME_SIZE];
  unsigned char *payload;
  unsigned int payload_len, frame_len;
  unsigned long crc;
  int i,j;
  unsigned char seed[8];

  // The 5 MSBs of the seed represent the key,
  // which do not change with the frame
  // The 3 LSBs represent the IV, which change
  // with each new frame (part of the 802.11 specification)
  for(j = 0; j < 5; j++) {
    seed[3+j] = test_key[j];
  }

  	//for (i = 0; i< 1; i++)
    for (i = 0; i< NUM_TEST_FRAMES; i++)
    {
	  //Performance Counter
	  PERF_RESET(PERF_CNTR_BASE);
	  PERF_START_MEASURING(PERF_CNTR_BASE);

      //Copy in data from the i-th test frame
      payload_len = test_frames[i].payload_length;

      frame_len = 0;
      for(j = 0; j < HEADER_SIZE; j++)
	frame[frame_len++] = test_frames[i].header[j];
      for(j = 0; j < IV_SIZE; j++)
	frame[frame_len++] = test_frames[i].iv[j];
      for(j = 0; j < payload_len; j++)
	frame[frame_len++] = test_frames[i].payload[j];

      seed[0] = test_frames[i].iv[0];
      seed[1] = test_frames[i].iv[1];
      seed[2] = test_frames[i].iv[2];

      // payload is just a pointer into the frame array
      // It comes after the header and IV
      payload = &(frame[HEADER_SIZE+IV_SIZE]);

      // Compute the ICV (CRC of the payload)
      //printf("\n\nCRC1\n");
      if(mode == 1) crc = get_crc(payload_len, payload);
      else if(mode == 2) crc = get_crchw(payload_len, payload);
      else if(mode == 3) crc = get_crchw2(payload_len, payload);
      else if(mode == 4) crc = get_crchw3(payload_len, payload);
      else if(mode == 5) crc = get_crc_comboCI(payload_len, payload);
      else if(mode == 6) crc = get_crc_multiCI(payload_len, payload);
      else if(mode == 7) crc = get_crc_xCreditCI(payload_len, payload);

		// Append the ICV to the frame
      frame[frame_len++] = (unsigned char)(crc >> 24);
      frame[frame_len++] = (unsigned char)(crc >> 16);
      frame[frame_len++] = (unsigned char)(crc >> 8);
      frame[frame_len++] = (unsigned char)(crc);

      // Encrypt the payload and ICV. Plaintext and Ciphertext
      // pointers are the same, which means that
      // Ciphertext overwrites the Plaintext
      wep_encrypt(payload_len+ICV_SIZE, payload, payload, seed);

      // Compute the FCS (CRC of the whole frame)
      //printf("\n\nCRC2\n");
      if(mode == 1) crc = get_crc(frame_len, frame);
      else if(mode == 2) crc = get_crchw(frame_len, frame);
      else if(mode == 3) crc = get_crchw2(frame_len, frame);
      else if(mode == 4) crc = get_crchw3(frame_len, frame);
      else if(mode == 5) crc = get_crc_comboCI(frame_len, frame);
      else if(mode == 6) crc = get_crc_multiCI(frame_len, frame);
      else if(mode == 7) crc = get_crc_xCreditCI(frame_len, frame);

      // Append the FCS to the frame
      frame[frame_len++]   = (unsigned char)(crc >> 24);
      frame[frame_len++] = (unsigned char)(crc >> 16);
      frame[frame_len++] = (unsigned char)(crc >> 8);
      frame[frame_len++] = (unsigned char)(crc);

      //Performance Counter
  	  PERF_STOP_MEASURING(PERF_CNTR_BASE);
  	  perf_print_formatted_report(PERF_CNTR_BASE, 50000000, 1, framesize[i]);


      // Display the final frame that would be transmitted
      printf("Output frame %d (%d Bytes):\n", i, frame_len);
      for(j = 0; j < frame_len; j++) {
	     printf("%2x ", frame[j]);
	     if(j > 2 && (j+1)%8 == 0) printf("\n");
      }

      printf("\n\n\n");

    }
  return(0);
}
