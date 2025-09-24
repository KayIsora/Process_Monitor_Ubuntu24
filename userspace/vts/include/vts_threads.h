
#pragma once
#include <stddef.h>
// Khởi động các thread nền (gửi log định kỳ từ FIFO đến vts.log
int vts_threads_start(const char *fifo_path, size_t fifo_cap);
void vts_threads_request_stop(void);
int vts_threads_join_all(int timeout_ms);

