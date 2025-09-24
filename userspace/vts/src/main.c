#include "vts_fifo.h"
#include "vts_log.h"
#include "vts_threads.h"

#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

// ===== Globals (giữ như cũ) =====
vts_fifo_t g_q;
_Atomic int g_stop = 0;

// Threads sẵn có
extern int vts_thread_spawn_recv(const char *fifo_path);
extern int vts_thread_spawn_coords(void);
extern pthread_t g_thr_recv, g_thr_coords;

// ===== Networking server (from net_server.c) =====
extern int start_vts_networking(void);

// Networking thread handle
static pthread_t g_thr_net;

// ===== Signal handler =====
static void on_sig(int s) { (void)s; atomic_store(&g_stop, 1); }

// ===== VTS worker threads bootstrap =====
int vts_threads_start(const char *fifo_path, size_t fifo_cap) {
  if (vts_fifo_init(&g_q, fifo_cap) != 0) {
    fprintf(stderr, "fifo init fail\n");
    return -1;
  }
  if (vts_thread_spawn_recv(fifo_path) != 0) return -1;
  if (vts_thread_spawn_coords() != 0) return -1;
  return 0;
}

void vts_threads_request_stop(void) { atomic_store(&g_stop, 1); }

int vts_threads_join_all(int timeout_ms) {
  // simple join with timeout: poll exit flag for up to timeout_ms
  int waited = 0;
  while (waited < timeout_ms && !atomic_load(&g_stop)) { sleep(1000); waited += 1; }

  pthread_join(g_thr_coords, NULL);
  pthread_join(g_thr_recv, NULL);
  vts_fifo_free(&g_q);
  return 0;
}

// ===== Networking thread wrapper =====
static void *net_thread(void *arg) {
  (void)arg;
  // start_vts_networking() là vòng lặp epoll blocking
  // In ra 1 log ngắn để biết đã khởi động
  fprintf(stderr, "[vts] networking: starting TCP server on 0.0.0.0:9000\n");
  fflush(stderr);
  (void)start_vts_networking(); // nếu lỗi sẽ in từ bên trong
  return NULL;
}

// ===== CLI usage =====
static void usage() {
  fprintf(stderr, "vts -f <fifo_path> [-c <cap>] [-m <max_log_mb>]\n");
}

// ===== Main =====
int main(int argc, char **argv) {
  const char *fifo_path = "logs/vts_input.fifo";
  size_t cap = 65536;
  size_t max_mb = 10;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-f") && i + 1 < argc)      fifo_path = argv[++i];
    else if (!strcmp(argv[i], "-c") && i + 1 < argc) cap = (size_t)atol(argv[++i]);
    else if (!strcmp(argv[i], "-m") && i + 1 < argc) max_mb = (size_t)atol(argv[++i]);
    else { usage(); return 2; }
  }

  vts_log_set_maxbytes(max_mb * 1024 * 1024);
  signal(SIGINT, on_sig);
  signal(SIGTERM, on_sig);

  // Khởi động các thread VTS hiện có
  if (vts_threads_start(fifo_path, cap) != 0) return 1;

  // Khởi động server TCP trên thread riêng
  if (pthread_create(&g_thr_net, NULL, net_thread, NULL) != 0) {
    perror("pthread_create(net_thread)");
    return 1;
  }
  fprintf(stderr, "[vts] networking thread started\n");

  // Daemon loop: chờ tín hiệu dừng
  while (!atomic_load(&g_stop)) pause();

  // Shutdown
  vts_threads_request_stop();
  vts_threads_join_all(1000); // <1s

  // Net server hiện chưa có cơ chế stop mềm -> dùng cancel tạm
  pthread_cancel(g_thr_net);
  pthread_join(g_thr_net, NULL);

  // In metrics khi thoát
  vts_fifo_metrics_t m = vts_fifo_metrics(&g_q);
  fprintf(stderr,
          "vts_metrics pushes=%llu pops=%llu drops=%llu max_depth=%llu\n",
          (unsigned long long)m.pushes,
          (unsigned long long)m.pops,
          (unsigned long long)m.drops,
          (unsigned long long)m.max_depth);

  return 0;
}
