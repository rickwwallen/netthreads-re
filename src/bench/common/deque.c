#include "deque.h"

int deque_pops(struct deque *deq, struct deque_item **result, int num) {
  if (deque_empty(deq)) {
    *result = 0;
    return 0;
  }
  int i;
  struct deque_item *item = deq->head;
  for (i = 1; item->next != 0 && i < num; i++) {
    log("consider item %d\n", i);
    item = item->next;
  }
  /* item is now the last item that will be returned. Cut the
     linked list after it. */

  *result = deq->head;
  deq->head = item->next;
  item->next = 0;
  if (deq->head == 0) {
    deq->tail = 0;
  }
  return i;
}
