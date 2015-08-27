#include "support.h"
#include "pktbuff.h"
#include <netinet/udp.h>
#include "router.h"

int registers[ROUTER_LAST_REG];

struct intshort router_mac[4];

unsigned filter[ROUTER_OP_LUT_DST_IP_FILTER_TABLE_DEPTH], 
  packed_filter[ROUTER_OP_LUT_DST_IP_FILTER_TABLE_DEPTH];
int num_packed_filter=0;

struct  routing_item routing_table[ROUTER_OP_LUT_ROUTE_TABLE_DEPTH], packed_routing_table[ROUTER_OP_LUT_ROUTE_TABLE_DEPTH];
int num_packed_routing=0;


struct arp_item arp_table[ROUTER_OP_LUT_ARP_TABLE_DEPTH];
struct packed_arp_item packed_arp_table[ROUTER_OP_LUT_ARP_TABLE_DEPTH];
int num_packed_arp=0;

/****************************************************************************************************/

static inline u_int16_t do_checksum(char *data, int len) 
{
  int sum = 0;
  
  while (len > 1)  {
    /*  This is the inner loop */
    sum += * (unsigned short*) data;
    data += 2;
    len -= 2;
  }

  /*  Add left-over byte, if any */
  if( len > 0 )
    sum += * (unsigned char *) data;
  
  /*  Fold 32-bit sum to 16 bits */
  sum = (sum & 0xffff) + (sum >> 16);
  sum += (sum>>16);
  
  return ~((u_int16_t) sum);
}


static inline uint calc_ctrl(char* start_add, char* end_addr) {
  int bytes = end_addr - start_add;
  int rem = bytes & 0x7;
  return (0x101 >> rem) & 0xFF;
}

/*------------------------------------------------------------------------------------------*/
#define SOME_VALUE 16

// THE FOLLOWING 3 functions compact the table entries at the top of a separate array and count the number 
// of non-NULL entries to speed up the comparison, since the comparison is done by iterating among the entries (for now)

void update_packed_routing_table()
{
  int p=0, pp=0;
  for(p=0; p< ROUTER_OP_LUT_ROUTE_TABLE_DEPTH; p++)
    {
      if(routing_table[p].ip != 0)
	{
	  packed_routing_table[pp] = routing_table[p];
	  packed_routing_table[pp].ip = routing_table[p].ip &  routing_table[p].mask;
	  pp++;
	}
    }
  num_packed_routing = pp;
  num_packed_routing = SOME_VALUE;
  log("got %d packed routing entries\n", pp);
}

void update_packed_arp_table()
{
  int p=0, pp=0;
  for(p=0; p< ROUTER_OP_LUT_ARP_TABLE_DEPTH; p++)
    {
      if(arp_table[p].ip != 0)
	{
	  packed_arp_table[pp].ip = arp_table[p].ip;
	  packed_arp_table[pp].arp.hi_b4 = (arp_table[p].arp.hi_b2 << 16) | (arp_table[p].arp.lo_b4 >> 16);
	  packed_arp_table[pp].arp.lo_b2 = (arp_table[p].arp.lo_b4 & 0xffff);
	  pp++;
	}
    }
  num_packed_arp = pp;
  num_packed_arp = SOME_VALUE;
  log("got %d packed ARP entries\n", pp);
}

void update_packed_filter()
{
  int p=0, pp=0;
  for(p=0; p< ROUTER_OP_LUT_ARP_TABLE_DEPTH; p++)
    {
      if(filter[p] != 0)
	{
	  packed_filter[pp] = filter[p];
	  pp++;
	}
    }
  num_packed_filter = pp;
  num_packed_filter = SOME_VALUE;
  log("got %d packed filter entries\n", pp);
}


// update data structures based on register accesses
void process_admin(struct admin_header *head)
{
  unsigned regnum = (head->regnum - ROUTER_REG_BASE)/4;

  if(regnum >= ROUTER_LAST_REG)
    return;

  if(head->read)
    {
      head->val = registers[regnum];
      log("READ reg %x regnum %x (%s) val %x\n",   head->regnum, regnum, router_regs_str[regnum], head->val);
    }
  else
    {
      // ip or arp 0 means that it has been cleared
      unsigned v = ntohl(head->val);
      registers[regnum] = v;
      log("WRITE reg %x regnum %x (%s) val %x\n",  head->regnum, regnum, router_regs_str[regnum], head->val);
      
      switch(regnum)
	{
	case ROUTER_OP_LUT_ROUTE_TABLE_RD_ADDR_REG:    // index of the table being read, write-only
	  if(v >= ROUTER_OP_LUT_ROUTE_TABLE_DEPTH ) return;
	  registers[ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_IP_REG] = routing_table[v].ip;
	  registers[ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_MASK_REG] = routing_table[v].mask;
	  registers[ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_NEXT_HOP_IP_REG] = routing_table[v].next_hop;
	  registers[ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_OUTPUT_PORT_REG] = routing_table[v].port;	  
	  break;
	case ROUTER_OP_LUT_ROUTE_TABLE_WR_ADDR_REG:    // index of the table affected, write-only, triggers the write
	  if(v >= ROUTER_OP_LUT_ROUTE_TABLE_DEPTH ) return;
	  routing_table[v].ip = registers[ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_IP_REG];
	  routing_table[v].mask = registers[ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_MASK_REG];
	  routing_table[v].next_hop = registers[ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_NEXT_HOP_IP_REG];
	  routing_table[v].port = registers[ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_OUTPUT_PORT_REG];
	  update_packed_routing_table();
	  break;

	case ROUTER_OP_LUT_ARP_TABLE_RD_ADDR_REG:
	  if( v >= ROUTER_OP_LUT_ARP_TABLE_DEPTH ) return;
	  registers[ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_HI_REG] = arp_table[v].arp.hi_b2;
	  registers[ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_LO_REG] = arp_table[v].arp.lo_b4;
	  registers[ROUTER_OP_LUT_ARP_TABLE_ENTRY_NEXT_HOP_IP_REG] = arp_table[v].ip;
	  break;

	case ROUTER_OP_LUT_ARP_TABLE_WR_ADDR_REG:
	  if( v >= ROUTER_OP_LUT_ARP_TABLE_DEPTH ) return;
	  arp_table[v].arp.hi_b2 =registers[ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_HI_REG];
	  arp_table[v].arp.lo_b4 = registers[ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_LO_REG];
	  arp_table[v].ip = registers[ROUTER_OP_LUT_ARP_TABLE_ENTRY_NEXT_HOP_IP_REG];
	  update_packed_arp_table();
	  break;

	case ROUTER_OP_LUT_DST_IP_FILTER_TABLE_RD_ADDR_REG:
	  if(v >= ROUTER_OP_LUT_DST_IP_FILTER_TABLE_DEPTH) return;
	  registers[ROUTER_OP_LUT_DST_IP_FILTER_TABLE_ENTRY_IP_REG] = filter[v];
	  break;

	case ROUTER_OP_LUT_DST_IP_FILTER_TABLE_WR_ADDR_REG:
	  if(v >= ROUTER_OP_LUT_DST_IP_FILTER_TABLE_DEPTH) return;
	  filter[v] = registers[ROUTER_OP_LUT_DST_IP_FILTER_TABLE_ENTRY_IP_REG];
	  update_packed_filter();
	  break;

	default:
	  break;
	}
    }
}


// search functions involved in routing packets, could be sped up in many ways

int search_filter(int dest_ip)
{
  int p;
  for(p=0; p< num_packed_filter; p++)
    {
      if(filter[p] == dest_ip)
	return 1;
    }
  return 0;
}
	  
int search_ip(int dest_ip, int *next_hop_ip, short* next_port)
{
  int p;
  for(p=0; p<num_packed_routing; p++)
    {
      if(packed_routing_table[p].ip == (dest_ip & packed_routing_table[p].mask))
	{
	  *next_hop_ip = packed_routing_table[p].next_hop;
	  *next_port = packed_routing_table[p].port;
	  return 1;
	}
    }
  return 0;
}

int search_arp(int next_hop_ip, int* hi_b4, short* lo_b2)
{
  int p;
  for(p=0; p<num_packed_arp; p++)
    {
      if(packed_arp_table[p].ip == next_hop_ip)
	{
	  *hi_b4 = packed_arp_table[p].arp.hi_b4;
	  *lo_b2 = packed_arp_table[p].arp.lo_b2;
	  return 1;
	}
    }
  return 0;
}


/*******************************************************************************************/

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

      for(i=0; i<ROUTER_LAST_REG; i++)
	registers[i] = 0;
      for(i=0; i< ROUTER_OP_LUT_ROUTE_TABLE_DEPTH; i++)
	routing_table[i].ip = 0;
      for(i=0; i< ROUTER_OP_LUT_ARP_TABLE_DEPTH; i++)
	arp_table[i].ip = 0;
      for(i=0; i< ROUTER_OP_LUT_DST_IP_FILTER_TABLE_DEPTH; i++)
	filter[i] = 0;

      update_packed_routing_table();
      update_packed_arp_table();
      update_packed_filter();

      router_mac[0].hi_b4 = ntohl((ROUTER_OP_LUT_DEFAULT_MAC_0_HI <<16)  |  (ROUTER_OP_LUT_DEFAULT_MAC_0_LO >> 16));
      router_mac[0].lo_b2 = ntohs(ROUTER_OP_LUT_DEFAULT_MAC_0_LO && 0xffff);

      router_mac[1].hi_b4 = ntohl((ROUTER_OP_LUT_DEFAULT_MAC_1_HI <<16)  |  (ROUTER_OP_LUT_DEFAULT_MAC_1_LO >> 16));
      router_mac[1].lo_b2 = ntohs(ROUTER_OP_LUT_DEFAULT_MAC_1_LO && 0xffff);
      
      router_mac[2].hi_b4 = ntohl((ROUTER_OP_LUT_DEFAULT_MAC_2_HI <<16)  |  (ROUTER_OP_LUT_DEFAULT_MAC_2_LO >> 16));
      router_mac[2].lo_b2 = ntohs(ROUTER_OP_LUT_DEFAULT_MAC_2_LO && 0xffff);
      
      router_mac[3].hi_b4 = ntohl((ROUTER_OP_LUT_DEFAULT_MAC_3_HI <<16)  |  (ROUTER_OP_LUT_DEFAULT_MAC_3_LO >> 16));
      router_mac[3].lo_b2 = ntohs(ROUTER_OP_LUT_DEFAULT_MAC_3_LO && 0xffff);

    }
  nf_unlock(LOCK_INIT); 


  while(1) 
    {  
#ifndef DEBUG
      uint volatile* const loc7 = (uint*)(HEADER_FLUSH_W);
      next_packet = (t_addr*)((*loc7)|(1<<HEADER_MEM_SEL)); 
#else
      next_packet = nf_pktin_pop(); 
#endif

      if (nf_pktin_is_valid(next_packet)) 
	{
	  log("thread %d receiving packet addr %p time %d\n", nf_tid(), next_packet, nf_time());
	  
	  
	  struct ioq_header *dioq = (struct ioq_header *)next_packet;
	  struct al_ether_header *eth = (struct al_ether_header *)( ((char*)dioq) + sizeof(struct ioq_header));
	  char* out = (char*)dioq;
	  
	  // prepare to send, most likely behavior
	  unsigned int size = ntohs(dioq->byte_length);
	  char* start_addr= (char*)out;
	  char* end_addr = (char*)(out + size + sizeof(struct ioq_header));	  
	  uint ctrl = calc_ctrl(start_addr, end_addr);
	  end_addr--;

	  short src_port = htons(dioq->src_port);
	  short ether_type = ntohs(eth->ether_type);


	  if(ether_type == ADMIN_ETHER_TYPE)
	    {
	      struct admin_header *head = (struct admin_header *)(((char*)dioq) + sizeof(struct ioq_header) + ETHER_HDR_LEN); 
	      nf_lock(LOCK_DS0);
	      process_admin(head);
	      nf_unlock(LOCK_DS0);
	      if(head->read)
		{
		  short dport = src_port;
		  dioq->dst_port = htons(1 << dport);
		  head->reply = 1;
		  eth->ether_src.hi_b2 = 0xaabb;

		  log("sending reply to admin message\n");
		  do_send(start_addr, end_addr, ctrl);
		}
	      else
		{
		  log("finished processing write admin message\n");
		  nf_pktin_free(next_packet);
		}
	      continue;
	    }


	  if(src_port & 1) // packet is coming from a DMA port
	    {
		{
		  short dport = src_port & ~1;
		  dioq->dst_port = htons(1 << dport);
	  
		  log("packet from cpu forwarded directly\n");		      
		  registers[ROUTER_OP_LUT_NUM_CPU_PKTS_SENT_REG]++;
		  do_send(start_addr, end_addr, ctrl);
		  continue;
		}

	      nf_pktin_free(next_packet);
	      continue;
	    }

	  
	  // check if the dest mac address matches the ports of this router
	  if(!((eth->ether_dest.hi_b4 == router_mac[src_port].hi_b4 && eth->ether_dest.lo_b2 == router_mac[src_port].lo_b2) ||
	       (eth->ether_dest.hi_b4 & 0x00800000 )))  // broadcast
	    {
	      log("packet not destined to this router is dropped\n");
	      registers[  ROUTER_OP_LUT_NUM_WRONG_DEST_REG ]++;
	      nf_pktin_free(next_packet);
	      continue;
	    }


	  if(ether_type != ETHERTYPE_IP)
	    {
	      log("packet not IP is forwarded to host\n");
	      registers[  ROUTER_OP_LUT_NUM_NON_IP_RCVD_REG] ++;
	      goto to_host_label;
	    }

		  
	  struct al_iphdr *iph = (struct al_iphdr *)(((char*)dioq) + sizeof(struct ioq_header) + ETHER_HDR_LEN); 


	  // check the checksum
	  short old_check = iph->check;
	  iph->check  = 0;
	  short cur_check = do_checksum((char*) iph, 20);   // the reference router doesn't consider options i think

	  if(cur_check != old_check)
	    {
	      log("packet with bad checksum is dropped\n");
	      registers[  ROUTER_OP_LUT_NUM_BAD_CHKSUMS_REG ] ++;

	      nf_pktin_free(next_packet);
	      continue;
	    }


	  /* if the packet has any options or ver!=4 then send it to the cpu
	   * queue corresponding to the input queue*/
	  if(iph->version_ihl != 0x45)
	    {
	      log("packet not IPv4 or with options is forwarded to host\n");
	      registers[ROUTER_OP_LUT_NUM_BAD_OPTS_VER_REG] ++;
	      goto to_host_label;
	    }


	  /* if the TTL is 1 or 0, then send it to the cpu queue corresponding
	   * to the input queue*/
	  if(iph->ttl <= 1)
	    {
	      log("packet with too small TTL is forwarded to host\n");
	      registers[ROUTER_OP_LUT_NUM_BAD_TTLS_REG]++;
	      goto to_host_label;
	    }

	  /* if the ip destination address is in the destination filter list,
	   * then send it to the cpu queue corresponding to the input queue */
	  unsigned dest_ip = (iph->daddr_h << 16) | iph->daddr_l;
	  int found = search_filter(dest_ip);
	  
	  if(found)
	    {
	      log("packet filtered is forwarded to host\n");
	      registers[ROUTER_OP_LUT_NUM_FILTERED_PKTS_REG]++;
	      goto to_host_label;
	    }


	   /* Also send pkt to CPU if we don't find it in the ARP lookup or in
	   * the IP LPM lookup*/
	  int next_hop_ip=0;
	  short next_port=0;
	  int match = search_ip(dest_ip, &next_hop_ip, &next_port);
	  if(!match)
	    {
	      log("packet not matching routing table is forwarded to host\n");
	      registers[ROUTER_OP_LUT_LPM_NUM_MISSES_REG]++;
	      goto to_host_label;
	    }	  
	  
	  int hi_b4=0;
	  short lo_b2=0;
	  match = search_arp(next_hop_ip, &hi_b4, &lo_b2);	  
	  if(!match)
	    {
	      log("packet not matching ARP table is forwarded to host\n");
	      registers[ROUTER_OP_LUT_ARP_NUM_MISSES_REG]++;
	      goto to_host_label;
	    }

	  // fixup the port, ip, arp, ttl, and checksum
	  dioq->dst_port = ntohs(next_port);

	  eth->ether_dest.hi_b4 = hi_b4;
	  eth->ether_dest.lo_b2 = lo_b2;

	  iph->daddr_h = next_hop_ip;
	  iph->daddr_l = next_hop_ip;
	  iph->ttl--;

	  iph->check = do_checksum((char*) iph, 20);   // the reference router doesn't consider options i think, so header is 20 bytes

	  log("packet routed to destination\n");

	  do_send(start_addr, end_addr, ctrl);
	  continue;

	to_host_label:
	  log("packet sent to host (length %d)\n", size);

	  short dport = src_port | 1;
	  dioq->dst_port = htons(1 << dport);
	  
	  do_send(start_addr, end_addr, ctrl);
	  continue;
	
      }
    }

  while(1); 

  return 0;
}


const char* router_regs_str[] = {
  "ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_HI_REG",
  "ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_LO_REG",
  "ROUTER_OP_LUT_ARP_TABLE_ENTRY_NEXT_HOP_IP_REG",
  "ROUTER_OP_LUT_ARP_TABLE_RD_ADDR_REG",
  "ROUTER_OP_LUT_ARP_TABLE_WR_ADDR_REG",

  "ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_IP_REG",
  "ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_MASK_REG",
  "ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_NEXT_HOP_IP_REG",
  "ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_OUTPUT_PORT_REG",
  "ROUTER_OP_LUT_ROUTE_TABLE_RD_ADDR_REG",
  "ROUTER_OP_LUT_ROUTE_TABLE_WR_ADDR_REG",

  "ROUTER_OP_LUT_MAC_0_HI_REG",
  "ROUTER_OP_LUT_MAC_0_LO_REG",
  "ROUTER_OP_LUT_MAC_1_HI_REG",
  "ROUTER_OP_LUT_MAC_1_LO_REG",
  "ROUTER_OP_LUT_MAC_2_HI_REG",
  "ROUTER_OP_LUT_MAC_2_LO_REG",
  "ROUTER_OP_LUT_MAC_3_HI_REG",
  "ROUTER_OP_LUT_MAC_3_LO_REG",

  "ROUTER_OP_LUT_DST_IP_FILTER_TABLE_ENTRY_IP_REG",
  "ROUTER_OP_LUT_DST_IP_FILTER_TABLE_RD_ADDR_REG",
  "ROUTER_OP_LUT_DST_IP_FILTER_TABLE_WR_ADDR_REG",

  "ROUTER_OP_LUT_ARP_NUM_MISSES_REG",
  "ROUTER_OP_LUT_LPM_NUM_MISSES_REG",
  "ROUTER_OP_LUT_NUM_CPU_PKTS_SENT_REG",
  "ROUTER_OP_LUT_NUM_BAD_OPTS_VER_REG",
  "ROUTER_OP_LUT_NUM_BAD_CHKSUMS_REG",
  "ROUTER_OP_LUT_NUM_BAD_TTLS_REG",
  "ROUTER_OP_LUT_NUM_NON_IP_RCVD_REG",
  "ROUTER_OP_LUT_NUM_PKTS_FORWARDED_REG",
  "ROUTER_OP_LUT_NUM_WRONG_DEST_REG",
  "ROUTER_OP_LUT_NUM_FILTERED_PKTS_REG",

  "ROUTER_LAST_REG"
};
