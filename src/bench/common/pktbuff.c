#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pktbuff.h"
#include "support.h"

void pkt_fill(struct pkt_buff *pkt, char *data, uint size) {
  pkt->len = pkt->total_size = size;
  pkt->data = pkt->head = data;
}

char*pkt_alloc(struct pkt_buff *pkt, uint size)
{
  t_addr* ptr = nf_pktout_alloc(size);
  pkt_fill(pkt, ptr, size);
  return pkt->data;
}

void pkt_free(struct pkt_buff *pkt)
{
#ifdef DEBUG
  //  memset(pkt->data, 0xFF, pkt->total_size);
  //  free(pkt->data);
  //  memset(pkt, 0xFF, sizeof(struct pkt_buff));
  //  free(pkt);
#else
  nf_pktout_free(pkt->data);
  pkt->data = pkt->head = 0;
#endif
}

void fill_ioq(struct ioq_header *ioq, unsigned short port, unsigned short bytes) {
  // return to sender
  ioq->src_port = htons(port); // probably don't need to set source port
  ioq->dst_port = htons(1 << port);

  ioq->byte_length = htons(bytes);
  //log("setting ioq byte length of packet to %d\n", bytes);

  ioq->word_length = bytes / 8;
  ioq->word_length += ((ioq->word_length * 8) != bytes); // round up
  ioq->word_length = htons(ioq->word_length);
}
