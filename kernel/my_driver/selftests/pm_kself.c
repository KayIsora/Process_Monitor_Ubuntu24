#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

static int write_all(int fd, const void *buf, size_t len) {
  const char *p = (const char *)buf;
  while (len > 0) {
    ssize_t w = write(fd, p, len);
    if (w < 0) {
      if (errno == EINTR) continue;
      perror("write");
      return -1;
    }
    p += (size_t)w;
    len -= (size_t)w;
  }
  return 0;
}

static int burst_read(const char* path){
  char buf[4096];
  for (int i=0;i<10000;i++){
    int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) { perror("open status"); return -1; }
    ssize_t r = read(fd, buf, sizeof(buf));
    if (r < 0 && errno != EAGAIN) { perror("read status"); close(fd); return -1; }
    close(fd);
  }
  return 0;
}

int main(void){
  int fd = open("/proc/pm_drv/config", O_WRONLY | O_CLOEXEC);
  if (fd < 0){ perror("open config"); return 1; }
  if (write_all(fd, "1\n", 2) != 0){ close(fd); return 1; }
  close(fd);

  if (burst_read("/proc/pm_drv/status") != 0){
    fprintf(stderr, "burst_read failed\n"); return 2;
  }

  // race: write while reading
  for (int i=0; i<1000; i++){
    int f = open("/proc/pm_drv/config", O_WRONLY | O_CLOEXEC);
    if (f < 0){ perror("open config (race)"); return 3; }
    if (write_all(f, "1\n", 2) != 0){ close(f); return 3; }
    close(f);
    if (burst_read("/proc/pm_drv/status") != 0) return 3;
  }
  return 0;
}
