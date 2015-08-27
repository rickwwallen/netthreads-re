#include <support.h>

void nf_unlock(int id)
{
#ifndef DEBUG
#if HAS_LOCKS
  volatile uint* loc = (uint*)((uint)UN_LOCK);  
  *loc  = id;
#endif
#else
  log("UNLOCKING LOCK %d\n", id);
#endif
}

// has to write twice to the same register to avoid written_to hazards with
// the commit signal in the register file
void nf_lock(int id)
{
#ifndef DEBUG
#if HAS_LOCKS
  volatile uint* loc = (uint*)((uint)GET_LOCK);  
  *loc  = id;
#endif
#else
  log("LOCKING LOCK %d\n", id);
#endif
}


int nf_tid()
{
#ifndef DEBUG
  asm(".set    noreorder");
  asm("addu $2,$26,$0"); 
  asm("jr $31"); 
#endif
  return 0;
}

void relax()
{
#ifndef DEBUG 
  int i;
	int test = nf_time() & 0x07;
	for(i=0; i<test; i++)
	{
		asm("nop"
				:
				:
				);
	}
	asm("nop\n\t" 
      "nop\n\t" 
      "nop\n\t" 
      "nop\n\t" 
      "nop\n\t" 
      "nop\n\t" 
      "nop\n\t" 
      "nop\n\t" 
      "nop\n\t" 
      "nop" 
      : 
      :  );  
#endif 
}


void mytest ()
{
#ifndef DEBUG 
	      asm(".set    noreorder");
	      asm("nop");
	      asm("nop");
	      asm("nop");
	      asm("nop");
	      asm("nop");
	      asm("lui $26,0x7ff"); 
	      asm("ori $26,$26,0xfdbc"); 
	      asm("lw $26,0($26)"); 
	      asm("lui $26,0x7ff"); 
	      asm("ori $26,$26,0x759c"); 
	      asm("lw $26,0($26)"); 
	      asm("jr $31");
	      asm("nop");
	      asm("nop");
	      asm("nop");
	      asm("nop");
	      asm("nop");
	      asm("nop");
	      asm("nop");
#endif
}

void trap()
{
  uint   volatile *loc10 = (uint*)((uint)(COMMON_TIME));
  *loc10 = 0;
}

t_addr* nf_pktin_pop()
{
#ifndef DEBUG
  uint volatile*loc1 = (uint*)(HEADER_FLUSH_W);
  return (t_addr*)((*loc1)|(1<<HEADER_MEM_SEL)); // or the mem-id bit to the base address
#else
  extern char* sw_pktin_pop();
  return (sw_pktin_pop()); 	/** sw_pkt_io.cc**/
#endif
}

void nf_pktin_free(t_addr* val)
{
#ifndef DEBUG
      uint volatile*loc1 = (uint*)(HEADER_RETURN_W);
      *loc1 = (uint)val;
#else
  return;
#endif
}
