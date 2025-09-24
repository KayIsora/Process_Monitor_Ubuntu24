#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpm.h"

static inline void pm_test_bootstrap(const char* root){
  pm_cfg cfg = {.root=root, .timeout_ms=200, .backoff_init_us=200,
                .backoff_max_us=5000, .breaker_fail_n=8, .breaker_half_ms=100};
  pm_init(&cfg);
  pm_reset_stats();
}
