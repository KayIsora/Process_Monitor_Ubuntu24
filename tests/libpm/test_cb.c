#include <assert.h>
#include <stdio.h>
#include "libpm.h"
#include "test_util.h"

int main(){
  const char* ROOT = "/path/not/exist"; // ép lỗi liên tiếp
  pm_test_bootstrap(ROOT);

  char b[8];
  int trips=0;
  for(int i=0;i<20;i++){
    pm_rc rc = pm_read_status(PM_NODE_CPU, b, sizeof(b));
    if(rc==PM_AGAIN) trips++;
  }
  pm_stats st; pm_get_stats(&st);
  // Breaker phải có lần trip
  assert(trips>0 || st.breaker_trips>0);
  return 0;
}
