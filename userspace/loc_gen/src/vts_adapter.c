#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "loc_api.h"
#include "backoff.h"
#include "circuit_breaker.h"
#include <sys/time.h>

#ifndef VTS_HOST
#define VTS_HOST "127.0.0.1"
#endif
#ifndef VTS_PORT
#define VTS_PORT 9090
#endif

static uint64_t now_ms(void){
  struct timespec ts; timespec_get(&ts, TIME_UTC);
  return (uint64_t)ts.tv_sec*1000ull + ts.tv_nsec/1000000ull;
}
static uint64_t gen_req_id(void){ return now_ms(); }

static int connect_vts(void){
  int s=socket(AF_INET,SOCK_STREAM,0); if(s<0) return -1;

  struct timeval tv; tv.tv_sec = 1; tv.tv_usec = 0;
  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

  struct sockaddr_in a={0}; a.sin_family=AF_INET; a.sin_port=htons(VTS_PORT); a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  if(connect(s,(struct sockaddr*)&a,sizeof(a))<0){ close(s); return -1; }
  return s;
}
static int send_frame(int s, const void*buf, uint32_t len){
  uint32_t nlen=htonl(len);
  if(write(s,&nlen,4)!=4) return -1;
  size_t off=0; const char *p=(const char*)buf;
  while(off<len){ ssize_t w=write(s,p+off,len-off); if(w<=0) return -1; off+=w; }
  return 0;
}
static int recv_frame(int s, char *buf, size_t buflen){
  uint32_t nlen; if(read(s,&nlen,4)!=4) return -1;
  uint32_t len=ntohl(nlen); if(len+1>buflen) return -2;
  size_t off=0; while(off<len){ ssize_t r=read(s,buf+off,len-off); if(r<=0) return -1; off+=r; }
  buf[len]=0; return (int)len;
}

static int vts_cmd_with_retry(const char *json_req, char *resp, size_t respsz){
  pm_backoff bo;
  pm_backoff_init(&bo, /*init_us*/ 10000, /*max_us*/ 200000); // 10ms -> 200ms
  int attempt = 0;

  for(;;){
    int s = connect_vts();
    if(s >= 0){
      int rc = send_frame(s, json_req, (uint32_t)strlen(json_req));
      if(rc == 0){
        int n = recv_frame(s, resp, respsz);
        close(s);
        if(n > 0) return 0;            // OK
      } else {
        close(s);
      }
    }
    if(++attempt >= 5) return -1;      // hết retry
    unsigned us = pm_backoff_next(&bo);
    sleep(us);
  }
}

/* ---- Public ops ---- */

int vts_apply_room(const room_t *r){
  char req[512];
  uint64_t req_id=gen_req_id();
  // Operation is idempotent by definition (set desired state & type for id)
  // Server will upsert room by id
  snprintf(req,sizeof(req),
    "{\"type\":\"room.apply\",\"req_id\":%llu,\"room\":{"
    "\"id\":\"%s\",\"type\":\"%s\",\"state\":\"%s\"}}",
    (unsigned long long)req_id, r->id, r->type, room_state_str(r->state));
  char resp[512];
  int rc=vts_cmd_with_retry(req,resp,sizeof(resp));
  return rc;
}

int vts_delete_room(const char *id){
  char req[256];
  uint64_t req_id=gen_req_id();
  snprintf(req,sizeof(req),
    "{\"type\":\"room.delete\",\"req_id\":%llu,\"id\":\"%s\"}",
    (unsigned long long)req_id, id);
  char resp[512];
  return vts_cmd_with_retry(req,resp,sizeof(resp));
}

int vts_show_room(const char *id, char *buf, size_t buflen){
  char req[256];
  uint64_t req_id=gen_req_id();
  snprintf(req,sizeof(req),
    "{\"type\":\"room.show\",\"req_id\":%llu,\"id\":\"%s\"}",
    (unsigned long long)req_id, id);
  return vts_cmd_with_retry(req,buf,buflen);
}
