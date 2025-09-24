#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
int main(){
  int f = open("/proc/pm/cpu/config", O_WRONLY);
  assert(f>=0);
  assert(write(f,"1 100\n",6)>=0); // bật với interval 100ms
  close(f);
  int d = open("/proc/pm/cpu/data", O_RDONLY);
  assert(d>=0);
  char buf[4096]; int n = read(d, buf, sizeof(buf)-1);
  assert(n>0); buf[n]=0;
  // tối thiểu phải có các khóa KV “cpu.usage” / “ts_ns=”
  assert(strstr(buf,"cpu.usage") && strstr(buf,"ts_ns"));
  close(d);
  return 0;
}
