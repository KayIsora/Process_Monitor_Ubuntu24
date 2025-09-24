#pragma once
#include <stddef.h>

void vts_log_set_maxbytes(size_t bytes);
void vts_log_write(const char *fmt, ...);

// THÊM prototype wrapper:
int vts_log_write_line(const char *line);
void log_write_req_result(const char *req_id, int success);

