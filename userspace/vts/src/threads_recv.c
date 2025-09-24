// SPDX-License-Identifier: MIT
#define _XOPEN_SOURCE 700   // để usleep có prototype
#include "vts_fifo.h"
#include <pthread.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>  // mkfifo
#include <stdlib.h>

extern vts_fifo_t g_q;        // SPSC FIFO (đã khởi tạo ở core)
extern _Atomic int g_stop;    // set = 1 để dừng threads gọn gàng

static int open_fifo_ro(const char *p){
  if (mkfifo(p, 0666) < 0 && errno != EEXIST) {
    perror("mkfifo");
    // vẫn thử mở nếu FIFO đã tồn tại
  }
  int fd = open(p, O_RDONLY | O_NONBLOCK);
  return fd;
}

static void handle_line(const char *line){
  // Một message mỗi dòng; tùy implementation của FIFO:
  // vts_fifo_push_spsc() trong dự án đã copy dữ liệu vào queue (Pha 3-6),
  // nên ta có thể truyền con trỏ tạm thời.
  vts_fifo_push_spsc(&g_q, line);
}

static void *thr_recv(void *arg){
  const char *path = (const char*)arg;
  int fd = open_fifo_ro(path);
  if(fd < 0){ perror("open fifo"); return NULL; }

  int ep = epoll_create1(0);
  if (ep < 0) { perror("epoll_create1"); close(fd); return NULL; }

  struct epoll_event ev = {.events=EPOLLIN, .data.fd=fd};
  epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev);

  char buf[65536];
  char line[4096]; size_t lpos=0;

  while(!atomic_load(&g_stop)){
    struct epoll_event out;
    int n = epoll_wait(ep, &out, 1, 100); // timeout 100ms
    if(n <= 0) continue;

    if(out.events & (EPOLLHUP | EPOLLERR)){
      // Writer đóng; reopen chờ writer mới
      close(fd);
      fd = open_fifo_ro(path);
      if(fd < 0) { sleep(1000*100); continue; }
      ev.events = EPOLLIN; ev.data.fd = fd;
      epoll_ctl(ep, EPOLL_CTL_MOD, fd, &ev);
      continue;
    }

    ssize_t r = read(fd, buf, sizeof(buf));
    if(r < 0){
      if(errno==EAGAIN || errno==EINTR) continue;
      // writer closed -> reopen for new writers
      close(fd); fd = open_fifo_ro(path);
      if(fd < 0) { sleep(1000*100); }
      continue;
    }
    if(r == 0){
      // EOF: all writers closed; re-open to block until new writer
      close(fd); fd = open_fifo_ro(path);
      if(fd < 0) { sleep(1000*100); }
      continue;
    }

    for(ssize_t i=0;i<r;i++){
      if(buf[i]=='\n' || lpos==sizeof(line)-1){
        line[lpos] = '\0';
        if(lpos > 0) handle_line(line);
        lpos = 0;
      } else {
        line[lpos++] = buf[i];   // <-- quan trọng: tăng lpos
      }
    }
  }

  close(fd);
  close(ep);
  return NULL;
}

/* Public API */
pthread_t g_thr_recv;   // bỏ 'static' để export symbol
int vts_thread_spawn_recv(const char *fifo_path){
  return pthread_create(&g_thr_recv, NULL, thr_recv, (void*)fifo_path);
}
