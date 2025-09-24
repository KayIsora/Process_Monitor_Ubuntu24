#ifndef IO_UTIL_H
#define IO_UTIL_H
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

static inline ssize_t write_full(int fd, const void *buf, size_t len){
  const uint8_t *p = (const uint8_t*)buf;
  size_t off = 0;
  while(off < len){
    ssize_t n = write(fd, p + off, len - off);
    if(n > 0){ off += (size_t)n; continue; }
    if(n < 0 && (errno == EINTR)) continue;
    return n; // error
  }
  return (ssize_t)off;
}
#endif
