#pragma once

#include <time.h>

#define B (1000000000L)
#define K (1000L)

uint64_t getCurNs() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  uint64_t t = ts.tv_sec * K * K * K + ts.tv_nsec;
  return t;
}
