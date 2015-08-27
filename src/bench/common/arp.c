#include <string.h>

#include "arp.h"

void arp_init(struct arp_table *arp) {
  memset(arp, 0, sizeof(struct arp_table));
}

struct arp_entry *arp_add(struct arp_table *arp) {
  struct arp_entry *entry = arp->entries + arp->next_entry;
  arp->next_entry++;
  arp->next_entry %= ARP_TABLE_SIZE;
  return entry;
}

struct arp_entry *arp_lookup(struct arp_table *arp, unsigned char *ip) {
  int i;
  for (i = 0; i < ARP_TABLE_SIZE; i++) {
    if (memcmp(&arp->entries[i].ip, ip, 4) == 0) {
      return &arp->entries[i];
    }
  }
  return 0;
}
