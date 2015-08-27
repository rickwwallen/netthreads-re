#include "workqueue.h"

#include "support.h"

void workq_init(struct work_queue *queue, int lock1, int lock2) {
  mtdeque_init(&queue->tasks, lock1, lock2);
  queue->workers_waiting = 0;
}

void workq_serve(struct work_queue *queue, int num_tasks) {
  while (num_tasks) {
    struct deque_item *item = mtdeque_pop(&queue->tasks, 1);
    struct work_task *task = deque_entry(item, struct work_task, item);
    
    /* Do not touch task after calling work, the work function may
       have reused it */
    //log("Serving task %p\n", task);
    //    log("Serving task %p to work %p with data %lu\n", task, task->work, data);
    task->work(task->data);

    if (num_tasks > 0) {
      num_tasks--;
    }
  }
}

void workq_add_task(struct work_queue *queue, struct work_task *task, work_t work, unsigned long data) {
  //log("Enqueue task %p to work %p with data %lu\n", task, work, data);
  task->work = work;
  task->data = data;
  mtdeque_push_tail(&queue->tasks, &task->item);
}

void workq_add_task_nolock(struct work_queue *queue, struct work_task *task, work_t work, unsigned long data) {
  //log("Enqueue task %p to work %p with data %lu\n", task, work, data);
  task->work = work;
  task->data = data;
  mtdeque_push_tail_nolock(&queue->tasks, &task->item);
}
