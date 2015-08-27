#ifndef TIMER_H
#define TIMER_H


typedef void (*timer_func)(void*);

struct TimerEntry;

struct Timer {
  //  unsigned int time;

  int numTasks;

  struct TimerEntry *scheduledEntries;
};


void timer_init(struct Timer *timer);
void timer_destroy(struct Timer *timer);

void timer_fire_expired(struct Timer *timer);

// Moves time forward 1 unit. Any timers that are scheduled for the
// new time are notified
//void timer_tick(struct Timer *timer);
// Moves time forward to a new time. Any timers that are scheduled for the
// new time or earlier are notified
//void timer_set(struct Timer *timer, unsigned int time);

// Returns the time that the next event will fire at
unsigned int timer_next_time(struct Timer *timer);

// Schedules a funtion to be called at a given time with a given value
void timer_schedule(struct Timer *timer, unsigned int time, timer_func function, void *value);

#endif
