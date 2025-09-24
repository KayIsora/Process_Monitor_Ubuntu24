#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "libpm.h"
#include "test_util.h"

int main(){
  const char* ROOT = getenv("PM_FAKE_ROOT");
  if(!ROOT) ROOT="/tmp/fake_pm";
  pm_test_bootstrap(ROOT);

  // đọc 3 node
  char sbuf[256];
  assert(pm_read_status(PM_NODE_CPU, sbuf, sizeof(sbuf))==PM_OK);
  assert(pm_read_status(PM_NODE_MEM, sbuf, sizeof(sbuf))==PM_OK);
  assert(pm_read_status(PM_NODE_INF, sbuf, sizeof(sbuf))==PM_OK);

  // config bật/tắt
  assert(pm_set_config(PM_NODE_CPU, "1 50\n", 5)==PM_OK);
  assert(pm_set_config(PM_NODE_CPU, "0\n", 2)==PM_OK);
  return 0;
}
