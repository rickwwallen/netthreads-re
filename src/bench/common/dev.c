#include <string.h>

#include "dev.h"


void dev_init(struct net_iface *iface) {
  memset(iface, 0, sizeof(struct net_iface));
  arp_init(&iface->arp);
}
