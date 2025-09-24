 #pragma once
 #include <stdatomic.h>
 #include <stddef.h>
 #include <stdint.h>

 typedef struct {
   uint64_t pushes, pops, drops, max_depth;
 } vts_fifo_metrics_t;

 typedef struct {
   size_t cap;
   _Atomic size_t head;   // consumer reads from head
   _Atomic size_t tail;   // producer writes at tail
   char **buf;            // messages (null-terminated strings)
   _Atomic vts_fifo_metrics_t m;
 } vts_fifo_t;

 // Allocate ring of 'cap' slots. Returns 0 on success.
 int vts_fifo_init(vts_fifo_t *q, size_t cap);
 void vts_fifo_free(vts_fifo_t *q);

 // SPSC push: duplicates 'msg' into queue. Drop-oldest on full.
 // Returns 1 if enqueued, 0 if dropped (policy may still count drop even if replaced).
 int vts_fifo_push_spsc(vts_fifo_t *q, const char *msg);

 // SPSC pop: returns owned pointer (malloc) or NULL if empty.
 char* vts_fifo_pop_spsc(vts_fifo_t *q);

 // Snapshot metrics
 vts_fifo_metrics_t vts_fifo_metrics(vts_fifo_t *q);
 size_t vts_fifo_depth(vts_fifo_t *q);
