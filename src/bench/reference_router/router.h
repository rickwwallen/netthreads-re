
//Number of entrties in the ARP table
#define ROUTER_OP_LUT_ARP_TABLE_DEPTH             32

//Number of entrties in the routing table table
#define ROUTER_OP_LUT_ROUTE_TABLE_DEPTH           32

//Number of entries in the destination IP filter table
#define ROUTER_OP_LUT_DST_IP_FILTER_TABLE_DEPTH   32

//Default MAC address for port 0
#define ROUTER_OP_LUT_DEFAULT_MAC_0_HI            0xcafe
#define ROUTER_OP_LUT_DEFAULT_MAC_0_LO            0xf00d0001

//Default MAC address for port 1
#define ROUTER_OP_LUT_DEFAULT_MAC_1_HI            0xcafe
#define ROUTER_OP_LUT_DEFAULT_MAC_1_LO            0xf00d0002

//Default MAC address for port 2
#define ROUTER_OP_LUT_DEFAULT_MAC_2_HI            0xcafe
#define ROUTER_OP_LUT_DEFAULT_MAC_2_LO            0xf00d0003

//Default MAC address for port 3
#define ROUTER_OP_LUT_DEFAULT_MAC_3_HI            0xcafe
#define ROUTER_OP_LUT_DEFAULT_MAC_3_LO            0xf00d0004

#define ADMIN_ETHER_TYPE                                            0x7fff

#define ROUTER_REG_BASE                                             0x2000100

enum router_regs {
  ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_HI_REG=0,
  ROUTER_OP_LUT_ARP_TABLE_ENTRY_MAC_LO_REG,
  ROUTER_OP_LUT_ARP_TABLE_ENTRY_NEXT_HOP_IP_REG,
  ROUTER_OP_LUT_ARP_TABLE_RD_ADDR_REG,
  ROUTER_OP_LUT_ARP_TABLE_WR_ADDR_REG,

  ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_IP_REG,
  ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_MASK_REG,
  ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_NEXT_HOP_IP_REG,
  ROUTER_OP_LUT_ROUTE_TABLE_ENTRY_OUTPUT_PORT_REG,
  ROUTER_OP_LUT_ROUTE_TABLE_RD_ADDR_REG,    // index of the table being read, write-only
  ROUTER_OP_LUT_ROUTE_TABLE_WR_ADDR_REG,    // index of the table affected, write-only, triggers the write

  ROUTER_OP_LUT_MAC_0_HI_REG,
  ROUTER_OP_LUT_MAC_0_LO_REG,
  ROUTER_OP_LUT_MAC_1_HI_REG,
  ROUTER_OP_LUT_MAC_1_LO_REG,
  ROUTER_OP_LUT_MAC_2_HI_REG,
  ROUTER_OP_LUT_MAC_2_LO_REG,
  ROUTER_OP_LUT_MAC_3_HI_REG,
  ROUTER_OP_LUT_MAC_3_LO_REG,

  ROUTER_OP_LUT_DST_IP_FILTER_TABLE_ENTRY_IP_REG,
  ROUTER_OP_LUT_DST_IP_FILTER_TABLE_RD_ADDR_REG,
  ROUTER_OP_LUT_DST_IP_FILTER_TABLE_WR_ADDR_REG,

  ROUTER_OP_LUT_ARP_NUM_MISSES_REG,
  ROUTER_OP_LUT_LPM_NUM_MISSES_REG,
  ROUTER_OP_LUT_NUM_CPU_PKTS_SENT_REG,
  ROUTER_OP_LUT_NUM_BAD_OPTS_VER_REG,
  ROUTER_OP_LUT_NUM_BAD_CHKSUMS_REG,
  ROUTER_OP_LUT_NUM_BAD_TTLS_REG,
  ROUTER_OP_LUT_NUM_NON_IP_RCVD_REG,
  ROUTER_OP_LUT_NUM_PKTS_FORWARDED_REG,
  ROUTER_OP_LUT_NUM_WRONG_DEST_REG,
  ROUTER_OP_LUT_NUM_FILTERED_PKTS_REG,
  
  ROUTER_LAST_REG
};

extern const char* router_regs_str[];


struct intshort {
  int hi_b4;
  short lo_b2;
} __attribute__ ((__packed__)); 

struct shortint {
  short hi_b2;
  int lo_b4;
} __attribute__ ((__packed__));;

struct al_ether_header
{
  union {
    u_int8_t  ether_dhost[ETH_ALEN];      /* destination eth addr */
    struct intshort ether_dest;
  };
  union {
    u_int8_t  ether_shost[ETH_ALEN];      /* source ether addr    */
    struct shortint ether_src;    
  };
  u_int16_t ether_type;                 /* packet type ID field */
} __attribute__ ((__packed__));

struct al_iphdr
  {
    unsigned char version_ihl;
    u_int8_t tos;
    u_int16_t tot_len;
    u_int16_t id;
    u_int16_t frag_off;
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t check;
    u_int16_t saddr_h; 
    u_int16_t saddr_l; 
    u_int16_t daddr_h; 
    u_int16_t daddr_l; 
    /*The options start here. */
  };

struct  arp_item{
  struct shortint arp;
  short pad;
  int ip;
};

struct  packed_arp_item{
  struct intshort arp;
  short pad;
  int ip;
};

struct routing_item {
  unsigned ip;
  unsigned mask;
  unsigned next_hop;
  unsigned port;
};


struct admin_header {
  char read;
  char reply;
  unsigned regnum;
  unsigned val;
  char padpack[64]; 
} __attribute__ ((__packed__));

void do_send(char* start_addr, char* end_addr, uint ctrl);
