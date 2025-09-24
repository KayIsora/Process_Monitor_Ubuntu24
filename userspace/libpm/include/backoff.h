#pragma once
#include <stdint.h>
typedef struct {
  uint32_t cur_us, max_us;
} pm_backoff;
static inline void pm_backoff_init(pm_backoff* b, uint32_t init_us, uint32_t max_us){
  b->cur_us = init_us ? init_us : 100;
  b->max_us = max_us ? max_us : 20000;
}
static inline uint32_t pm_backoff_next(pm_backoff* b){
  uint32_t v = b->cur_us;
  uint64_t nxt = (uint64_t)b->cur_us * 2u;
  b->cur_us = (nxt > b->max_us) ? b->max_us : (uint32_t)nxt;
  return v;
}
