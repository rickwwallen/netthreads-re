#ifndef ARP_H
#define ARP_H


#define ARP_TABLE_SIZE 10

struct arp_entry {
  unsigned char mac[6];
  unsigned char ip[4];
};

struct arp_table {
  struct arp_entry entries[ARP_TABLE_SIZE];
  int next_entry;
};

void arp_init(struct arp_table *arp);
struct arp_entry * arp_add(struct arp_table *arp);
struct arp_entry *arp_lookup(struct arp_table *arp, unsigned char *ip);

#endif
