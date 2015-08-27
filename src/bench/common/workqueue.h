#ifndef WORKQUEUE_H
#define WORKQUEUE_H

/*
  A workqueue allows multiple threads to be assigned tasks. Tasks are
  added and served in FIFO order. Each task is defined by a function
  pointer and unsigned long value that will be passed to the function.
 */

#include "mtdeque.h"

typedef void (*work_t)(unsigned long);

struct work_task {
  work_t work;
  unsigned long data;
  struct deque_item item;
};

struct work_queue {
  int workers_waiting;
  struct mtdeque tasks;
};

/* Initialize a workqueue. Must be called before any other
   function. lock1 and lock2 are locks that the the workqueue will use
   to control access to state and to sleep threads waiting to serve
   tasks. These locks should not be locked or unlocked anywhere else
   in the program while the workqueue is in use. */
void workq_init(struct work_queue *queue, int lock1, int lock2);


/* The current thread will execute num_tasks tasks from the queue
   before returning from this function. This function blocks if there
   are not enough tasks remaining. If num_tasks is negative, the
   thread will execute tasks forever and will never return.  */
void workq_serve(struct work_queue *queue, int num_tasks);

/* Add a task to the queue. The work_task struct must be allocated and
   provided by the caller */
void workq_add_task(struct work_queue *queue, struct work_task *task, work_t work, unsigned long data);

/* Same as workq_add_task, but this function assumes the current
   thread has called workq_lock already. This is more efficient if you
   are adding multiple tasks. */
void workq_add_task_nolock(struct work_queue *queue, 
                struct work_task *task, 
                work_t work, 
                unsigned long data);

/* Explicitly lock the state lock of the workqueue. Use with
   workq_add_task_nolock to add multiple tasks. Must call workq_unlock
   after finished. */
static inline void workq_lock(struct work_queue *queue) {
  mtdeque_lock(&queue->tasks);
}

static inline void workq_unlock(struct work_queue *queue) {
  mtdeque_unlock(&queue->tasks);
}

#endif
