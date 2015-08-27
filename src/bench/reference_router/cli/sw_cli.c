/* ****************************************************************************
 * $Id$
 *
 * Module: cli.c
 * Project: NetFPGA 2 Linux Kernel Driver
 * Description: Manage the NetFPGA router's ARP and IP tables.
 *
 * Change history:
 *
 *   Jan 23 2006: greg: lisip was not correctly displaying the port.
 *   Apr 7, 2007: jad modified for NF2.1 reference design
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/ethernet.h>
#include <assert.h>

#include <time.h>
#include <inttypes.h>

#include "reg_defines.h"
#include "nf2util.h"
#include "util.h"

#define PATHLEN         80

//#define DEFAULT_IFACE   "nf2c0"
#define DEFAULT_IFACE   "eth1"

/* Global vars */
static struct nf2device nf2;
static int verbose = 0;
static int force_cnet = 0;

/* Function declarations */
void prompt (void);
void help (void);
int  parse (char *);
void board (void);
void setip (void);
void setarp (void);
void listip (void);
void listarp (void);
void loadip (void);
void loadarp (void);
void clearip (void);
void cleararp (void);
void sendpacket_open_pf(const char *device);

uint8_t * parseip(char *str) {
        uint8_t *ret = (uint8_t *)malloc(4 * sizeof(uint8_t));
        char *num = (char *)strtok(str, ".");
        int index = 0;
        while (num != NULL) {
                ret[index++] = atoi(num);
                num = (char *)strtok(NULL, ".");
        }
        return ret;
}

uint8_t * parsemac(char *str) {
        uint8_t *ret = (uint8_t *)malloc(6 * sizeof(char));
        char *num = (char *)strtok(str, ":");
        int index = 0;
        while (num != NULL) {
                int i;
                sscanf(num, "%x", &i);
                ret[index++] = i;
                num = (char *)strtok(NULL, ":");
        }
        return ret;
}

int main(int argc, char *argv[])
{
  unsigned val;

  nf2.device_name = DEFAULT_IFACE;

  sendpacket_open_pf(nf2.device_name);
  
  prompt();


  return 0;
}

void prompt(void) {
  while (1) {
    printf("> ");
    char c[10], d[10], e[10], f[10];
    scanf("%s", c);
    int res = parse(c);
    switch (res) {
    case 0:
      listip();
      break;
    case 1:
      listarp();
      break;
    case 2:
      setip();
      break;
    case 3:
      setarp();
      break;
    case 4:
      loadip();
      break;
    case 5:
      loadarp();
      break;
    case 6:
      clearip();
      break;
    case 7:
      cleararp();
      break;
    case 8:
      help();
      break;
    case 9:
      return;
    default:
      printf("Unknown command, type 'help' for list of commands\n");
    }
  }
}

void help(void) {
  printf("Commands:\n");
  printf("  listip        - Lists entries in IP routing table\n");
  printf("  listarp       - Lists entries in the ARP table\n");
  printf("  setip         - Set an entry in the IP routing table\n");
  printf("  setarp        - Set an entry in the ARP table\n");
  printf("  loadip        - Load IP routing table entries from a file\n");
  printf("  loadarp       - Load ARP table entries from a file\n");
  printf("  clearip       - Clear an IP routing table entry\n");
  printf("  cleararp      - Clear an ARP table entry\n");
  printf("  setq          - Set Queue limits\n");
  printf("  setpkts       - Set Packet limits (max packets per queue)\n");
  printf("  help          - Displays this list\n");
  printf("  quit          - Exit this program\n");
}

struct admin_header {
  char read;
  char reply;
  unsigned regnum;
  unsigned val;
  char padpack[64];
} __attribute__ ((__packed__));

#define ADMIN_ETHER_TYPE    0x7fff 

int writeReg( struct nf2device *nf2, unsigned addr, unsigned val)
{
  int len = sizeof(struct ether_header) + sizeof(struct admin_header);
  char buffer[len];
  memset(buffer, 0, len);

  struct ether_header* eh = (struct ether_header*)buffer;
  eh->ether_type = ntohs(ADMIN_ETHER_TYPE);
  eh->ether_shost[0] = 0x00;
  eh->ether_shost[1] = 0x1c;
  eh->ether_shost[2] = 0x23;
  eh->ether_shost[3] = 0xdc;
  eh->ether_shost[4] = 0xa5;
  eh->ether_shost[5] = 0xc4;

  eh->ether_dhost[0] = 0xff;
  eh->ether_dhost[1] = 0xff;
  eh->ether_dhost[2] = 0xff;
  eh->ether_dhost[3] = 0xff;
  eh->ether_dhost[4] = 0xff;
  eh->ether_dhost[5] = 0xff;


  struct admin_header* ah = (struct admin_header*)(&(buffer[sizeof(struct ether_header)]));  
  ah->regnum = ntohl(addr);
  ah->val = ntohl(val);
  ah->read = 0;
  ah->reply = 0;
  //printf("writing %x to reg %x\n", val, addr);
  int r = sendpacket(buffer, len);
  assert(r == len);
  return r;
}

int readReg( struct nf2device *nf2, unsigned addr, unsigned *val)
{
  int len = sizeof(struct ether_header) + sizeof(struct admin_header);
  char buffer[len*3];
  memset(buffer, 0, len);

  struct ether_header*eh  = (struct ether_header*)buffer;
  eh->ether_type = ntohs(ADMIN_ETHER_TYPE);
  eh->ether_shost[0] = 0x00;
  eh->ether_shost[1] = 0x1c;
  eh->ether_shost[2] = 0x23;
  eh->ether_shost[3] = 0xdc;
  eh->ether_shost[4] = 0xa5;
  eh->ether_shost[5] = 0xc4;

  eh->ether_dhost[0] = 0xff;
  eh->ether_dhost[1] = 0xff;
  eh->ether_dhost[2] = 0xff;
  eh->ether_dhost[3] = 0xff;
  eh->ether_dhost[4] = 0xff;
  eh->ether_dhost[5] = 0xff;

  struct admin_header*ah  = (struct admin_header*)(&(buffer[sizeof(struct ether_header)]));  
  ah->regnum = ntohl(addr);
  ah->val = 0;
  ah->read = 1;
  ah->reply = 0;

  int r = sendpacket(buffer, len);
  //printf("sent read packet for reg %x\n", addr);

  r = get_packet(buffer, len*3);
  //printf("recv read packet for reg %x, length %d, expected %d\n", addr, r, len);

/*
  //print received packet
  int i;
  int k = r/2;
  short* pbuf = (short*)buffer;
  for(i=0; i<k; i++)
    {
      int nb = i+8;
      if(nb > k) nb = k;

      for(; i<nb; i++)
	fprintf(stdout, "%04x ", ntohs(pbuf[i]));
      printf("\n");
    }
*/

  //assert(r == len);

  *val = ntohl(ah->val);
  return r;
}

void addarp(int entry, uint8_t *ip, uint8_t *mac) {
  writeReg(&nf2, ROUTER_OP_LUT_ARP_NEXT_HOP_IP_REG, ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3]);
  writeReg(&nf2, ROUTER_OP_LUT_ARP_MAC_HI_REG, mac[0] << 8 | mac[1]);
  writeReg(&nf2, ROUTER_OP_LUT_ARP_MAC_LO_REG, mac[2] << 24 | mac[3] << 16 | mac[4] << 8 | mac[5]);
  writeReg(&nf2, ROUTER_OP_LUT_ARP_LUT_WR_ADDR_REG, entry);
}

void addip(int entry, uint8_t *subnet, uint8_t *mask, uint8_t *nexthop, int port) {
  writeReg(&nf2, ROUTER_OP_LUT_RT_IP_REG, subnet[0] << 24 | subnet[1] << 16 | subnet[2] << 8 | subnet[3]);
  writeReg(&nf2, ROUTER_OP_LUT_RT_MASK_REG, mask[0] << 24 | mask[1] << 16 | mask[2] << 8 | mask[3]);
  writeReg(&nf2, ROUTER_OP_LUT_RT_NEXT_HOP_IP_REG, nexthop[0] << 24 | nexthop[1] << 16 | nexthop[2] << 8 | nexthop[3]);
  writeReg(&nf2, ROUTER_OP_LUT_RT_OUTPUT_PORT_REG, port);
  writeReg(&nf2, ROUTER_OP_LUT_RT_LUT_WR_ADDR_REG, entry);
}

void setip(void) {
  printf("Enter [entry] [subnet]      [mask]       [nexthop] [port]:\n");
  printf("e.g.     0   192.168.1.0  255.255.255.0  15.1.3.1     4:\n");
  printf(">> ");

  char subnet[15], mask[15], nexthop[15];
  int port, entry;
  scanf("%i %s %s %s %x", &entry, subnet, mask, nexthop, &port);

  if ((entry < 0) || (entry > 15)) {
    printf("Entry must be between 0 and 15. Aborting\n");
    return;
  }

  if ((port < 1) || (port > 255)) {
    printf("Port must be between 1 and ff.  Aborting\n");
    return;
  }

  uint8_t *sn = parseip(subnet);
  uint8_t *m = parseip(mask);
  uint8_t *nh = parseip(nexthop);

  addip(entry, sn, m, nh, port);
}

void setarp(void) {
  printf("Enter [entry] [ip] [mac]:\n");
  printf(">> ");

  char nexthop[15], mac[30];
  int entry;
  scanf("%i %s %s", &entry, nexthop, mac);

  if ((entry < 0) || (entry > 15)) {
    printf("Entry must be between 0 and 15. Aborting\n");
    return;
  }

  uint8_t *nh = parseip(nexthop);
  uint8_t *m = parsemac(mac);

  addarp(entry, nh, m);
}



void listip(void) {
  int i;
  for (i = 0; i < 16; i++) {
    unsigned subnet, mask, nh, valport;

    writeReg(&nf2, ROUTER_OP_LUT_RT_LUT_RD_ADDR_REG, i);

    readReg(&nf2, ROUTER_OP_LUT_RT_IP_REG, &subnet);

    readReg(&nf2,ROUTER_OP_LUT_RT_MASK_REG , &mask);

    readReg(&nf2,ROUTER_OP_LUT_RT_NEXT_HOP_IP_REG , &nh);

    readReg(&nf2,ROUTER_OP_LUT_RT_OUTPUT_PORT_REG , &valport);

    printf("Entry #%i:   ", i);
    int port = valport & 0xff;
    if (subnet!=0 || mask!=0xffffffff || port!=0) {
      printf("Subnet: %i.%i.%i.%i, ", subnet >> 24, (subnet >> 16) & 0xff, (subnet >> 8) & 0xff, subnet & 0xff);
      printf("Mask: 0x%x, ", mask);
      printf("Next Hop: %i.%i.%i.%i, ", nh >> 24, (nh >> 16) & 0xff, (nh >> 8) & 0xff, nh & 0xff);
      printf("Port: 0x%02x\n", port);
    }
    else {
      printf("--Invalid--\n");
    }
  }
}

void listarp(void) {
  int i = 0;
  for (i = 0; i < 16; i++) {
    unsigned ip, machi, maclo;

    writeReg(&nf2, ROUTER_OP_LUT_ARP_LUT_RD_ADDR_REG, i);

    readReg(&nf2, ROUTER_OP_LUT_ARP_NEXT_HOP_IP_REG, &ip);

    readReg(&nf2, ROUTER_OP_LUT_ARP_MAC_HI_REG, &machi);

    readReg(&nf2, ROUTER_OP_LUT_ARP_MAC_LO_REG, &maclo);

    printf("Entry #%i:   ", i);
    if (ip!=0) {
      printf("IP: %i.%i.%i.%i, ", ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
      printf("MAC: %x:%x:%x:%x:%x:%x\n", (machi >> 8) & 0xff, machi & 0xff, 
              (maclo >> 24) & 0xff, (maclo >> 16) & 0xff,
              (maclo >> 8) & 0xff, (maclo) & 0xff);
    }
    else {
      printf("--Invalid--\n");
    }
  }
}

void loadip(void) {
  char fn[30];
  printf("Enter filename:\n");
  printf(">> ");
  scanf("%s", fn);

  FILE *fp;
  char subnet[20], mask[20], nexthop[20];
  int entry, port;
  if((fp = fopen(fn, "r")) ==NULL) {
    printf("Error: cannot open file %s.\n", fn);
    return;
  }
  while (fscanf(fp, "%i %s %s %s %x", &entry, subnet, mask, nexthop, &port) != EOF) {
    uint8_t *sn = parseip(subnet);
    uint8_t *m = parseip(mask);
    uint8_t *nh = parseip(nexthop);

    addip(entry, sn, m, nh, port);
  }
}

void loadarp(void) {
  char fn[30];
  printf("Enter filename:\n");
  printf(">> ");
  scanf("%s", fn);

  FILE *fp = fopen(fn, "r");
  char ip[20], mac[20];
  int entry;
  while (fscanf(fp, "%i %s %s", &entry, ip, mac) != EOF) {
    uint8_t *i = parseip(ip);
    uint8_t *m = parsemac(mac);

    addarp(entry, i, m);
  }
}

void clearip(void) {
  int entry;
  printf("Specify entry:\n");
  printf(">> ");
  scanf("%i", &entry);

  writeReg(&nf2, ROUTER_OP_LUT_RT_IP_REG, 0);
  writeReg(&nf2, ROUTER_OP_LUT_RT_MASK_REG, 0xffffffff);
  writeReg(&nf2, ROUTER_OP_LUT_RT_NEXT_HOP_IP_REG, 0);
  writeReg(&nf2, ROUTER_OP_LUT_RT_OUTPUT_PORT_REG, 0);
  writeReg(&nf2, ROUTER_OP_LUT_RT_LUT_WR_ADDR_REG, entry);
}       

void cleararp(void) {
  int entry;
  printf("Specify entry:\n");
  printf(">> ");
  scanf("%i", &entry);

  writeReg(&nf2, ROUTER_OP_LUT_ARP_NEXT_HOP_IP_REG,0);
  writeReg(&nf2, ROUTER_OP_LUT_ARP_MAC_HI_REG,0);
  writeReg(&nf2, ROUTER_OP_LUT_ARP_MAC_LO_REG,0);
  writeReg(&nf2, ROUTER_OP_LUT_ARP_LUT_WR_ADDR_REG, entry);
}       


int parse(char *word) {
  if (!strcmp(word, "listip"))
    return 0;
  if (!strcmp(word, "listarp"))
    return 1;
  if (!strcmp(word, "setip"))
    return 2;
  if (!strcmp(word, "setarp"))
    return 3;
  if (!strcmp(word, "loadip"))
    return 4;
  if (!strcmp(word, "loadarp"))
    return 5;
  if (!strcmp(word, "clearip"))
    return 6;
  if (!strcmp(word, "cleararp"))
    return 7;
  if (!strcmp(word, "help"))
    return 8;
  if (!strcmp(word, "quit"))
    return 9;
  return -1;
}
