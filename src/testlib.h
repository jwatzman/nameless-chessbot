#ifndef _TESTLIB_H
#define _TESTLIB_H

#include <sys/resource.h>

static inline double test_elapsed_time(void) {
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  return (double)(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) +
         (1.0e-6) * (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec);
}

#endif
