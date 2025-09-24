#include "vts_fifo.h"
#include <stdlib.h>
#include <string.h>

static size_t next_idx(size_t i, size_t cap){
  return (i + 1 == cap) ? 0 : (i + 1);
}

 int vts_fifo_init(vts_fifo_t *q, size_t cap){
   q->cap = cap;
   atomic_store(&q->head, 0);
   atomic_store(&q->tail, 0);
   q->buf = (char**)calloc(cap, sizeof(char*));
   if(!q->buf) return -1;
   vts_fifo_metrics_t z = {0};
   atomic_store(&q->m, z);
   return 0;
 }

 void vts_fifo_free(vts_fifo_t *q){
   if(!q || !q->buf) return;
   size_t h = atomic_load(&q->head), t = atomic_load(&q->tail);
   while(h != t){
     free(q->buf[h]); q->buf[h]=NULL;
     h = next_idx(h, q->cap);
   }
   free(q->buf); q->buf=NULL;
 }

 size_t vts_fifo_depth(vts_fifo_t *q){
   size_t h = atomic_load(&q->head), t = atomic_load(&q->tail);
   return (t >= h) ? (t - h) : (q->cap - (h - t));
 }

 static void metrics_update_depth(vts_fifo_t *q){
   vts_fifo_metrics_t m = atomic_load(&q->m);
   size_t d = vts_fifo_depth(q);
   if(d > m.max_depth) { m.max_depth = d; atomic_store(&q->m, m); }
 }

 int vts_fifo_push_spsc(vts_fifo_t *q, const char *msg){
   size_t tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
   size_t head = atomic_load_explicit(&q->head, memory_order_acquire);
   size_t nxt  = next_idx(tail, q->cap);
   vts_fifo_metrics_t m = atomic_load(&q->m);
   int dropped = 0;
   if(nxt == head){
     // FULL -> drop-oldest: advance head and free oldest
     free(q->buf[head]); q->buf[head]=NULL;
     atomic_store_explicit(&q->head, next_idx(head, q->cap), memory_order_release);
     m.drops;
     dropped = 1;
   }
   char *dup = strdup(msg ? msg : "");
   q->buf[tail] = dup;
   atomic_store_explicit(&q->tail, nxt, memory_order_release);
   m.pushes;
   atomic_store(&q->m, m);
   metrics_update_depth(q);
   return dropped ? 0 : 1;
 }

 char* vts_fifo_pop_spsc(vts_fifo_t *q){
   size_t head = atomic_load_explicit(&q->head, memory_order_relaxed);
   size_t tail = atomic_load_explicit(&q->tail, memory_order_acquire);
   if(head == tail) return NULL;
   char *out = q->buf[head]; q->buf[head]=NULL;
   atomic_store_explicit(&q->head, next_idx(head, q->cap), memory_order_release);
   vts_fifo_metrics_t m = atomic_load(&q->m); m.pops; atomic_store(&q->m, m);
   return out;
 }

 vts_fifo_metrics_t vts_fifo_metrics(vts_fifo_t *q){
   return atomic_load(&q->m);
 }
