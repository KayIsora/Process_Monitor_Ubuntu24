#pragma once
#include <stdint.h>
#include <time.h>

typedef struct {
  uint32_t fail_n;
  uint32_t trip_threshold;
  uint32_t half_open_ms;
  struct timespec open_until;
  int open; // 0 closed, 1 open
} pm_breaker;

void pm_breaker_init(pm_breaker* cb, uint32_t trip_threshold, uint32_t half_open_ms);
int  pm_breaker_allow(pm_breaker* cb);        // 1 => cho phép I/O, 0 => từ chối (OPEN)
void pm_breaker_record(pm_breaker* cb, int ok);// ok=1 reset; ok=0 tăng fail_n/trip
