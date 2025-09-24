// SPDX-License-Identifier: MIT
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/queue.h>

#include "vts_protocol.h"   // VTS_MAX_FRAME, framing, etc.
#include "req_handler.h"    // req_handler_start(), req_queue_push()

/* ---------- Config ---------- */
#ifndef DEFAULT_TCP_PORT
#define DEFAULT_TCP_PORT 9000
#endif
#ifndef DEFAULT_UDP_PORT
#define DEFAULT_UDP_PORT 9001
#endif

#define MAX_EVENTS  256
#define BACKLOG     128



static int make_nonblocking(int fd){
  int flags = fcntl(fd, F_GETFL, 0);
  if(flags == -1) return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int tcp_create_and_bind(int port){
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sfd < 0) return -1;
  int on = 1; setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#ifdef SO_REUSEPORT
  setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
#endif
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((uint16_t)port);
  if(bind(sfd, (struct sockaddr*)&addr, sizeof(addr))<0){
    close(sfd); return -1;
  }
  return sfd;
}

static int udp_create_and_bind(int port){
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if(fd < 0) return -1;
  int on = 1; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#ifdef SO_REUSEPORT
  setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
#endif
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((uint16_t)port);
  if(bind(fd, (struct sockaddr*)&addr, sizeof(addr))<0){
    close(fd); return -1;
  }
  return fd;
}

/* ---------- Per-connection state ---------- */
struct conn {
  int fd;
  uint8_t header[4];
  size_t header_read;
  uint32_t payload_len;
  uint8_t *payload;
  size_t payload_read;
  TAILQ_ENTRY(conn) entries;
};
TAILQ_HEAD(conn_head, conn);
static struct conn_head g_conns = TAILQ_HEAD_INITIALIZER(g_conns);

static struct conn* conn_new(int fd){
  struct conn *c = calloc(1, sizeof(*c));
  if(!c) return NULL;
  c->fd = fd;
  return c;
}
static void conn_free(struct conn *c){
  if(!c) return;
  if(c->payload) free(c->payload);
  close(c->fd);
  free(c);
}

/* ---------- Frame handling ---------- */
/* Handoff full frame to request handler (queue takes ownership). */
static void handle_frame(struct conn *c){
  /* payload is c->payload, len = c->payload_len, origin fd = c->fd */
  req_queue_push(c->payload, c->payload_len, c->fd);
  /* ownership moved to queue */
  c->payload = NULL;
  c->payload_len = 0;
  c->payload_read = 0;
}

/* Non-blocking read; supports multiple frames per event (edge-triggered). */
static int read_from_conn(struct conn *c){
  ssize_t n;

  /* Loop while readable: may contain multiple frames */
  for(;;){
    /* Read header if needed */
    if(c->header_read < 4){
      n = read(c->fd, c->header + c->header_read, 4 - c->header_read);
      if(n == 0) return 0;                 // peer closed
      if(n < 0){
        if(errno==EAGAIN || errno==EWOULDBLOCK) return 1; // no more data now
        return -1; // hard error
      }
      c->header_read += (size_t)n;
      if(c->header_read < 4) continue;     // need more
      uint32_t le;
      memcpy(&le, c->header, 4);
      uint32_t plen = le32_to_host(le);
      if(plen == 0 || plen > VTS_MAX_FRAME) {
        errno = EMSGSIZE;
        return -1;
      }
      c->payload_len = plen;
      c->payload = (uint8_t*)malloc(plen);
      if(!c->payload) return -1;
      c->payload_read = 0;
    }

    /* Read payload */
    while(c->payload_read < c->payload_len){
      n = read(c->fd, c->payload + c->payload_read, c->payload_len - c->payload_read);
      if(n == 0) return 0; // closed in the middle -> treat as close
      if(n < 0){
        if(errno==EAGAIN || errno==EWOULDBLOCK) return 1;
        return -1;
      }
      c->payload_read += (size_t)n;
    }

    /* Full frame ready */
    c->header_read = 0; /* reset for next frame */
    handle_frame(c);
    /* loop to see if there's another frame already buffered by kernel */
  }
}

/* ---------- EPOLL server main loop (TCP + UDP) ---------- */
static int run_server_loop(int tcp_port, int udp_port){
  int sfd = tcp_create_and_bind(tcp_port);
  if(sfd < 0){ perror("bind(tcp)"); return -1; }
  if(make_nonblocking(sfd) < 0){ perror("nonblock(tcp)"); close(sfd); return -1; }
  if(listen(sfd, BACKLOG) < 0){ perror("listen"); close(sfd); return -1; }

  int ufd = -1;
  if(udp_port > 0){
    ufd = udp_create_and_bind(udp_port);
    if(ufd < 0){
      perror("bind(udp)"); /* continue with TCP only */
    } else {
      if(make_nonblocking(ufd) < 0){ perror("nonblock(udp)"); close(ufd); ufd = -1; }
    }
  }

  int efd = epoll_create1(0);
  if(efd < 0){ perror("epoll_create1"); close(sfd); if(ufd>=0) close(ufd); return -1; }

  struct epoll_event ev = { .events = EPOLLIN, .data.fd = sfd };
  epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &ev);

  if(ufd >= 0){
    struct epoll_event evu = { .events = EPOLLIN, .data.fd = ufd };
    epoll_ctl(efd, EPOLL_CTL_ADD, ufd, &evu);
  }

  struct epoll_event events[MAX_EVENTS];
  for(;;){
    int n = epoll_wait(efd, events, MAX_EVENTS, 1000);
    if(n < 0){
      if(errno == EINTR) continue;
      perror("epoll_wait");
      break;
    }
    for(int i=0;i<n;i++){
      int fd = events[i].data.fd;

      /* New TCP connection */
      if(fd == sfd){
        for(;;){
          struct sockaddr_in inaddr; socklen_t inlen = sizeof(inaddr);
          int infd = accept(sfd, (struct sockaddr*)&inaddr, &inlen);
          if(infd < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("accept"); break;
          }
          make_nonblocking(infd);
          struct epoll_event ev2 = { .events = EPOLLIN | EPOLLET | EPOLLRDHUP, .data.fd = infd };
          epoll_ctl(efd, EPOLL_CTL_ADD, infd, &ev2);

          struct conn *c = conn_new(infd);
          if(!c){
            epoll_ctl(efd, EPOLL_CTL_DEL, infd, NULL);
            close(infd);
            continue;
          }
          TAILQ_INSERT_TAIL(&g_conns, c, entries);
        }
        continue;
      }

      /* UDP datagram */
      if(ufd >= 0 && fd == ufd){
        uint8_t buf[2048];
        for(;;){
          ssize_t r = recvfrom(ufd, buf, sizeof(buf), 0, NULL, NULL);
          if(r < 0){
            if(errno==EAGAIN || errno==EWOULDBLOCK) break;
            perror("recvfrom"); break;
          }
          if(r == 0) break;
          /* Treat each datagram as one full request (idempotent GETs) */
          size_t len = (size_t)r;
          if(len == 0 || len > VTS_MAX_FRAME) continue;
          uint8_t *copy = (uint8_t*)malloc(len);
          if(!copy) continue;
          memcpy(copy, buf, len);
          /* origin fd = -1 for UDP (no response channel) or handled elsewhere */
          req_queue_push(copy, (uint32_t)len, -1);
        }
        continue;
      }

      /* Activity on an existing TCP connection */
      struct conn *it, *tmp;
      TAILQ_FOREACH(it, &g_conns, entries){
        if(it->fd == fd){
          int r = read_from_conn(it);
          if(r <= 0 || (events[i].events & (EPOLLHUP|EPOLLRDHUP|EPOLLERR))){
            epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
            /* Remove & free */
            tmp = it;
            it = TAILQ_NEXT(it, entries);  /* safe cursor advance */
            TAILQ_REMOVE(&g_conns, tmp, entries);
            conn_free(tmp);
          }
          break;
        }
      }
    }
  }

  /* Cleanup */
  while(!TAILQ_EMPTY(&g_conns)){
    struct conn *c = TAILQ_FIRST(&g_conns);
    TAILQ_REMOVE(&g_conns, c, entries);
    conn_free(c);
  }
  close(efd);
  close(sfd);
  if(ufd>=0) close(ufd);
  return 0;
}

/* ---------- Public entry (called by vts main) ---------- */
int start_vts_networking(void){
  /* Start worker threads that consume req_queue_push() items */
  if(req_handler_start() != 0) {
    fprintf(stderr, "req_handler_start failed\n");
    return -1;
  }

  /* Ports via env (fallback to defaults) */
  int tcp_port = DEFAULT_TCP_PORT;
  int udp_port = DEFAULT_UDP_PORT;
  const char *p = getenv("VTS_PORT");
  if(p && *p) tcp_port = atoi(p);
  p = getenv("VTS_UDP_PORT");
  if(p && *p) udp_port = atoi(p);

  return run_server_loop(tcp_port, udp_port);
}
