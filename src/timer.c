#include "timer.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static unsigned int secs = 0;
static time_t target;

static unsigned int timeup_calls;

#define MOVE_WIGGLE_ROOM 2
#define SECS_PER_MIN 60

#define TIMEUP_CALLS_PER_CHECK 10000

void timer_init_xboard(char* level) {
  unsigned int moves, base, inc;
  int ret = sscanf(level, "level %u %u %u", &moves, &base, &inc);

  if (ret == 3) {
    if (moves == 0 || base == 0)
      secs = inc;
    else
      secs = inc + (base * SECS_PER_MIN) / (moves - MOVE_WIGGLE_ROOM);
  } else {
    secs = 5;
  }
}

void timer_init_secs(unsigned int n) {
  secs = n;
}

void timer_begin(void) {
  if (secs < 1)
    timer_init_secs(5);

  target = timer_get_centiseconds() + secs * 100;
  timeup_calls = 0;
}

uint8_t timer_timeup(void) {
  if (timeup_calls++ < TIMEUP_CALLS_PER_CHECK)
    return 0;

  timeup_calls = 0;
  if (timer_get_centiseconds() > target)
    return 1;
  else
    return 0;
}

void timer_end(void) {
  // Do nothing for now.
}

time_t timer_get_centiseconds(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 100 + t.tv_nsec / 10000000;
}
