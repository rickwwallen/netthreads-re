#include "mtdeque.h"

void mtdeque_init(struct mtdeque *mtdeq, int lock1, int lock2) {
  deque_init(&mtdeq->deq);

  mtdeq->size = 0;
  mtdeq->threads_waiting = 0;
      
  mtdeq->state_lock = lock1;
  mtdeq->wait_lock = lock2;
          
  nf_lock(mtdeq->wait_lock);

#ifndef CONTEXT_NF
  mtdeq->state_lock_thread = -1;
#endif
}


struct deque_item *mtdeque_pop_nolock(struct mtdeque *mtdeq, int can_block) {
  LOCK_CHECK(mtdeq);
  while (1) {
    int size = mtdeq->size;
    if (size) {
      struct deque_item *item = deque_pop(&mtdeq->deq);
      mtdeq->size--;
      if (size != 1 && mtdeq->threads_waiting) {
        /* There are still items on stack. Wake any waiting threads. */
        nf_unlock(mtdeq->wait_lock);
      }
      return item;
    } else if (can_block) {
#ifndef CONTEXT_NF
  if (mtdeq->debug) {
    log("t%d: mtdeque_pop blocking\n", nf_tid());
  }
#endif

      /* Drop the state_lock and sleep  */
      mtdeq->threads_waiting++;
      mtdeque_unlock(mtdeq);
      /* WARNING: There is a potential race condition here.  If
         another thread pushes something onto the queue, and unlocks
         mtdeq->wait_lock before this thread can lock mtdeq->wait_lock
         to sleep then this thread will not be awoken. 

         However, the threads scheduling is round robin so other
         threads shouldn't be able to execute a lot of instructions
         before this thread can lock mtdeq->wait_lock below.

         Is there an absolutely safe way to do this? Need an atomic
         way to unlock one lock and lock another.
       */
      nf_lock(mtdeq->wait_lock);
      mtdeque_lock(mtdeq);
      mtdeq->threads_waiting--;
    } else {
      return 0;
    }
  }

  /*struct deque_item *result = 0;
  mtdeque_pops_nolock(mtdeq, &result, 1, can_block);
  return result;*/
}

int mtdeque_pops_nolock(struct mtdeque *mtdeq, struct deque_item **result, int num, int can_block) {
  LOCK_CHECK(mtdeq);
  while (1) {
    int size = mtdeq->size;
    if (size) {
      if (num >= size) {
        /* Return entire contents of stack */
        *result = mtdeq->deq.head;
        /* clear the deque */
        deque_init(&mtdeq->deq);
        num = size;
        mtdeq->size = 0;
        return num;
      } else {
        /* Return num elements from stack */
        num = deque_pops(&mtdeq->deq, result, num);
        mtdeq->size -= num;
        
        /* There are still items on stack. Wake any waiting threads. */
        if (mtdeq->threads_waiting) {
          nf_unlock(mtdeq->wait_lock);
        }

        return num;
      }      
    } else if (can_block) {
#ifndef CONTEXT_NF
  if (mtdeq->debug) {
    log("t%d: mtdeque_pops blocking\n", nf_tid());
  }
#endif
      /* Drop the state_lock and sleep  */
      mtdeq->threads_waiting++;
      mtdeque_unlock(mtdeq);
      /* WARNING: There is a potential race condition here.  If
         another thread pushes something onto the queue, and unlocks
         mtdeq->wait_lock before this thread can lock mtdeq->wait_lock
         to sleep then this thread will not be awoken. 

         However, the threads scheduling is round robin so other
         threads shouldn't be able to execute a lot of instructions
         before this thread can lock mtdeq->wait_lock below.

         Is there an absolutely safe way to do this? Need an atomic
         way to unlock one lock and lock another.
       */
      nf_lock(mtdeq->wait_lock);
      mtdeque_lock(mtdeq);
      mtdeq->threads_waiting--;
    } else {
      return 0;
    }
  }
}
