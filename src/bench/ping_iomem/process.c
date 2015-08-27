
#include "common.h"
#include "pktbuff.h"
#include "dev.h"
#include "support.h"


int send_pkt(char* data, unsigned len)
{
  char* start_addr = data;
  char* end_addr = &(data[len]);
  uint ctrl = calc_ctrl(start_addr, end_addr);
  end_addr--;

  do_send(start_addr, end_addr, ctrl);
  return len;
}

static void print_mac(unsigned char *mac) {
  int i = 0;
  for (;;) {
    log("%02hhx", mac[i]);

    i++;
    if (i == ETH_ALEN) {
      break;
    }

    log(":");
  }
}

static void print_ip(unsigned char *ip) {
  int i = 0;
  for (;;) {
    log("%hhu", ip[i]);

    i++;
    if (i == 4) {
      break;
    }

    log(".");
  }
}


u_int16_t ones_complement_sum(char *data, int len) {
  u_int32_t sum = 0;
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
  while (sum>>16)
    sum = (sum & 0xffff) + (sum >> 16);
  
  return (u_int16_t) sum;
}


int process_arp(struct net_iface *iface, struct ioq_header *ioq, struct ether_header *eth, struct pkt_buff *pkt) {
  log("Process arp\n");

  struct ether_arp *etharp = (struct ether_arp *)pkt_pull(pkt, sizeof(struct ether_arp));
  if (!etharp) {
    return -1;
  }

  log("remaining bytes: %u\n", pkt->len);

  struct arphdr *arp = &etharp->ea_hdr;
  
  log("hdr: %hu pro: %hx hln: %hhu pln: %hhu op: %hu\n",
         ntohs(arp->ar_hrd), 
         ntohs(arp->ar_pro), 
         arp->ar_hln, 
         arp->ar_pln, 
         ntohs(arp->ar_op));

  unsigned short op = ntohs(arp->ar_op);

  // sanity checks
  if (ntohs(arp->ar_hrd) != 1 || // ethernet
      ntohs(arp->ar_pro) != 0x800 || // ipv4
      arp->ar_hln != 6 || // 6 byte MACs
      arp->ar_pln != 4 || // 4 byte IPs
      (op != 1 && op != 2)  // valid operations
      ) {
    log("arp failed sanity check");
    return -2;
  }

#ifdef CONTEXT_SIM
  log("sender "); print_mac(etharp->arp_sha); log(" ");
  print_ip(etharp->arp_spa); log("\n");

  log("target "); print_mac(etharp->arp_tha); log(" ");
  print_ip(etharp->arp_tpa); log("\n");
#endif

  struct arp_entry *entry = arp_lookup(&iface->arp, etharp->arp_spa);
  if (entry) {
    // update table with new hardware address
    memcpy(entry->mac, etharp->arp_sha, ETH_ALEN);
  }
  
  // check if this iface is the target
  if (memcmp(etharp->arp_tpa, iface->ip, 4) == 0) { /* marvin ETH_ALEN */
    log("I am target\n");
    if (!entry) {
      entry = arp_add(&iface->arp);
      memcpy(entry->mac, etharp->arp_sha, ETH_ALEN);
      memcpy(entry->ip, etharp->arp_spa, 4);
    }

    if (op == ARPOP_REQUEST) {
      // Create and send a reply.
      pkt_push_all(pkt);

      struct pkt_buff reply = *pkt;

      // ARP was designed to easily build replys
      // from the request.

      struct ioq_header *reply_ioq = (struct ioq_header *)pkt_pull(&reply, sizeof(struct ioq_header));
      fill_ioq(reply_ioq, ioq->src_port, reply.len);

      struct ether_header *reply_ether = (struct ether_header *)pkt_pull(&reply, sizeof(struct ether_header));
      struct ether_arp *reply_arp = (struct ether_arp *)pkt_pull(&reply, sizeof(struct ether_arp));
      // copy source ip and mac to target
      memcpy(reply_arp->arp_tha, reply_arp->arp_sha, ETH_ALEN);
      memcpy(reply_arp->arp_tpa, reply_arp->arp_spa, 4);
      // copy this interface's mac and ip to source
      memcpy(reply_arp->arp_sha, iface->mac, ETH_ALEN);
      memcpy(reply_arp->arp_spa, iface->ip, 4);

      reply_arp->ea_hdr.ar_op = htons(ARPOP_REPLY);
      memcpy(reply_ether->ether_shost, iface->mac, ETH_ALEN);
      memcpy(reply_ether->ether_dhost, reply_arp->arp_tha, ETH_ALEN);
      
      pkt_push_all(&reply);
      send_pkt(reply.data, reply.len);

      return 1;
    }
  }
  else
    log("I am not target (%d)\n", ETH_ALEN);

  return -3;
}

int process_icmp(struct net_iface *iface, struct ioq_header *ioq, struct ether_header *eth, struct iphdr *ip, struct pkt_buff *pkt) {
  log("Process ICMP of size %u\n", pkt->len);
  // check min size
  if (pkt->len < 8) {
    return -4;
  }

  struct icmphdr *icmp = (struct icmphdr *)pkt->head;

  // verify checksum
  u_int16_t check = ones_complement_sum((char *)icmp, pkt->len);
  if (check != 0xFFFF) {
    log("Checksum failed %x\n", check);
    return -5;
  }
  
  if( icmp->code != 0)
    return -16;

  if (icmp->type == ICMP_ECHO) {
    log("Is ping request\n");
    
    // Create and send a reply.
    struct pkt_buff reply = *pkt;
    pkt_push_all(&reply);

    struct ioq_header *rioq = (struct ioq_header *)pkt_pull(&reply, sizeof(struct ioq_header));
    unsigned short reply_bytes = (unsigned short) reply.len;
    struct ether_header *reth = (struct ether_header *)pkt_pull(&reply, sizeof(struct ether_header));
    struct iphdr *rip = (struct iphdr *)pkt_pull(&reply, sizeof(struct iphdr));
    struct icmphdr *ricmp = (struct icmphdr *)pkt_pull(&reply, sizeof(struct icmphdr));

    fill_ioq(rioq, ioq->src_port, reply_bytes);

    // fill ethernet header
    memcpy(reth->ether_shost, iface->mac, ETH_ALEN);
    memcpy(reth->ether_dhost, eth->ether_shost, ETH_ALEN);
    reth->ether_type = htons(ETHERTYPE_IP);

    // fill ip header
#ifndef DEBUG
    rip->version_ihl = 0x45;
//    rip->ihl = 5;
#else
    rip->version = 4;
    rip->ihl = 5;
#endif
    rip->tos = 0; // not sure about this one
    rip->tot_len = htons(20 + pkt->len);
    rip->id = 0; // not sure about this one
    rip->frag_off = 0;
    rip->ttl = 64;
    rip->protocol = IPPROTO_ICMP;
#ifndef DEBUG
    rip->saddr_h = ip->daddr_h;
    rip->saddr_l = ip->daddr_l;
    rip->daddr_h = ip->saddr_h;
    rip->daddr_l = ip->saddr_l;
#else
    rip->saddr = ip->daddr;
    rip->daddr = ip->saddr;
#endif
    rip->check = ~ones_complement_sum((char *)rip, 20);

    // edit icmp, based on icmp request
    ricmp->type = 0;
    // update checksum instead of recompute
    u_int32_t acc = (u_int32_t) ~ntohs(ricmp->checksum) + (u_int32_t) ~0x0400;
    while (acc >> 16)
      acc = (acc & 0xffff) + (acc >> 16);
    ricmp->checksum = htons(~acc);
        
    pkt_push_all(&reply);
    send_pkt(reply.data, reply.len);
    
    return 1;
  }
  return -6;
}


int process_ip(struct net_iface *iface, struct ioq_header *ioq, struct ether_header *eth, struct pkt_buff *pkt) {
  log("Process ip\n");

  struct iphdr *ip = (struct iphdr *)pkt_pull(pkt, sizeof(struct iphdr));
  if (!ip) {
    return -7;
  }

  int ihl = 0;
#ifndef DEBUG
  ihl = ip->version_ihl&0xf;
  if ((ip->version_ihl&0xf0) != 0x40 ||
      ihl < 5) {
    return -8;
  }
#else
  ihl = ip->ihl;
  if (ip->version != 4 ||
      ihl < 5) {
    return -8;
  }
#endif

  int options_size = ihl * 4 - sizeof(struct iphdr);
  void *options = pkt_pull(pkt, options_size);
  if (!options) {
    log("Options truncated. size=%d\n", options_size);
    return -9;
  }

  // verify checksum
  u_int16_t check = ones_complement_sum((char *)ip, ihl * 4);
  if (check != 0xFFFF) {
    log("Checksum failed %x\n", check);
    return -10;
  }

  if (ntohs(ip->tot_len) != ihl * 4 + pkt->len) {
    log("Packet data truncated %d instead of %d\n", ntohs(ip->tot_len), ihl * 4 + pkt->len);
    return -11;
  }

  int result = -12;

  switch (ip->protocol) {
  case IPPROTO_ICMP:
    result = process_icmp(iface, ioq, eth, ip, pkt);
    break;
  }
  return result;
}

void process_pkt(struct net_iface *iface, void* data) {
  struct pkt_buff pkt;
  struct ioq_header *ioq = (struct ioq_header *)data;
  unsigned int size = ntohs(ioq->byte_length);
    int result = -2;

  log("ioq_hdr: dst=%hx words=%hu src=%hu bytes=%hu\n", ntohs(ioq->dst_port), ntohs(ioq->word_length), ntohs(ioq->src_port), size);
  pkt_fill(&pkt, (char*)data, size + sizeof(struct ioq_header));

  pkt_pull(&pkt, sizeof(struct ioq_header));
  struct ether_header *eth = (struct ether_header *)pkt_pull(&pkt, ETHER_HDR_LEN);
  if (eth) {
#ifdef CONTEXT_SIM
    log("dest: "); print_mac(eth->ether_dhost); log("\n");
    log("source: "); print_mac(eth->ether_shost); log("\n");
    log("eth_proto: %hx\n", ntohs(eth->ether_type));
#endif


    switch (ntohs(eth->ether_type)) {
    case ETHERTYPE_ARP:
      result = process_arp(iface, ioq, eth, &pkt);
      break;
    case ETHERTYPE_IP:
      result = process_ip(iface, ioq, eth, &pkt);
      break;
    default:
      result = -13;
      break;
    }
  }


  // We failed to send a reply for some reason. Echo the packet  
  if (result <= 0) { 
    pkt_push_all(&pkt);

    char* ptr;
    struct pkt_buff reply = pkt;
    struct ioq_header *dioq;


    dioq = (struct ioq_header *)pkt_pull(&reply, sizeof(struct ioq_header));
    fill_ioq(dioq, ioq->src_port, ioq->byte_length);

    ptr = (char*)reply.head;
    ptr[0] = -result;
    log("Set result %d\n", -result);

    send_pkt(reply.data, reply.total_size);
  }

}

#ifdef DEBUG
char buf1[] = {0xff,0xff,0xff,0xff,0xff,0xff,0x00,0xff,0x3b,0x5d,0xdb,0x10,0x08,0x06,0x00,0x01, //
	      0x08,0x00,0x06,0x04,0x00,0x01,0x00,0xff,0x3b,0x5d,0xdb,0x10,0x0a,0x00,0x00,0x01,
	      0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0x00,0x02};
char buf2[] = {0x11,0x22,0x33,0x44,0x55,0x66,0x00,0xff,0x3b,0x5d,0xdb,0x10,0x08,0x00,0x45,0x00,
	       0x00,0x54,0x00,0x00,0x40,0x00,0x40,0x01,0x26,0xa7,0x0a,0x00,0x00,0x01,0x0a,0x00,
	       0x00,0x02,0x08,0x00,0x57,0xde,0x52,0x5b,0x00,0x01,0x33,0x12,0xa3,0x48,0x8b,0x67,
	       0x01,0x00,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,
	       0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,
	       0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,
	       0x36,0x37};


char buf1_ioq[] = {0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x2A,
              0xff,0xff,0xff,0xff,0xff,0xff,0x00,0xff,0x3b,0x5d,0xdb,0x10,0x08,0x06,0x00,0x01, //
	      0x08,0x00,0x06,0x04,0x00,0x01,0x00,0xff,0x3b,0x5d,0xdb,0x10,0x0a,0x00,0x00,0x01,
	      0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0x00,0x02};
char buf2_ioq[] = {0x00,0x00,0x00,0x0D,0x00,0x00,0x00,0x62,
               0x11,0x22,0x33,0x44,0x55,0x66,0x00,0xff,0x3b,0x5d,0xdb,0x10,0x08,0x00,0x45,0x00,
	       0x00,0x54,0x00,0x00,0x40,0x00,0x40,0x01,0x26,0xa7,0x0a,0x00,0x00,0x01,0x0a,0x00,
	       0x00,0x02,0x08,0x00,0x57,0xde,0x52,0x5b,0x00,0x01,0x33,0x12,0xa3,0x48,0x8b,0x67,
	       0x01,0x00,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,
	       0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,
	       0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,
	       0x36,0x37};

char buf3_ioq_checksum_fail[] = {0x00,0x00,0x00,0x0D,0x00,0x00,0x00,0x62,
               0x11,0x22,0x33,0x44,0x55,0x66,0x00,0xff,0x3b,0x5d,0xdb,0x10,0x08,0x00,0x45,0x00,
	       0x00,0x54,0x00,0x00,0x40,0x00,0x40,0x01,0x26,0xa7,0x0a,0x00,0x00,0x01,0x0a,0x00,
	       0x00,0x02,0x08,0x00,0x57,0xde,0x52,0x5b,0x00,0x01,0x33,0x12,0xa3,0x48,0x8b,0x67,
	       0x01,0x00,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,
	       0x16,0x17,0x18,0x19,0x1a,0x2b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,
	       0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,
	       0x36,0x37};

char buf4[] = {0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x3C,0x00,0xdd,0xdd,0xdd,0xdd,0xdd,0x00,0x55,
	       0x55,0x55,0x55,0x55,0x08,0x00,0x45,0x00,0x00,0x28,0x00,0x00,0x00,0x00,0x40,0x00,
	       0xf9,0x82,0xc0,0xa8,0x00,0x02,0xc0,0xa8,0x00,0x01,0x23,0x24,0x25,0x26,0x27,0x28,
	       0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
	       0x39,0x3a,0x3b,0x3c};

char buf5[] = {0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x2a, 
	       0x00,0xdd,0xdd,0xdd,0xdd,0xdd,0x00,0x55,
	       0x55,0x55,0x55,0x55,0x08,0x00,
	       0x45,0x00,0x00,0x1c,0x45,0x67,0x00,0x00,0x40,0x01,0x2b,0x7a,0x0a,0x00,0x00,0x01,
0xff,0xff,0xff,0xff,0x08,0x00,0xb2,0x98,0x45,0x67,0x00,0x00

};
#define LEN4 68
#endif

/*
const char* mystr = "hello world";

void test_this()
{
  char sum=0;
  int i;
  volatile char*mydest = (char*)0x4041000;
  for(i=0; i<5; i++)
    {
      mydest[i] = mystr[2*i];
      sum += mydest[i];  
    }
}*/



int main(void)
{
  struct net_iface iface;
#ifndef DEBUG
  t_addr* next_packet;
  int i=nf_tid();
  
  if(i== 0)
    {
      nf_lock(LOCK_INIT); // should get it on the first attempt
      nf_pktout_init();
      nf_pktin_init();
    }
  else 
    {
      nf_stall_a_bit();
      nf_lock(LOCK_INIT); // should not get it
    }
   nf_unlock(LOCK_INIT); 
   
#endif

   //test_this();

   // iface is not shared, it's on the stack
   arp_init(&iface.arp);
   
   iface.mac[0] = 0x11;
   iface.mac[1] = 0x22;
   iface.mac[2] = 0x33;
   iface.mac[3] = 0x44;
   iface.mac[4] = 0x55;
   iface.mac[5] = 0x66;
   
   iface.ip[0] = 10;
   iface.ip[1] = 0;
   iface.ip[2] = 0;
   iface.ip[3] = 2;
   
#ifndef DEBUG
  //for(i=0; i<2; i++)
  while(1)
    {
      next_packet = nf_pktin_pop();  // test for next_packet
      if(!nf_pktin_is_valid(next_packet))
        continue;

      process_pkt(&iface, next_packet);

      nf_pktin_free(next_packet);
    }
#else
  process_pkt(&iface, buf5);
 // process_pkt(&iface, buf3_ioq_checksum_fail);
#endif

  return 0;
}
