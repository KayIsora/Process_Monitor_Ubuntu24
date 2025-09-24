#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include "vts_protocol.h"
#include "vts_log.h"

#include <unistd.h>   // usleep
#include "io_util.h"

#define WORKER_COUNT 8
#define QUEUE_DEPTH 10240
static void send_resp(int fd, const char *req_id, int success);

typedef struct work {
  char *payload;
  uint32_t len;
  int peer_fd;
  struct work *next;
} work_t;

/* queue + cond */
static pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t qcond = PTHREAD_COND_INITIALIZER;
static work_t *qhead = NULL, *qtail = NULL;
static int qlen = 0;

/* dedup bằng vòng tròn đơn giản */
#define DEDUP_MAX 4096
static struct { char id[64]; int result; int used; } dedup_tab[DEDUP_MAX];
static size_t dedup_pos = 0;
static pthread_mutex_t dedup_lock = PTHREAD_MUTEX_INITIALIZER;

static int dedup_find(const char *id, int *out_res){
  for(size_t i=0;i<DEDUP_MAX;i++){
    if(dedup_tab[i].used && strcmp(dedup_tab[i].id, id)==0){
      if(out_res) *out_res = dedup_tab[i].result;
      return 1;
    }
  }
  return 0;
}
static int json_get_str(const char *payload, const char *key, char *out, size_t outsz){
  // Tìm "key":"..."; không hỗ trợ escape phức tạp (đủ cho payload của ta)
  char pat[64];
  snprintf(pat, sizeof(pat), "\"%s\"", key);
  const char *k = strstr(payload, pat);
  if(!k) return -1;
  const char *colon = strchr(k, ':'); if(!colon) return -1;
  const char *q1 = strchr(colon, '"'); if(!q1) return -1;
  const char *q2 = strchr(q1+1, '"'); if(!q2) return -1;
  size_t n = (size_t)(q2 - (q1+1));
  if(n >= outsz) n = outsz-1;
  memcpy(out, q1+1, n); out[n]=0;
  return 0;
}

static void dedup_put(const char *id, int res){
  strncpy(dedup_tab[dedup_pos].id, id, sizeof(dedup_tab[dedup_pos].id)-1);
  dedup_tab[dedup_pos].id[sizeof(dedup_tab[dedup_pos].id)-1] = 0;
  dedup_tab[dedup_pos].result = res;
  dedup_tab[dedup_pos].used = 1;
  dedup_pos = (dedup_pos + 1) % DEDUP_MAX;
}

int req_handler_start(void); /* fwd */

void req_queue_push(uint8_t *payload, uint32_t len, int peer_fd){
  work_t *w = malloc(sizeof(*w));
  w->payload = malloc(len+1);
  memcpy(w->payload, payload, len);
  w->payload[len]=0;
  w->len = len;
  w->peer_fd = peer_fd;
  w->next = NULL;
  free(payload);

  pthread_mutex_lock(&qlock);
  if(qlen >= QUEUE_DEPTH){
    work_t *old = qhead;
    if(old){
      qhead = old->next;
      if(!qhead) qtail = NULL;
      free(old->payload); free(old);
      qlen--;
    }
  }
  if(!qhead) qhead = w, qtail = w; else { qtail->next = w; qtail = w; }
  qlen++;
  pthread_cond_signal(&qcond);
  pthread_mutex_unlock(&qlock);
}


static void process_one(work_t *w){
  /* Rất tối giản: trích req_id và op bằng sscanf */
  char req_id[64]={0}, op[32]={0};

if (json_get_str(w->payload, "req_id", req_id, sizeof(req_id)) != 0) strcpy(req_id,"noid");
if (json_get_str(w->payload, "op",     op,     sizeof(op))     != 0) strcpy(op,"NOP");

  if(!req_id[0]) strcpy(req_id, "noid");
  if(!op[0]) strcpy(op, "NOP");

  int cached = 0, cached_res = 0;
  pthread_mutex_lock(&dedup_lock);
  cached = dedup_find(req_id, &cached_res);
  pthread_mutex_unlock(&dedup_lock);

  if(cached){
    log_write_req_result(req_id, cached_res);
    send_resp(w->peer_fd, req_id, cached_res);
  } else {
    /* xử lý: mọi op != "FAIL" coi là OK */
    int success = strcmp(op,"FAIL")==0 ? 0 : 1;
    /* mô phỏng latency nhẹ */
    //sleep(1000);
    pthread_mutex_lock(&dedup_lock);
    dedup_put(req_id, success);
    pthread_mutex_unlock(&dedup_lock);
    log_write_req_result(req_id, success);
    send_resp(w->peer_fd, req_id, success);
  }

  free(w->payload);
  free(w);
}

static void *worker_fn(void *arg){
  (void)arg;
  while(1){
    pthread_mutex_lock(&qlock);
    while(!qhead) pthread_cond_wait(&qcond, &qlock);
    work_t *w = qhead;
    qhead = w->next;
    if(!qhead) qtail = NULL;
    qlen--;
    pthread_mutex_unlock(&qlock);
    process_one(w);
  }
  return NULL;
}

int req_handler_start(void){
  pthread_t th[WORKER_COUNT];
  for(int i=0;i<WORKER_COUNT;i++){
    pthread_create(&th[i], NULL, worker_fn, NULL);
    pthread_detach(th[i]);
  }
  return 0;
}


// --- chỉ giữ MỘT bản send_resp duy nhất ---
static void send_resp(int fd, const char *req_id, int success){
  char resp[256];
  int n = snprintf(resp, sizeof(resp),
                   "{\"req_id\":\"%s\",\"result\":\"%s\"}",
                   req_id ? req_id : "-", success ? "OK" : "ERR");
  if (n <= 0) return;
  uint32_t len_be = htonl((uint32_t)n);
  (void)write_full(fd, &len_be, 4);
  (void)write_full(fd, resp, (size_t)n);
}
