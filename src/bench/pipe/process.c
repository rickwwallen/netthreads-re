#include "support.h"
#include "pktbuff.h"


int main(void)
{
  t_addr* next_packet;
  int i = 0;

  int mytid = nf_tid();

  if(mytid != 0)
    {
      nf_stall_a_bit();
      nf_lock(LOCK_INIT); // should not get it
    }
  else
    {
      nf_lock(LOCK_INIT); // should get it immediately
      nf_pktin_init();      
    }

  nf_unlock(LOCK_INIT); 

  while(1) 
    {
      next_packet = nf_pktin_pop();
      if (nf_pktin_is_valid(next_packet)) {
        struct ioq_header *dioq = (struct ioq_header *)next_packet;
        
	// set the destination port
	short dport = htons(dioq->src_port) ^ 2; 
	dioq->dst_port = htons(1 << dport); 

  
	unsigned int size = ntohs(dioq->byte_length);
	char* start_addr= next_packet;
	char* end_addr = (char*)(start_addr + size + sizeof(struct ioq_header));       
	uint ctrl = calc_ctrl(start_addr, end_addr);
	end_addr--;

	do_send(start_addr, end_addr, ctrl);  // packet slots sent are recycled automatically
      }
    }


  return 0;
}
