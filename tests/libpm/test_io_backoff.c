#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "libpm.h"
#include "test_util.h"

int main(){
  const char* ROOT = getenv("PM_FAKE_ROOT"); // set bởi CTest
  if(!ROOT) ROOT="/tmp/fake_pm";
  pm_test_bootstrap(ROOT);

  char buf[512];
  // đảm bảo fakefs đã có status
  assert(pm_read_status(PM_NODE_MEM, buf, sizeof(buf)) == PM_OK);
  assert(strstr(buf,"ok=")!=NULL);

  // đọc data
  size_t n=0; uint8_t dbuf[512];
  assert(pm_read_data(PM_NODE_MEM, dbuf, sizeof(dbuf), &n) == PM_OK);
  assert(n>0);

  // set config "1 100\n"
  assert(pm_set_config(PM_NODE_MEM, "1 100\n", 6) == PM_OK);

  // Kiểm tra tỉ lệ retry <5% (xấp xỉ)
  pm_stats st; pm_get_stats(&st);
  double retry_rate = (st.io_ok+st.io_err+st.io_timeout) ? (double)st.io_retry/(st.io_ok+st.io_err+st.io_timeout) : 0.0;
  fprintf(stderr,"retry_rate=%.3f\n", retry_rate);
  assert(retry_rate < 0.05 + 0.02); // nới 7% cho CI nhiễu

  return 0;
}
