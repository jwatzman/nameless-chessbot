#include "timer.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static unsigned int secs_per_move = 0;
static time_t target_cs;

static unsigned int timeup_calls;

#define MOVE_WIGGLE_ROOM 2
#define SECS_PER_MIN 60

#define TIMEUP_CALLS_PER_CHECK 10000

void timer_init_xboard(char* level) {
  unsigned int moves, base_m, inc;
  unsigned int base_s = 0;

  int ret = sscanf(level, "level %u %u %u", &moves, &base_m, &inc);
  if (ret != 3) {
    int ret =
        sscanf(level, "level %u %u:%u %u", &moves, &base_m, &base_s, &inc);

    if (ret != 4) {
      secs_per_move = 5;
      return;
    }
  }

  base_s += SECS_PER_MIN * base_m;

  if (moves == 0 || base_s == 0)
    secs_per_move = inc;
  else
    secs_per_move = inc + base_s / (moves - MOVE_WIGGLE_ROOM);
}

void timer_init_secs(unsigned int n) {
  secs_per_move = n;
}

void timer_begin(void) {
  if (secs_per_move < 1)
    timer_init_secs(5);

  target_cs = timer_get_centiseconds() + secs_per_move * 100;
  timeup_calls = 0;
}

uint8_t timer_timeup(void) {
  if (timeup_calls++ < TIMEUP_CALLS_PER_CHECK)
    return 0;

  timeup_calls = 0;
  if (timer_get_centiseconds() >= target_cs)
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
