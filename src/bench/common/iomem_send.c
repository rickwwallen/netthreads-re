#include "support.h"

volatile  uint    * const loc1 = (uint*)((uint)(CTRL_START));
volatile   uint    * const loc2 = (uint*)((uint)(CTRL_END));
volatile   uint    * const loc3 = (uint*)((uint)(CTRL_END_W));
volatile   uint    * const loc4 = (uint*)((uint)(SEND_OUT_W));
volatile   uint * const loc5 = (uint*)(HEADER_RETURN_W);

// this has to be done in one shot because of iomem_stall in the interconnect that tries to improve the sending
// this code should remain as is unless the hardware is changed
void do_send(char* start_addr, char* end_addr, uint ctrl)
{
#ifndef DEBUG
  t_addr* last_packet;
  *loc1 = (uint)start_addr;
  *loc2 = (uint)end_addr;
  *loc3 = ctrl;
  last_packet = (t_addr*)((*loc4)); // this should be the only memory mapped read to correspond to the hardware  

  *loc5 = (uint)last_packet;  // recycle the packet address to be managed in hardware, the user could prefer to have self-managed buffers in some cases
#else
	extern void sw_pktout_send(char*, char*);
	sw_pktout_send(start_addr, end_addr);
#endif
}
