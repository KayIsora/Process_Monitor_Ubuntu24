
#pragma once
#include <stddef.h>
#include <pthread.h>
extern pthread_t g_thr_recv;
// Khởi động các thread nền (gửi log định kỳ từ FIFO đến vts.log
int vts_threads_start(const char *fifo_path, size_t fifo_cap);
void vts_threads_request_stop(void);
int vts_threads_join_all(int timeout_ms);
int vts_udp_open(int port);
int vts_udp_recv(int fd, void *buf, int cap);
int vts_tcp_listen(int port, int backlog);
int vts_tcp_accept(int listen_fd);
