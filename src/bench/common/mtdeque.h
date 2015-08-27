#ifndef MTDEQUE_H
#define MTDEQUE_H

/*
A data structure exactly like deque.h but adds thread safety. Multiple
threads can push items onto the mtdeque and other threads can pop
items and can optionally block until items are pushed.
 */

#include "support.h"
#include "deque.h"

#define CAN_BLOCK 1
#define NO_BLOCK 0

struct mtdeque {
  struct deque deq;
  
  volatile int size;
  volatile int threads_waiting;

  int state_lock;
  int wait_lock;

#ifndef CONTEXT_NF
  int state_lock_thread;
  int debug;
#endif
};

/* Initialize the mtdeque. Must call this before other
   functions. lock1 and lock2 are locks that the the workqueue will
   use to control access to state and to sleep threads waiting to
   serve tasks. These locks should not be locked or unlocked anywhere
   else in the program while the mtdeque is in use. */
void mtdeque_init(struct mtdeque *mtdeq, int lock1, int lock2);

/* Lock the state lock of the mtdeque. Only call this if you want to
   use the "nolock" versions of push or pop */
static inline void mtdeque_lock(struct mtdeque *mtdeq) {
#ifndef CONTEXT_NF
  if (mtdeq->state_lock_thread == nf_tid()) {
    log("ERROR: Thread %d locked mtdeque twice!\n", nf_tid());
  }
#endif
  nf_lock(mtdeq->state_lock);
#ifndef CONTEXT_NF
  mtdeq->state_lock_thread = nf_tid();
#endif
}

/* Unlock the state lock of the mtdeque. Call this after calling
   mtdeque_lock */
static inline void mtdeque_unlock(struct mtdeque *mtdeq) {
#ifndef CONTEXT_NF
  if (mtdeq->state_lock_thread != nf_tid()) {
    log("ERROR: Thread %d unlocking mtdeque owned by thread %d!\n", 
        nf_tid(), mtdeq->state_lock_thread);
  }
  mtdeq->state_lock_thread = -1;
#endif
  nf_unlock(mtdeq->state_lock);
}

/* Checks if the mtdeque is empty */
static inline int mtdeque_empty(struct mtdeque *mtdeq) {
  return deque_empty(&mtdeq->deq);
}

/* Debugging code */
#ifdef CONTEXT_NF
#define LOCK_CHECK(mtdeq) ;
#else
#define LOCK_CHECK(mtdeq)                                               \
  if (mtdeq->state_lock_thread != nf_tid()) {                           \
    log("ERROR: Thread %d called mtdeque_push_nolock without lock: %d!\n", \
        nf_tid(), mtdeq->state_lock_thread);                            \
  }
#endif

/* Push an item at the head of the mtdeque without any locking. Alone,
   this is not thread-safe. */
static inline void mtdeque_push_nolock(struct mtdeque *mtdeq, struct deque_item *elem) {
  LOCK_CHECK(mtdeq);
  deque_push(&mtdeq->deq, elem);
  mtdeq->size++;
  if (mtdeq->size == 1 && mtdeq->threads_waiting) {
#ifndef CONTEXT_NF
  if (mtdeq->debug) {
    log("t%d: mtdeque_push waking up other thread\n", nf_tid());
  }
#endif
    /* wake any thread who might be waiting to pop */
    nf_unlock(mtdeq->wait_lock);
  }
}

/* Push an item at the head of the mtdeque. This is thread-safe. */
static inline void mtdeque_push(struct mtdeque *mtdeq, struct deque_item *elem) {
  mtdeque_lock(mtdeq);
  mtdeque_push_nolock(mtdeq, elem);
  mtdeque_unlock(mtdeq);
}

/* Push an item at the tail of the mtdeque without any locking. Alone,
   this is not thread-safe. */
static inline void mtdeque_push_tail_nolock(struct mtdeque *mtdeq, struct deque_item *elem) {
  LOCK_CHECK(mtdeq);
  deque_push_tail(&mtdeq->deq, elem);
  mtdeq->size++;
  if (mtdeq->size == 1 && mtdeq->threads_waiting) {
#ifndef CONTEXT_NF
  if (mtdeq->debug) {
    log("t%d: mtdeque_push_tail waking up other thread\n", nf_tid());
  }
#endif
    /* wake any thread who might be waiting to pop */
    nf_unlock(mtdeq->wait_lock);
  }
}

/* Push an item at the tail of the mtdeque. This is thread-safe. */
static inline void mtdeque_push_tail(struct mtdeque *mtdeq, struct deque_item *elem) {
  mtdeque_lock(mtdeq);
  mtdeque_push_tail_nolock(mtdeq, elem);
  mtdeque_unlock(mtdeq);
}

/* Like mtdeque_pop but does no locking */
struct deque_item *mtdeque_pop_nolock(struct mtdeque *mtdeq, int can_block);

/* Like mtdeque_pops but does no locking */
int mtdeque_pops_nolock(struct mtdeque *mtdeq, struct deque_item **result, int num, int can_block);


/* Pop an item from the head of the mtdeque. If the mtdeque is empty
   and can_block == CAN_BLOCK then the function will block until an
   item can be returned. Otherwise, if the mtqueue is empty this
   function returns 0. This is thread-safe. */
static inline struct deque_item *mtdeque_pop(struct mtdeque *mtdeq, int can_block) {
  mtdeque_lock(mtdeq);
  struct deque_item *result = mtdeque_pop_nolock(mtdeq, can_block);
  mtdeque_unlock(mtdeq);
  return result;
}

/* Pops at most num items from the head of the mtdeque. The first item
is written to *result. Use the item's next pointers to access the
rest. The actual number of items in the *result list is returned by
this function. If can_block == CAN_BLOCK then this function will block
until at least one item can be popped. This is thread-safe*/
static inline int mtdeque_pops(struct mtdeque *mtdeq, struct deque_item **result, int num, int can_block) {
  mtdeque_lock(mtdeq);
  num = mtdeque_pops_nolock(mtdeq, result, num, can_block);
  mtdeque_unlock(mtdeq);
  return num;
}

#endif
