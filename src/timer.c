#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "timer.h"

static time_t start_cs;
static time_t target_cs;
static time_t hard_stop_cs;

static unsigned int moves;
static time_t base_cs;
static time_t inc_cs;

static unsigned int remaining_moves;  // Moves until next time control.
static time_t remaining_cs;           // How much time left on our clock.
static unsigned int opening;

static unsigned int timeup_calls;

#define REMAINING_MOVE_WIGGLE_ROOM 2
#define WIGGLE_ROOM_CS_PER_MOVE 1

#define SECS_PER_MIN 60
#define CS_PER_SEC 100

#define MOVES_IN_OPENING 3
#define MOVES_ASSUMED_INCREMENTAL 50

#define TIMEUP_CALLS_PER_CHECK 10000

void timer_init_xboard(char* level) {
  unsigned int base_m;
  unsigned int base_s = 0;
  float inc;

  int ret = sscanf(level, "level %u %u %f", &moves, &base_m, &inc);
  if (ret != 3) {
    int ret =
        sscanf(level, "level %u %u:%u %f", &moves, &base_m, &base_s, &inc);

    if (ret != 4) {
      timer_init_secs(5);
      return;
    }
  }

  base_cs = (base_m * SECS_PER_MIN + base_s) * CS_PER_SEC;
  inc_cs = inc * CS_PER_SEC;
  remaining_moves = moves;
  remaining_cs = base_cs;
  opening = MOVES_IN_OPENING;
}

void timer_init_secs(unsigned int n) {
  moves = 0;
  base_cs = 0;
  inc_cs = n * CS_PER_SEC;
  remaining_moves = moves;
  remaining_cs = base_cs;
  opening = MOVES_IN_OPENING;
}

void timer_begin(void) {
  assert(base_cs > 0 || inc_cs > 0);
  start_cs = timer_get_centiseconds();

  // Do not ever use more than 2/3 of our remaining time.
  hard_stop_cs = start_cs + inc_cs + (2 * remaining_cs / 3);

  time_t target_usage_cs;
  if (moves > 0) {
    assert(remaining_moves > 0);
    target_usage_cs =
        remaining_cs / (remaining_moves + REMAINING_MOVE_WIGGLE_ROOM);
  } else {
    target_usage_cs = remaining_cs / MOVES_ASSUMED_INCREMENTAL;
  }
  if (opening)
    target_usage_cs /= 2;
  target_cs = start_cs + inc_cs + target_usage_cs;
}

uint8_t timer_timeup(void) {
  if (timeup_calls++ < TIMEUP_CALLS_PER_CHECK)
    return 0;

  timeup_calls = 0;

  time_t now = timer_get_centiseconds();
  if (now >= target_cs)
    return 1;
  else if (now >= hard_stop_cs)
    return 1;
  else
    return 0;
}

uint8_t timer_stop_deepening(void) {
  time_t target_usage_cs = target_cs - start_cs;
  time_t used_cs = timer_get_centiseconds() - start_cs;

  // If we have used more than 3/5 of our allotted time, we have no chance of
  // finishing the next depth before a timeup (due to the exponential growth of
  // the tree) -- better to bank the time for later.
  if (used_cs > 3 * target_usage_cs / 5)
    return 1;
  else
    return 0;
}

void timer_end(void) {
  if (opening)
    opening--;

  time_t time_used = timer_get_centiseconds() - start_cs;
  remaining_cs += inc_cs;
  remaining_cs -= time_used;

  if (moves > 0) {
    assert(remaining_moves > 0);
    remaining_moves--;
    if (remaining_moves == 0) {
      remaining_cs += base_cs;
      remaining_moves = moves;
    }
  }

  // More wiggle room.
  remaining_cs -= WIGGLE_ROOM_CS_PER_MOVE;
  if (remaining_cs < 0)
    remaining_cs = 0;
}

time_t timer_get_centiseconds(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * CS_PER_SEC + t.tv_nsec / 10000000;
}
