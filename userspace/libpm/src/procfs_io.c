#define _GNU_SOURCE
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>

#include "libpm.h"
#include "backoff.h"
#include "circuit_breaker.h"

static pm_cfg g_cfg = {
  .root="/proc/pm", .open_flags_extra=0,
  .timeout_ms=200, .backoff_init_us=200, .backoff_max_us=20000,
  .breaker_fail_n=8, .breaker_half_ms=200
};
static pm_stats g_stats;
static pm_breaker g_cb;

static inline uint64_t now_ms(void){
  struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec*1000ull + ts.tv_nsec/1000000ull;
}
static inline const char* node_dir(pm_node_t n){
  switch(n){case PM_NODE_CPU:return "cpu"; case PM_NODE_MEM:return "mem"; default:return "inf";}
}
const char* pm_node_str(pm_node_t n){ return node_dir(n); }

void pm_init(const pm_cfg* opt){
  if(opt) g_cfg = *opt;
  memset(&g_stats,0,sizeof(g_stats));
  pm_breaker_init(&g_cb, g_cfg.breaker_fail_n, g_cfg.breaker_half_ms);
}
void pm_get_stats(pm_stats* out){ if(out) *out = g_stats; }
void pm_reset_stats(void){ memset(&g_stats,0,sizeof(g_stats)); }

static int open_nb(const char* p, int write_mode){
  int flags = (write_mode? O_WRONLY: O_RDONLY) | O_NONBLOCK | g_cfg.open_flags_extra;
  return open(p, flags);
}

static pm_rc read_text_file(const char* path, uint8_t* buf, size_t buflen, size_t* out_n){
  if(!pm_breaker_allow(&g_cb)) { g_stats.breaker_trips++; return PM_AGAIN; }
  uint64_t t0 = now_ms(); pm_backoff bo; pm_backoff_init(&bo,g_cfg.backoff_init_us,g_cfg.backoff_max_us);

  for(;;){
    int fd = open_nb(path, 0);
    if(fd<0){ if(errno==EAGAIN||errno==EMFILE){ goto retry; } return PM_IOERR; }

    struct pollfd pfd={.fd=fd,.events=POLLIN};
    int pr = poll(&pfd,1,(int)g_cfg.timeout_ms);
    if(pr==0){ close(fd); g_stats.io_timeout++; pm_breaker_record(&g_cb,0); return PM_TIMEOUT; }
    if(pr<0){ int e=errno; close(fd); if(e==EINTR) goto retry; pm_breaker_record(&g_cb,0); return PM_IOERR; }

    ssize_t n = read(fd, buf, buflen? buflen-1:0);
    int e=errno; close(fd);
    if(n>=0){
      if(buflen) ((char*)buf)[n] = '\0';
      if(out_n) *out_n=(size_t)n;
      g_stats.io_ok++; pm_breaker_record(&g_cb,1);
      return PM_OK;
    }
    if(e==EAGAIN || e==EINTR){ goto retry; }
    g_stats.io_err++; pm_breaker_record(&g_cb,0);
    return PM_IOERR;

retry:;
    g_stats.io_retry++;
    if(now_ms()-t0 >= g_cfg.timeout_ms){ g_stats.io_timeout++; pm_breaker_record(&g_cb,0); return PM_TIMEOUT; }
    usleep(pm_backoff_next(&bo));
  }
}

static pm_rc write_text_file(const char* path, const char* buf, uint32_t len){
  if(!pm_breaker_allow(&g_cb)) { g_stats.breaker_trips++; return PM_AGAIN; }
  uint64_t t0 = now_ms(); pm_backoff bo; pm_backoff_init(&bo,g_cfg.backoff_init_us,g_cfg.backoff_max_us);

  for(;;){
    int fd = open_nb(path, 1);
    if(fd<0){ if(errno==EAGAIN||errno==EMFILE){ goto retry; } return PM_IOERR; }

    struct pollfd pfd={.fd=fd,.events=POLLOUT};
    int pr = poll(&pfd,1,(int)g_cfg.timeout_ms);
    if(pr==0){ close(fd); g_stats.io_timeout++; pm_breaker_record(&g_cb,0); return PM_TIMEOUT; }
    if(pr<0){ int e=errno; close(fd); if(e==EINTR) goto retry; pm_breaker_record(&g_cb,0); return PM_IOERR; }

    ssize_t n = write(fd, buf, len);
    int e=errno; close(fd);
    if(n==(ssize_t)len){ g_stats.io_ok++; pm_breaker_record(&g_cb,1); return PM_OK; }
    if(e==EAGAIN || e==EINTR){ goto retry; }
    g_stats.io_err++; pm_breaker_record(&g_cb,0);
    return PM_IOERR;

retry:;
    g_stats.io_retry++;
    if(now_ms()-t0 >= g_cfg.timeout_ms){ g_stats.io_timeout++; pm_breaker_record(&g_cb,0); return PM_TIMEOUT; }
    usleep(pm_backoff_next(&bo));
  }
}

static void build_path(char* out, size_t cap, pm_node_t node, pm_file_t file){
  const char* f = (file==PM_FILE_CONFIG?"config": file==PM_FILE_STATUS?"status":"data");
  snprintf(out, cap, "%s/%s/%s", g_cfg.root? g_cfg.root:"/proc/pm", node_dir(node), f);
}

pm_rc pm_read_status(pm_node_t node, char* buf, size_t buflen){
  char p[256]; build_path(p,sizeof(p),node,PM_FILE_STATUS);
  return read_text_file(p,(uint8_t*)buf,buflen,NULL);
}
pm_rc pm_read_data(pm_node_t node, uint8_t* buf, size_t buflen, size_t* out_n){
  char p[256]; build_path(p,sizeof(p),node,PM_FILE_DATA);
  return read_text_file(p,buf,buflen,out_n);
}
pm_rc pm_set_config(pm_node_t node, const char* payload, uint32_t payload_len){
  char p[256]; build_path(p,sizeof(p),node,PM_FILE_CONFIG);
  return write_text_file(p,payload,payload_len);
}
