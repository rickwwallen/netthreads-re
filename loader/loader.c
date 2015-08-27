/* ****************************************************************************
 * $Id: regread.c 2267 2007-09-18 00:09:14Z grg $
 *
 * Module: regread.c
 * Project: NetFPGA 2 Register Access
 * Description: Reads a register
 *
 * Change history:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <net/if.h>

#include "nf2.h"
#include "nf2util.h"

#define PATHLEN		80

#define DEFAULT_IFACE	"nf2c0"

/* Global vars */
static struct nf2device nf2;
static int verbose = 0;
static int force_cnet = 0;

/* Function declarations */
void readRegisters (int , char **);
void processArgs (int , char **);
void usage (void);
int mem_addr = 0;

#define FOR_REAL 1
#define DATA_OFFSET  0x04040000
#define INSTR_OFFSET 0
#define HAZARD_OFFSET 0

int main(int argc, char *argv[])
{
  char line[PATHLEN];
  int offset = 0;
  int doing_data = 0;
  int doing_hazards = 0;
  int enter_debug = 1;

  nf2.device_name = DEFAULT_IFACE;
  
  if((argc != 3) && (argc != 4))
    {
      printf("<-d for data/-i for instructions> <name of file to load>\n");
      printf("optional 4th argument to prevent entering debug\n");
      exit(1);
    }

  if(!strcmp(argv[1], "-d"))
    { 
      printf("Loading data\n");
      offset = DATA_OFFSET;
      doing_data = 1;
    }
  else if(!strcmp(argv[1], "-i"))
    { 
      printf("Loading instructions\n");
      offset = INSTR_OFFSET;
    }
  else if(!strcmp(argv[1], "-h"))
    { 
      printf("Loading hazards\n");
      offset = HAZARD_OFFSET;
      doing_hazards = 1;
    }
  else
    {
      printf("Bad 1st argument. Exiting.");
      exit(1);
    }
	
  printf("Opening file %s\n", argv[2]);
  FILE* input = fopen(argv[2], "r");
  assert(input && "File not found");

#if FOR_REAL
  // Open the interface if possible
  if (check_iface(&nf2))
    {
      exit(1);
    }
  if (openDescriptor(&nf2))
    {
      exit(1);
    }
#endif

  unsigned addr = (0x2000408);

  unsigned word_addr, word_val;
  int ret = fscanf(input, "%x : %x;", &word_addr, &word_val);
  int counter = 0;

  while(!feof(input))
    {
      if(ret ==2)
	{
	  if(!doing_hazards)
	    word_addr *= 4;

	  word_addr += offset;

	  if(counter == 0)
	    printf("Loading %08x:", word_addr);
	  //printf(" %08x", word_val);

	  counter++;
	  if(counter == 8)
	    { printf("\n"); counter = 0; /*usleep(1);*/}
	  
#if FOR_REAL 
	  //printf("CK ");
	  // enter word programming state
	  if(!doing_hazards)
	    ret = writeReg(&nf2, addr, 0x00000006); // reset and prog
	  else
	    ret = writeReg(&nf2, addr, 0x0000000A); // reset and hazard

	  //ret = writeReg(&nf2, addr, 0x00000007); // reset prog and debug
	  assert(ret == 0);

	  // enter address
	  ret = writeReg(&nf2, addr, word_addr);
	  assert(ret == 0);
		
	  // enter value
	  //writeReg(&nf2, addr, 0x3c1c0404 /*0x27bdfffc*/ );
	  ret = writeReg(&nf2, addr, word_val);
	  assert(ret == 0);

	  // dummy write, boot_iwe
	  ret = writeReg(&nf2, addr, 0);
	  assert(ret == 0);

	  /*usleep(2);*/
#endif
	}
      else
	{
	  fgets(line, PATHLEN-1, input);

	  printf("\nSkipping %s\n", line);
	}
      ret = fscanf(input, "%x : %x;", &word_addr, &word_val);
    }
	
  if(doing_data)
    {
      printf("Assuming instructions are loaded, will start the processor.\n");
      printf("Otherwise, reload the instructions and the data.\n");
      if((argc == 4))
	{
	  printf("not using debugging mode (required for continuous execution)\n");
	  enter_debug = 0;
	}
      else
	{
	  enter_debug = 1;
	  printf("using debugging mode, will not execute continuously\n");
	}

#if FOR_REAL
      if(enter_debug)
	ret = writeReg(&nf2, addr, 0x00000003);
      else
	{
	  //usleep(10);
	  ret = writeReg(&nf2, addr, 0x00000000);  // unreset processors
	}

      assert(ret == 0);
#endif
    }
  else
    {
      printf("Just loaded %s. "
	     "Now please load the data. "
	     "Processor was not started.\n", doing_hazards?"hazards":"instructions");
    }

#if FOR_REAL
  closeDescriptor(&nf2);
#endif

  return 0;
}

