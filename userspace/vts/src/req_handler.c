#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>       // usleep, write
#include <arpa/inet.h>    // htonl
#include "vts_protocol.h"
#include "vts_log.h"
#include "io_util.h"

/* ===================== Worker queue ===================== */
#define WORKER_COUNT 8
#define QUEUE_DEPTH  10240

typedef struct work {
  char *payload;
  uint32_t len;
  int peer_fd;
  struct work *next;
} work_t;

/* queue + cond */
static pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  qcond = PTHREAD_COND_INITIALIZER;
static work_t *qhead = NULL, *qtail = NULL;
static int qlen = 0;

/* ===================== Dedup (req_id) ===================== */
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
static void dedup_put(const char *id, int res){
  if (!id || !*id) return;
  strncpy(dedup_tab[dedup_pos].id, id, sizeof(dedup_tab[dedup_pos].id)-1);
  dedup_tab[dedup_pos].id[sizeof(dedup_tab[dedup_pos].id)-1] = 0;
  dedup_tab[dedup_pos].result = res;
  dedup_tab[dedup_pos].used = 1;
  dedup_pos = (dedup_pos + 1) % DEDUP_MAX;
}

/* ===================== Utilities ===================== */
static void send_length_prefixed(int fd, const char *buf, size_t n) {
  uint32_t len = (uint32_t)n;
  uint32_t nlen = htonl(len);
  (void)write_full(fd, &nlen, 4);
  (void)write_full(fd, buf, n);
}

/* Legacy response: {"req_id":"...","result":"OK|ERR"} */
static void send_resp_legacy(int fd, const char *req_id, int success){
  char resp[256];
  int n = snprintf(resp, sizeof(resp),
                   "{\"req_id\":\"%s\",\"result\":\"%s\"}",
                   req_id ? req_id : "-", success ? "OK" : "ERR");
  if (n <= 0) return;
  send_length_prefixed(fd, resp, (size_t)n);
}

/* Phase-6 response: {"code":int,"msg":"...",["data":"..."]} */
static void reply_json(int fd, int code, const char *msg, const char *data) {
  char body[512];
  int m;
  if (data && *data) {
    m = snprintf(body, sizeof(body),
                 "{\"code\":%d,\"msg\":\"%s\",\"data\":\"%s\"}", code, msg, data);
  } else {
    m = snprintf(body, sizeof(body),
                 "{\"code\":%d,\"msg\":\"%s\"}", code, msg);
  }
  if (m < 0) m = 0;
  send_length_prefixed(fd, body, (size_t)m);
}

/* Rất gọn: tìm "key":"value" (không xử lý escape phức tạp) */
static int json_get_str(const char *payload, const char *key, char *out, size_t outsz){
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

/* ===================== In-memory Room Registry ===================== */
#define MAX_ROOMS 64
static char g_rooms[MAX_ROOMS][64];
static int  g_room_count = 0;
static pthread_mutex_t rooms_lock = PTHREAD_MUTEX_INITIALIZER;

static bool room_exists(const char *name) {
  bool ok = false;
  pthread_mutex_lock(&rooms_lock);
  for (int i = 0; i < g_room_count; ++i)
    if (strcmp(g_rooms[i], name) == 0) { ok = true; break; }
  pthread_mutex_unlock(&rooms_lock);
  return ok;
}

static int room_create(const char *name) {
  pthread_mutex_lock(&rooms_lock);
  for (int i = 0; i < g_room_count; ++i) {
    if (strcmp(g_rooms[i], name) == 0) { pthread_mutex_unlock(&rooms_lock); return 1; } // exists
  }
  if (g_room_count >= MAX_ROOMS) { pthread_mutex_unlock(&rooms_lock); return -1; } // full
  strncpy(g_rooms[g_room_count], name, sizeof(g_rooms[0]) - 1);
  g_rooms[g_room_count][sizeof(g_rooms[0]) - 1] = '\0';
  g_room_count++;
  pthread_mutex_unlock(&rooms_lock);
  return 0;
}

static int room_delete(const char *name) {
  pthread_mutex_lock(&rooms_lock);
  for (int i = 0; i < g_room_count; ++i) {
    if (strcmp(g_rooms[i], name) == 0) {
      --g_room_count;
      if (i != g_room_count) {
        memcpy(g_rooms[i], g_rooms[g_room_count], sizeof(g_rooms[0]));
      }
      pthread_mutex_unlock(&rooms_lock);
      return 0;
    }
  }
  pthread_mutex_unlock(&rooms_lock);
  return 1; // not found
}

/* ===================== Public queue API ===================== */
int req_handler_start(void); /* fwd */

void req_queue_push(uint8_t *payload, uint32_t len, int peer_fd){
  work_t *w = (work_t*)malloc(sizeof(*w));
  w->payload = (char*)malloc(len+1);
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

/* ===================== Request processing ===================== */
static void handle_legacy(const char *payload, int fd){
  /* Legacy: fields req_id + op; mọi op != "FAIL" coi là OK */
  char req_id[64]={0}, op[32]={0};
  if (json_get_str(payload, "req_id", req_id, sizeof(req_id)) != 0) strcpy(req_id,"noid");
  if (json_get_str(payload, "op",     op,     sizeof(op))     != 0) strcpy(op,"NOP");

  /* Dedup nếu có req_id hợp lệ */
  int used_cache = 0, cached_res = 0;
  if (req_id[0]) {
    pthread_mutex_lock(&dedup_lock);
    used_cache = dedup_find(req_id, &cached_res);
    pthread_mutex_unlock(&dedup_lock);
  }

  if (used_cache){
    log_write_req_result(req_id, cached_res);
    send_resp_legacy(fd, req_id, cached_res);
  } else {
    int success = strcmp(op,"FAIL")==0 ? 0 : 1;
    if (req_id[0]) {
      pthread_mutex_lock(&dedup_lock);
      dedup_put(req_id, success);
      pthread_mutex_unlock(&dedup_lock);
    }
    log_write_req_result(req_id[0]?req_id:"-", success);
    send_resp_legacy(fd, req_id, success);
  }
}

static void handle_cli(const char *payload, int fd){
  /* Phase 6 CLI: fields cmd,target,name,[key,val] */
  char cmd[32]={0}, target[32]={0}, name[64]={0}, key[64]={0}, val[128]={0};
  int has_cmd = (json_get_str(payload, "cmd", cmd, sizeof(cmd))==0);
  int has_target = (json_get_str(payload, "target", target, sizeof(target))==0);
  int has_name = (json_get_str(payload, "name", name, sizeof(name))==0);

  if (!has_cmd || !has_target || !has_name) {
    reply_json(fd, 1001, "Bad request", NULL);
    return;
  }
  if (strcmp(target, "room") != 0) {
    reply_json(fd, 1001, "Bad request", NULL);
    return;
  }

  /* Map lệnh */
  if (strcmp(cmd, "create")==0) {
    int r = room_create(name);
    if (r==0)       reply_json(fd, 0,    "OK", NULL);
    else if (r==1)  reply_json(fd, 1003, "Already exists", NULL);
    else            reply_json(fd, 1100, "Internal error", NULL);
    return;
  }

  if (strcmp(cmd, "delete")==0) {
    int r = room_delete(name);
    if (r==0)       reply_json(fd, 0,    "OK", NULL);
    else            reply_json(fd, 1002, "Not found", NULL);
    return;
  }

  if (strcmp(cmd, "start")==0) {
    if (!room_exists(name)) { reply_json(fd, 1002, "Not found", NULL); }
    else { /* TODO start thực */ reply_json(fd, 0, "OK", NULL); }
    return;
  }

  if (strcmp(cmd, "stop")==0) {
    if (!room_exists(name)) { reply_json(fd, 1002, "Not found", NULL); }
    else { /* TODO stop thực */ reply_json(fd, 0, "OK", NULL); }
    return;
  }

  if (strcmp(cmd, "edit")==0) {
    if (!room_exists(name)) { reply_json(fd, 1002, "Not found", NULL); return; }
    (void)json_get_str(payload, "key", key, sizeof(key));
    (void)json_get_str(payload, "val", val, sizeof(val));
    /* TODO áp dụng key/val thật */
    reply_json(fd, 0, "OK", NULL);
    return;
  }

  if (strcmp(cmd, "show")==0) {
    if (!room_exists(name)) { reply_json(fd, 1002, "Not found", NULL); return; }
    /* TODO: trả dữ liệu thật; tạm trả mock */
    reply_json(fd, 0, "OK", "{\"room\":\"dummy\"}");
    return;
  }

  reply_json(fd, 1001, "Bad request", NULL);
}

static void process_one(work_t *w){
  /* Nhận diện protocol: nếu có "cmd" → CLI; ngược lại legacy */
  if (strstr(w->payload, "\"cmd\"")) {
    handle_cli(w->payload, w->peer_fd);
  } else {
    handle_legacy(w->payload, w->peer_fd);
  }

  free(w->payload);
  free(w);
}

/* ===================== Worker threads ===================== */
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
