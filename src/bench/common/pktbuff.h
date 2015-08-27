#ifndef PKTBUFF_H
#define PKTBUFF_H

#include <stdio.h>

#include "common.h"

struct pkt_buff
{
  char *data;      // always points to start of memory. doesn't change
 char *head;      // current position within packet. always >= data
  unsigned int	len;        // len of packet remaining after head.
  unsigned int  total_size; // total size of packet. doesn't change
};

struct ioq_header {
  unsigned short dst_port;
  unsigned short word_length;
  unsigned short src_port;
  unsigned short byte_length;
};


void pkt_fill(struct pkt_buff *pkt, char *data, uint size);
char*pkt_alloc(struct pkt_buff *pkt, uint size);
void pkt_free(struct pkt_buff *pkt);

static inline char*pkt_push(struct pkt_buff *pkt, unsigned int size)
{
  if (pkt->head < (pkt->data + size)) {
			log("%s: head: %p, data: %p, size: %u [%u], req_size: %u.\n",
				__func__, pkt->head, pkt->data, pkt->len, pkt->total_size, size);
		return NULL;
	}
	pkt->head -= size;
	pkt->len += size;
	return pkt->head;
}

#define pkt_pull_type(pkt, type) (type*) pkt_pull(pkt, sizeof(type))

static inline char *pkt_pull(struct pkt_buff *pkt, unsigned int size)
{
char *head = pkt->head;
	if (pkt->len < size) {
		log("%s: head: %p, data: %p, size: %u [%u], req_size: %u.\n",
				__func__, pkt->head, pkt->data, pkt->len, pkt->total_size, size);
		return NULL;
	}
	pkt->head += size;
	pkt->len -= size;
	return head;
}

static inline char *pkt_trim(struct pkt_buff *pkt, unsigned int size)
{
	if (size > pkt->len) {
		log("%s: head: %p, data: %p, size: %u, req_size: %u.\n",
				__func__, pkt->head, pkt->data, pkt->len, size);
		return NULL;
	}
	pkt->len = size;
	return pkt->head;
}

static inline char *pkt_push_all(struct pkt_buff *pkt)
{
  return pkt_push(pkt, pkt->head - pkt->data);
}

void fill_ioq(struct ioq_header *ioq, unsigned short port, unsigned short bytes);
#endif
