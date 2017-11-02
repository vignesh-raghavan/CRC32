/*
 * misc.h
 *
 *  Created on: Oct 7, 2017
 *      Author: 695r24
 */
#include <altera_avalon_performance_counter.h>
#include "io.h"

#ifndef MISC_H_
#define MISC_H_


#define PERF_CNTR_BASE alt_get_performance_counter_base()
#define DMA_BASE DMA_0_BASE

// 1:SW Basic
// 2:HW Basic
// 3:HW Intermediate
// 4:HW Advanced
// 5:CI Combinational
// 6:CI Multicycle
// 7:CI Advanced
static unsigned int mode = 0x7;
//#define DebugPrint //Disable Debug Prints by commenting this define.


// Hardware Accelerator Registers in C

static unsigned int* crc_value = CRC_AVALONSS_INTERFACE_0_BASE;
static unsigned int* crc_status = CRC_AVALONSS_INTERFACE_0_BASE + 4;
static unsigned int* crc_control = CRC_AVALONSS_INTERFACE_0_BASE + 8;

//Custom Instruction Templates in C

// Combinational Custom Instruction Template
// Inputs : 32-bit CRC and 8-bit Data
// Output : 32-bit CRC (Updated value)
#define CRC_comboCI(crc, data) ALT_CI_CRC_CUSTOM_COMBINATIONAL_0(crc, (data & 0xFF))

// Multicycle Custom Instruction Template
// Inputs : 32-bit Data/CRC, 32-bit Dummy Data
// Output : 32-bit CRC value (updated value)
#define CRC_multiCI_LOAD(data1)  		ALT_CI_CRC_CUSTOM_MULTICYCLE_0(0x0, data1, 0x0)
#define CRC_multiCI_08BIT(data1) 		ALT_CI_CRC_CUSTOM_MULTICYCLE_0(0x1, data1, 0x0)
#define CRC_multiCI_16BIT(data1) 		ALT_CI_CRC_CUSTOM_MULTICYCLE_0(0x2, data1, 0x0)
#define CRC_multiCI_24BIT(data1) 		ALT_CI_CRC_CUSTOM_MULTICYCLE_0(0x3, data1, 0x0)
#define CRC_multiCI_32BIT(data1) 		ALT_CI_CRC_CUSTOM_MULTICYCLE_0(0x4, data1, 0x0)
#define CRC_multiCI_64BIT(data1, data2) ALT_CI_CRC_CUSTOM_MULTICYCLE_0(0x5, data1, data2)
#define CRC_multiCI_READ()       	    ALT_CI_CRC_CUSTOM_MULTICYCLE_0(0x6, 0x0,   0x0)

//HW Basic - Software Design
// This function is invoked for the CRC computation by setting the mode variable to 2 in the software.
// The CRC computation is performed in a dedicated CRC hardware one byte at a time.
// The processor reads from memory one byte of payload/frame data at a time.
// It then writes back one byte to the CRC slave for the CRC computation.
// This implementation does not use the synchronous FIFO at the CRC slave interface.
unsigned long get_crchw(int no_of_bytes, unsigned char data[])
{
  unsigned long crc_temp = 0xFFFFFFFF;
  int i;

     //Performance Counter
	PERF_BEGIN(PERF_CNTR_BASE, 1);

  *(crc_status) = 0x1;

#ifdef DebugPrint
	  crc_temp = *(crc_value);
	  printf ("CRC 0x%08x \n", crc_temp);
#endif

  for (i = 0; i < no_of_bytes; i++)
  {
	  *(crc_control) = 0x80000000 + (unsigned int)data[i];
#ifdef DebugPrint
		  crc_temp = *(crc_value);
		  printf ("CRC 0x%08x \n", crc_temp);
#endif
  }

  crc_temp = *(crc_status);
#ifdef DebugPrint
  printf ("CRC 0x%08x \n", crc_temp);
#endif

	//Performance Counter
	PERF_END(PERF_CNTR_BASE, 1);

  return crc_temp;
}


//HW Intermediate - Software Design
// NOT IN USE : The implementation is similar to get_crchw2 function, except the processor reads one byte at a time from
// memory and writes back one word at a time to the CRC slave.
unsigned long get_crchw1(int no_of_bytes, unsigned char data[])
{
  unsigned long crc_temp = 0xFFFFFFFF;
  int i;
  unsigned int data32;

     //Performance Counter
	PERF_BEGIN(PERF_CNTR_BASE, 1);

  *(crc_status) = 0x1;

#ifdef DebugPrint
	  crc_temp = *(crc_value);
	  printf ("CRC 0x%08x \n", crc_temp);
#endif

  for (i = 0; i < no_of_bytes; i=i+4)
  {
	  if(no_of_bytes-i >= 4)
	  {
		  data32 = (((unsigned int)data[i+3])<<24) + (((unsigned int)data[i+2])<<16) + (((unsigned int)data[i+1])<<8) + (unsigned int)data[i];
		  *(crc_value) = data32;
	  }
	  else
	  {
		  *(crc_control) = 0x80000000 + (unsigned int)data[i];
		  i = i-3;
	  }

#ifdef DebugPrint
		  crc_temp = *(crc_value);
		  printf ("CRC 0x%08x \n", crc_temp);
#endif
  }

  crc_temp = *(crc_status);
#ifdef DebugPrint
  printf ("CRC 0x%08x \n", crc_temp);
#endif

	//Performance Counter
	PERF_END(PERF_CNTR_BASE, 1);

  return crc_temp;
}


// HW Intermediate - Software Design
// This function is invoked for the CRC computation when the mode variable is set to 3 in software.
// The processor performs reads from memory and writes to CRC slave (in words).
// Care is taken for the unaligned bytes of data at the start and end of a payload/frame.
// Also, the CRC computation is done on a word (in 4 cycles) instead of single byte for each processor write.
// It also employs synchronous FIFO at the CRC slave interface to capture the incoming data from the processor,
// one word at a time.

unsigned long get_crchw2(int no_of_bytes, unsigned char data[])
{
  unsigned long crc_temp;
  int i;
  unsigned int data32;
  unsigned int* word;
  unsigned int** addr;
  unsigned int soffset;
  int j;

      //Performance Counter
	  PERF_BEGIN(PERF_CNTR_BASE, 1);

	  word = (unsigned int*) data;
	  addr = &word;
	  //printf ("MEM 0x%08x 0x%08x\n", *word, *addr);
	  soffset = *addr;
	  //printf ("MEM %d\n", soffset);
	  soffset = soffset%4;
	  data32 = ((*word) >> 8*soffset);

	  //printf ("MEM %d 0x%08x\n", soffset, data32);

	  *(crc_status) = 0x1;

#ifdef DebugPrint
	  crc_temp = *(crc_value);
	  printf ("CRC 0x%08x \n", crc_temp);
#endif

	  if(soffset)
	  {
		  for(i = soffset; i < 4; i++)
		  {
			  *(crc_control) = 0x80000000 + data32;
			  //printf ("MEM0 0x%08x\n", data32);
			  data32 = data32 >> 8;
		  }
	  }
	  else
	  {
		  *(crc_value) = data32;
		  //printf ("MEM0 0x%08x\n", data32);
	  }


	  for (i = 4-soffset; i <= no_of_bytes-4; i=i+4)
	  {
		  word++;
		  *(crc_value) = *word;
		  //printf ("MEM1 0x%08x\n", *word);
	  }

	  if(i < no_of_bytes)
	  {
		  for(j = i; j < no_of_bytes; j++)
		  {
			  *(crc_control) = 0x80000000 + (unsigned int)(data[j]);
			  //printf ("MEM2 0x%08x\n", (unsigned int)(data[j]));
		  }
	  }

#ifdef DebugPrint
		  crc_temp = *(crc_value);
		  printf ("CRC 0x%08x \n", crc_temp);
#endif

  crc_temp = *(crc_status);

#ifdef DebugPrint
  printf ("CRC 0x%08x \n", crc_temp);
#endif

	//Performance Counter
	PERF_END(PERF_CNTR_BASE, 1);

  return crc_temp;
}


// HW Advanced - Software Design
// This function is invoked for CRC computation when mode variable is set to 4 in software.
// It uses DMA controller to do memory to CRC slave data transfer (in words) for most portion of payload/frame.
// Care is taken for the unaligned bytes at the start and end of a payload/frame.
// It also stresses the 32 deep synchronous FIFO inside the CRC slave interface in Hardware.
unsigned long get_crchw3(int no_of_bytes, unsigned char data[])
{
  unsigned long crc_temp;
  int i;
  unsigned int data32;
  unsigned int* word;
  unsigned int** addr;
  unsigned int soffset;
  int j;
  int length;

      //Performance Counter
	  PERF_BEGIN(PERF_CNTR_BASE, 1);

	  word = (unsigned int*) data;
	  addr = &word;
	  //printf ("MEM 0x%08x 0x%08x\n", *word, *addr);
	  soffset = *addr;
	  //printf ("MEM %d\n", soffset);
	  soffset = soffset%4;
	  data32 = ((*word) >> 8*soffset);

	  //printf ("MEM %d 0x%08x\n", soffset, data32);

	  *(crc_status) = 0x1;

#ifdef DebugPrint
	  crc_temp = *(crc_value);
	  printf ("CRC 0x%08x \n", crc_temp);
#endif

	  if(soffset)
	  {
		  for(i = soffset; i < 4; i++)
		  {
			  *(crc_control) = 0x80000000 + data32;
			  //printf ("MEM0 0x%08x\n", data32);
			  data32 = data32 >> 8;
		  }
	  }
	  else
	  {
		  *(crc_value) = data32;
		  //printf ("MEM0 0x%08x\n", data32);
	  }

	  length = (no_of_bytes + soffset - 4) - (no_of_bytes + soffset - 4)%4;
	  //printf ("DMA %d\n", length);
	  word++;

	  IOWR(DMA_BASE, 6, 1<<12);
	  IOWR(DMA_BASE, 6, 1<<12);

	  IOWR(DMA_BASE, 1, word);
	  IOWR(DMA_BASE, 2, crc_value);
	  IOWR(DMA_BASE, 3, length);

	  IOWR(DMA_BASE, 6, 1<<9 | 1<< 7 | 1<<3 | 1<<2);
	  while((IORD(DMA_BASE, 0) & 0x1) == 0);
	  IOWR(DMA_BASE, 0, 0);

	  for(j = length + 4-soffset ; j < no_of_bytes; j++)
	  {
		  *(crc_control) = 0x80000000 + (unsigned int)(data[j]);
		  //printf ("MEM2 0x%08x\n", (unsigned int)(data[j]));
	  }


#ifdef DebugPrint
		  crc_temp = *(crc_value);
		  printf ("CRC 0x%08x \n", crc_temp);
#endif

  crc_temp = *(crc_status);

#ifdef DebugPrint
  printf ("CRC 0x%08x \n", crc_temp);
#endif

	//Performance Counter
	PERF_END(PERF_CNTR_BASE, 1);

  return crc_temp;
}



//Combinational custom Instruction Design
// This is the Combinational custom Instruction Design for CRC computation, invoked by setting mode variable to 5.
unsigned long get_crc_comboCI(int no_of_bytes, unsigned char data[])
{
  unsigned int crc_temp;
  int i;

	//Performance Counter
	PERF_BEGIN(PERF_CNTR_BASE, 1);

   crc_temp = 0x4E08BFB4;
#ifdef DebugPrint
   printf ("CRC 0x%08x \n", crc_temp);
#endif

	for(i = 0; i < no_of_bytes; i++)
	{
		crc_temp = CRC_comboCI(crc_temp, data[i]);
#ifdef DebugPrint
		printf ("CRC 0x%08x \n", crc_temp);
#endif
	}

	crc_temp = ~crc_temp; // Same as crc_temp = crc_temp ^ 0xFFFFFFFF;
#ifdef DebugPrint
   printf ("CRC 0x%08x \n", crc_temp);
#endif

	//Performance Counter
	PERF_END(PERF_CNTR_BASE, 1);

  return crc_temp;
}



//Multicycle custom Instruction Design
// This is the Multicycle custom Instruction Design for CRC computation, invoked by setting mode variable to 6.
unsigned long get_crc_multiCI(int no_of_bytes, unsigned char data[])
{
  unsigned int crc_temp;
  int i;
  unsigned int data32;
  unsigned int readdata;
  unsigned int* word;
  unsigned int** addr;
  unsigned int soffset;
  unsigned int eoffset;
  int j;
  int length;


	//Performance Counter
	PERF_BEGIN(PERF_CNTR_BASE, 1);

	word = (unsigned int*) data;
	addr = &word;
	soffset = *addr;
	soffset = soffset%4;
#ifdef DebugPrint
	printf ("MEM 0x%08x 0x%08x\n", *word, *addr);
	printf ("SOF %d\n", soffset);
	length = (no_of_bytes + soffset - 4) - (no_of_bytes + soffset - 4)%4;
	printf ("LEN %d\n", length);
	eoffset = no_of_bytes - (length + 4-soffset);
	printf ("EOF %d\n", eoffset);
#endif
	data32 = ((*word) >> 8*soffset);

	crc_temp = 0x4E08BFB4;
	CRC_multiCI_LOAD(crc_temp);
#ifdef DebugPrint
	readdata = CRC_multiCI_READ();
	printf ("CRC 0x%08x \n", readdata);
#endif

	if(soffset == 1)
	{
		CRC_multiCI_24BIT(data32);
	}
	else if(soffset == 2)
	{
		CRC_multiCI_16BIT(data32);
	}
	else if(soffset == 3)
	{
		CRC_multiCI_08BIT(data32);
	}
	else
	{
		CRC_multiCI_32BIT(data32);
	}
	word++;

	#ifdef DebugPrint
		readdata = CRC_multiCI_READ();
		printf ("CRC 0x%08x 0x%08x \n", data32, readdata);
	#endif


	i = 4-soffset;
	while((no_of_bytes - i)/4)
	{
		CRC_multiCI_32BIT(*word);
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC4 0x%08x 0x%08x \n", *word, readdata);
		#endif
		word++;
		i = i+4;
	}

	if(no_of_bytes - i == 1)
	{
		CRC_multiCI_08BIT(data[i]);
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC 0x%08x 0x%08x \n", data32, readdata);
		#endif
	}
	else if(no_of_bytes - i == 2)
	{
		data32 = (data[i+1] << 8) | data[i];
		CRC_multiCI_16BIT(data32);
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC 0x%08x 0x%08x \n", data32, readdata);
		#endif
	}
	else if(no_of_bytes - i == 3)
	{
		data32 = (data[i+2] << 16) | (data[i+1] << 8) | data[i];
		CRC_multiCI_24BIT(data32);
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC 0x%08x 0x%08x \n", data32, readdata);
		#endif
	}

	crc_temp = CRC_multiCI_READ();
	crc_temp = ~crc_temp; // Same as crc_temp = crc_temp ^ 0xFFFFFFFF;
#ifdef DebugPrint
	printf ("CRC 0x%08x \n", crc_temp);
#endif

	//Performance Counter
	PERF_END(PERF_CNTR_BASE, 1);

  return crc_temp;
}


//Multicycle custom Instruction ADVANCED Design
// This is the Multicycle custom Instruction ADVANCED Design for CRC computation, invoked by setting mode variable to 7.
unsigned long get_crc_xCreditCI(int no_of_bytes, unsigned char data[])
{
  unsigned int crc_temp;
  int i;
  unsigned int data32;
  unsigned int readdata;
  unsigned int* word;
  unsigned int** addr;
  unsigned int soffset;
  unsigned int eoffset;
  int j;
  int length;


	//Performance Counter
	PERF_BEGIN(PERF_CNTR_BASE, 1);

	word = (unsigned int*) data;
	addr = &word;
	soffset = *addr;
	soffset = soffset%4;
#ifdef DebugPrint
	printf ("MEM 0x%08x 0x%08x\n", *word, *addr);
	printf ("SOF %d\n", soffset);
	length = (no_of_bytes + soffset - 4) - (no_of_bytes + soffset - 4)%4;
	printf ("LEN %d\n", length);
	eoffset = no_of_bytes - (length + 4-soffset);
	printf ("EOF %d\n", eoffset);
#endif
	data32 = ((*word) >> 8*soffset);

	crc_temp = 0x4E08BFB4;
	CRC_multiCI_LOAD(crc_temp);
#ifdef DebugPrint
	readdata = CRC_multiCI_READ();
	printf ("CRC 0x%08x \n", readdata);
#endif

	if(soffset == 1)
	{
		CRC_multiCI_24BIT(data32);
	}
	else if(soffset == 2)
	{
		CRC_multiCI_16BIT(data32);
	}
	else if(soffset == 3)
	{
		CRC_multiCI_08BIT(data32);
	}
	else
	{
		CRC_multiCI_32BIT(data32);
	}
	word++;

	#ifdef DebugPrint
		readdata = CRC_multiCI_READ();
		printf ("CRC 0x%08x 0x%08x \n", data32, readdata);
	#endif


	i = 4-soffset;
	while((no_of_bytes - i)/128)
	{
		CRC_multiCI_64BIT(*word, *(word+1));
		CRC_multiCI_64BIT(*(word+2), *(word+3));
		CRC_multiCI_64BIT(*(word+4), *(word+5));
		CRC_multiCI_64BIT(*(word+6), *(word+7));
		CRC_multiCI_64BIT(*(word+8), *(word+9));
		CRC_multiCI_64BIT(*(word+10), *(word+11));
		CRC_multiCI_64BIT(*(word+12), *(word+13));
		CRC_multiCI_64BIT(*(word+14), *(word+15));
		CRC_multiCI_64BIT(*(word+16), *(word+17));
		CRC_multiCI_64BIT(*(word+18), *(word+19));
		CRC_multiCI_64BIT(*(word+20), *(word+21));
		CRC_multiCI_64BIT(*(word+22), *(word+23));
		CRC_multiCI_64BIT(*(word+24), *(word+25));
		CRC_multiCI_64BIT(*(word+26), *(word+27));
		CRC_multiCI_64BIT(*(word+28), *(word+29));
		CRC_multiCI_64BIT(*(word+30), *(word+31));
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC128 0x%08x 0x%08x \n", *word, readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+1), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+2), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+3), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+4), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+5), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+6), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+7), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+8), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+9), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+10), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+11), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+12), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+13), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+14), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+15), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+16), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+17), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+18), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+19), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+20), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+21), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+22), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+23), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+24), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+25), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+26), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+27), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+28), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+29), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+30), readdata);
			printf ("CRC128 0x%08x 0x%08x \n", *(word+31), readdata);
		#endif
		word = word+32;
		i = i+128;
	}

	while((no_of_bytes - i)/64)
	{
		CRC_multiCI_64BIT(*word, *(word+1));
		CRC_multiCI_64BIT(*(word+2), *(word+3));
		CRC_multiCI_64BIT(*(word+4), *(word+5));
		CRC_multiCI_64BIT(*(word+6), *(word+7));
		CRC_multiCI_64BIT(*(word+8), *(word+9));
		CRC_multiCI_64BIT(*(word+10), *(word+11));
		CRC_multiCI_64BIT(*(word+12), *(word+13));
		CRC_multiCI_64BIT(*(word+14), *(word+15));
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC64 0x%08x 0x%08x \n", *word, readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+1), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+2), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+3), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+4), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+5), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+6), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+7), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+8), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+9), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+10), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+11), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+12), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+13), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+14), readdata);
			printf ("CRC64 0x%08x 0x%08x \n", *(word+15), readdata);
		#endif
		word = word+16;
		i = i+64;
	}

	while((no_of_bytes - i)/32)
	{
		CRC_multiCI_64BIT(*word, *(word+1));
		CRC_multiCI_64BIT(*(word+2), *(word+3));
		CRC_multiCI_64BIT(*(word+4), *(word+5));
		CRC_multiCI_64BIT(*(word+6), *(word+7));
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC32 0x%08x 0x%08x \n", *word, readdata);
			printf ("CRC32 0x%08x 0x%08x \n", *(word+1), readdata);
			printf ("CRC32 0x%08x 0x%08x \n", *(word+2), readdata);
			printf ("CRC32 0x%08x 0x%08x \n", *(word+3), readdata);
			printf ("CRC32 0x%08x 0x%08x \n", *(word+4), readdata);
			printf ("CRC32 0x%08x 0x%08x \n", *(word+5), readdata);
			printf ("CRC32 0x%08x 0x%08x \n", *(word+6), readdata);
			printf ("CRC32 0x%08x 0x%08x \n", *(word+7), readdata);
		#endif
		word = word+8;
		i = i+32;
	}

	while((no_of_bytes - i)/16)
	{
		CRC_multiCI_64BIT(*word, *(word+1));
		CRC_multiCI_64BIT(*(word+2), *(word+3));
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC16 0x%08x 0x%08x \n", *word, readdata);
			printf ("CRC16 0x%08x 0x%08x \n", *(word+1), readdata);
			printf ("CRC16 0x%08x 0x%08x \n", *(word+2), readdata);
			printf ("CRC16 0x%08x 0x%08x \n", *(word+3), readdata);
		#endif
		word = word+4;
		i = i+16;
	}

	while((no_of_bytes - i)/8)
	{
		CRC_multiCI_64BIT(*word, *(word+1));
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC8 0x%08x 0x%08x \n", *word, readdata);
			printf ("CRC8 0x%08x 0x%08x \n", *(word+1), readdata);
		#endif
		word = word+2;
		i = i+8;
	}

	while((no_of_bytes - i)/4)
	{
		CRC_multiCI_32BIT(*word);
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC4 0x%08x 0x%08x \n", *word, readdata);
		#endif
		word++;
		i = i+4;
	}

	if(no_of_bytes - i == 1)
	{
		CRC_multiCI_08BIT(data[i]);
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC 0x%08x 0x%08x \n", data32, readdata);
		#endif
	}
	else if(no_of_bytes - i == 2)
	{
		data32 = (data[i+1] << 8) | data[i];
		CRC_multiCI_16BIT(data32);
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC 0x%08x 0x%08x \n", data32, readdata);
		#endif
	}
	else if(no_of_bytes - i == 3)
	{
		data32 = (data[i+2] << 16) | (data[i+1] << 8) | data[i];
		CRC_multiCI_24BIT(data32);
		#ifdef DebugPrint
			readdata = CRC_multiCI_READ();
			printf ("CRC 0x%08x 0x%08x \n", data32, readdata);
		#endif
	}

	crc_temp = CRC_multiCI_READ();
	crc_temp = ~crc_temp; // Same as crc_temp = crc_temp ^ 0xFFFFFFFF;
#ifdef DebugPrint
	printf ("CRC 0x%08x \n", crc_temp);
#endif

	//Performance Counter
	PERF_END(PERF_CNTR_BASE, 1);

  return crc_temp;
}


#endif /* MISC_H_ */
