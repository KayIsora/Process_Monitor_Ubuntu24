#include "circuit_breaker.h"
#include <string.h>

static inline uint64_t now_ms(void){
  struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec*1000ull + ts.tv_nsec/1000000ull;
}

void pm_breaker_init(pm_breaker* cb, uint32_t trip_threshold, uint32_t half_open_ms){
  memset(cb,0,sizeof(*cb));
  cb->trip_threshold = trip_threshold ? trip_threshold : 8;
  cb->half_open_ms   = half_open_ms   ? half_open_ms   : 200;
}

int pm_breaker_allow(pm_breaker* cb){
  if(!cb->open) return 1;
  uint64_t now = now_ms();
  uint64_t open_ms = (uint64_t)cb->open_until.tv_sec*1000ull + cb->open_until.tv_nsec/1000000ull;
  if(now >= open_ms){ cb->open = 0; cb->fail_n = 0; return 1; } // half-open thử lại
  return 0;
}

void pm_breaker_record(pm_breaker* cb, int ok){
  if(ok){ cb->fail_n = 0; return; }
  if(++cb->fail_n >= cb->trip_threshold){
    cb->open = 1;
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ns = (uint64_t)ts.tv_nsec + (uint64_t)cb->half_open_ms*1000000ull;
    cb->open_until.tv_sec  = ts.tv_sec + ns/1000000000ull;
    cb->open_until.tv_nsec = ns%1000000000ull;
    cb->fail_n = 0;
  }
}
