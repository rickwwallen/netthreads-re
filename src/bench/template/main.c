#include <stdio.h>
#include <arpa/inet.h>
#include <support.h>

/*
  This is the skeleton of a typical NetThreads application. 
  There is a wealth of undocumented routines in the ../common folder.
  All received packets are preceeded by an ioq_header (8 bytes), followed
  usually by an ethernet header, ip header, etc.
  
  A restricted version of the standard C library is precompiled. It
  should fulfil most needs. There is no support for printf or file I/O
  on the netfpga.  You can however use the log() function as a
  printf(). This function will be omitted when compiling with:
  make. Compiling with "make CONTEXT=sw" will produce an executable
  for an executable for the machine you are using will be produced. It
  will be single threaded and has the option of reading packets either
  from a packet trace or from the network (by default using tap
  devices, see the sw_* files in the
  netthreads/compiler/src/bench/common/ folder). Using this mechanism,
  you can run the exact same code on the host machine (no changes
  necessary) to verify that the functionnality is correct.

  Software is very flexible: a number of deep packet inspection
  programs have been written with this framework.
*/

// all threads start here, NetThreads 1.0 has 8 threads (4 per CPU)
// instructions from the 4 thread on a CPU are interleaved in a
// round-robin fashion. use compiler/bin/mips-mips-elf-objdump -h to
// see the memory layout of your compiled application and support.h to
// see where some controls are memory-mapped.
int main(void)  
{   
  int mytid = nf_tid();
  
  if(mytid != 0)
    {
      nf_stall_a_bit();
      nf_lock(LOCK_INIT); // should not get it
    }
  else
    {
#ifndef DEBUG
      nf_lock(LOCK_INIT); // should get it on the first attempt
      nf_pktout_init();
      nf_pktin_init();
#endif
      sp_init_mem_single();  // initialize the multithreaded memory allocator

      // perform memory allocation for initialization purposes
      // only use sp_free() and sp_malloc()
      // two implementations of these functions exists. If you prefer the STANDARD_MALLOC
      // from ../common/memory.c, you should not perform sp_init_mem_single() nor sp_init_mem_pool().

      // finalize the initialization of the multithreaded memory allocator
      // since each thread allocates from its private head and adds to its heap
      // upon memory free, the heaps can get unbalanced if care is not taken
      sp_init_mem_pool();  
    }
  nf_unlock(LOCK_INIT); 


  while(1)
    { 
      // get the time if you need it
      uint t = nf_time();  // 32-bit unsigned wrap-around time


      nf_lock(LOCK_DS0);   // valid lock identifiers are integers from 0 to 15 inclusively
      // do some synchronized action here
      nf_unlock(LOCK_DS0);

      t_addr* next_packet = nf_pktin_pop();  // get the next available packet, this is non-blocking call
      if (nf_pktin_is_valid(next_packet)) { // test if we have a packet

	// process the packet

        nf_pktin_free(next_packet);  // free this packet from the input memory
      }
    }

  // rever reached
  return 0;
}
