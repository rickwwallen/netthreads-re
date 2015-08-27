#ifndef DEQUE_H
#define DEQUE_H

/*
A data structure similar to a C++ deque.  Implemented as a singly
linked-list that keeps track of the head and tail pointers. Allows
items to be pushed to either the head or tail, but only popped from
the head. Is not thread-safe. See mtdeque for a version that supports
multiple threads.
 */

#include "common.h"

/* Used by the deque to identify items. Add this as a member to any
   struct that you wish to add to the deque. This is similar to
   Linux's list implementation */
struct deque_item {
  struct deque_item *next;
};

struct deque {
  struct deque_item *head;
  struct deque_item *tail;
};

/* Initialize the deque. Must call this before any other function. */
static inline void deque_init(struct deque *deq) {
  deq->head = deq->tail = 0;
}

/* Check if the list is empty */
static inline int deque_empty(struct deque *deq) {
  return deq->head == 0;
}

/* Push an item at the head of the deque */
static inline void deque_push(struct deque *deq, struct deque_item *item) {
  struct deque_item* head = deq->head;

  item->next = head;
  if (head == 0) {
    //log("Push to empty head\n");
    deq->tail = item;
  } else {
    //    log("Push to non-empty head\n");
  }
  deq->head = item;
}

/* Push an item at the tail of the deque */
static inline void deque_push_tail(struct deque *deq, struct deque_item *item) {
  item->next = 0;
  if (deq->tail == 0) {
    //log("Push to empty tail\n");
    deq->head = deq->tail = item;
  } else {
    //log("Push to non-empty tail\n");
    deq->tail->next = item;
    deq->tail = item;
  }
}

/* Pop and return the head of the deque. Returns 0 if the deque is
   empty. */
static inline struct deque_item *deque_pop(struct deque *deq) {
  struct deque_item *head = deq->head;
  if (head) {
    deq->head = head->next;
    head->next = 0;
    if (deq->tail == head) {
      deq->tail = 0;
    }
  }
  return head;
}

/* Pops at most num items from the head of the deque. The first item
is written to *result. Use the item's next pointers to access the
rest. The actual number of items in the *result list is returned by
this function. */
int deque_pops(struct deque *deq, struct deque_item **result, int num);

/**
 * deque_entry - get the struct for this entry
 * @ptr:	the &struct deque_item pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define deque_entry(ptr, type, member) \
	container_of(ptr, type, member)

#endif
