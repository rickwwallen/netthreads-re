#include <stdlib.h>

#include "timer.h"
#include "support.h"

struct TimerEntry {
  unsigned int time;
  timer_func function;
  void *value;
	
  struct TimerEntry *next;
};

void timer_init(struct Timer *timer) {
  timer->numTasks = 0;
  timer->scheduledEntries = 0;
}

void timer_destroy(struct Timer *timer) {
  
}

void timer_schedule(struct Timer *timer, unsigned int time, timer_func function, void *value) {
  // grab an unused TimerEntry
  struct TimerEntry *entry = sp_malloc(sizeof(struct TimerEntry));
  entry->next = 0;
  entry->time = time;
  entry->function = function;
  entry->value = value;
	
  // find where to insert into the scheduled list
  struct TimerEntry **insertAt = &(timer->scheduledEntries);
  while (*insertAt != 0 && (*insertAt)->time <= time) {
    insertAt = &((*insertAt)->next);
  }
  // insert into scheduled list
  entry->next = *insertAt;
  *insertAt = entry;

  timer->numTasks++;
}

void timer_fire_expired(struct Timer *timer) {
  unsigned int time = nf_time();
  while (timer->scheduledEntries != 0 && timer->scheduledEntries->time <= time) {
    // pull entry off the scheduled list
    struct TimerEntry *entry = timer->scheduledEntries;
    timer->scheduledEntries = entry->next;

    timer->numTasks--;
    entry->function(entry->value);

    sp_free(entry);
  }
}

unsigned int timer_next_time(struct Timer *timer) {
  if (timer->scheduledEntries) {
    return timer->scheduledEntries->time;
  }
  return 0;
}
	
/*
#include <stdio.h>

static struct Timer timer;

void hook(void *p) {
  printf("hook called\n");
  int i = (int)p;
  i--;
  if (i > 0) 
    timer_schedule(&timer, timer.time + i, hook, i);
  
}

int main() {

  timer_init(&timer);

  timer_schedule(&timer, timer.time + 4, hook, 10);
  while (timer.numTasks) {
    printf("tick %u\n", timer.time);
    timer_tick(&timer);
  }

  printf("Finshed.\n");
  return 0;
}
*/
