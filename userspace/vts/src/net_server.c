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
#include "vts_protocol.h"

/* minimal threadpool queue */
#include <stdint.h>
#include <sys/queue.h>
#include "req_handler.h"

#define LISTEN_PORT 9000
#define MAX_EVENTS 128
#define BACKLOG 128

static int make_nonblocking(int fd){
  int flags = fcntl(fd, F_GETFL, 0);
  if(flags == -1) return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int create_and_bind(int port){
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sfd < 0) return -1;
  int on = 1; setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  if(bind(sfd, (struct sockaddr*)&addr, sizeof(addr))<0){
    close(sfd); return -1;
  }
  return sfd;
}

/* We keep a small per-connection read buffer state */
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

static struct conn_head conn_pool = TAILQ_HEAD_INITIALIZER(conn_pool);

static struct conn* conn_new(int fd){
  struct conn *c = calloc(1, sizeof(*c));
  c->fd = fd; c->header_read = 0; c->payload = NULL; c->payload_len = 0; c->payload_read = 0;
  return c;
}
static void conn_free(struct conn *c){
  if(!c) return;
  if(c->payload) free(c->payload);
  close(c->fd);
  free(c);
}

/* process one full frame: hand off to req handler (async)
 * We'll copy payload into handler queue to avoid keeping connection locked here.
 */
static void handle_frame(struct conn *c){
  /* payload is c->payload, len = c->payload_len */
  req_queue_push(c->payload, c->payload_len, c->fd);
  /* ownership moved to queue; null out to avoid double free */
  c->payload = NULL;
  c->payload_len = 0;
  c->payload_read = 0;
}

/* read loop per fd */
static int read_from_conn(struct conn *c){
  ssize_t n;
  /* read header if needed */
  if(c->header_read < 4){
    n = read(c->fd, c->header + c->header_read, 4 - c->header_read);
    if(n <= 0) return n;
    c->header_read += n;
    if(c->header_read < 4) return 1;
    uint32_t be;
    memcpy(&be, c->header, 4);
    uint32_t plen = le32_to_host(be);
    if(plen == 0 || plen > VTS_MAX_FRAME) {
      return -1;
    }
    c->payload_len = plen;
    c->payload = malloc(plen);
    if(!c->payload) return -1;
    c->payload_read = 0;
  }

  /* read payload */
  while(c->payload_read < c->payload_len){
    n = read(c->fd, c->payload + c->payload_read, c->payload_len - c->payload_read);
    if(n <= 0) return n;
    c->payload_read += n;
  }
  /* full frame */
  c->header_read = 0; /* reset for next frame */
  handle_frame(c);
  return 1;
}

/* epoll server main loop */
int vts_run_server(int port){
  int sfd = create_and_bind(port);
  if(sfd < 0) { perror("bind"); return -1; }
  if(make_nonblocking(sfd) < 0){ perror("nonblock"); close(sfd); return -1; }
  if(listen(sfd, BACKLOG) < 0){ perror("listen"); close(sfd); return -1; }

  int efd = epoll_create1(0);
  if(efd < 0){ perror("epoll"); close(sfd); return -1; }

  struct epoll_event ev = { .events = EPOLLIN, .data.fd = sfd };
  epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &ev);

  struct epoll_event events[MAX_EVENTS];
  while(1){
    int n = epoll_wait(efd, events, MAX_EVENTS, 1000);
    if(n < 0){
      if(errno == EINTR) continue;
      perror("epoll_wait"); break;
    }
    for(int i=0;i<n;i++){
      if(events[i].data.fd == sfd){
        /* accept loop */
        while(1){
          struct sockaddr_in inaddr; socklen_t inlen = sizeof(inaddr);
          int infd = accept(sfd, (struct sockaddr*)&inaddr, &inlen);
          if(infd < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("accept"); break;
          }
          make_nonblocking(infd);
          struct epoll_event ev2 = { .events = EPOLLIN | EPOLLET, .data.fd = infd };
          epoll_ctl(efd, EPOLL_CTL_ADD, infd, &ev2);
          /* create conn object and put into pool keyed by fd */
          struct conn *c = conn_new(infd);
          TAILQ_INSERT_TAIL(&conn_pool, c, entries);
        }
      } else {
        /* find conn by fd (O(n) but typical low n; replace by map if needed) */
        int fd = events[i].data.fd;
        struct conn *it;
        TAILQ_FOREACH(it, &conn_pool, entries){
          if(it->fd == fd){
            int r = read_from_conn(it);
            if(r <= 0){
              /* remove and free */
              epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
              TAILQ_REMOVE(&conn_pool, it, entries);
              conn_free(it);
            }
            break;
          }
        }
      }
    }
  }

  close(efd);
  close(sfd);
  return 0;
}

/* entry point called by vts main */
int start_vts_networking(void){
  /* start req handler threads */
  if(req_handler_start() != 0) return -1;
  return vts_run_server(LISTEN_PORT);
}
