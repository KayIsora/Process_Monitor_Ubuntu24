#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// Mã lỗi/return code đã chuẩn hoá
typedef enum {
  PM_OK = 0,
  PM_AGAIN = 11,      // thử lại (tương tự EAGAIN semantics)
  PM_TIMEOUT = 110,   // timeout
  PM_IOERR = 5,       // lỗi I/O chung (EIO)
} pm_rc;

// Loại node trong /proc/pm
typedef enum { PM_NODE_CPU, PM_NODE_MEM, PM_NODE_INF } pm_node_t;

// Kiểu file trong node
typedef enum { PM_FILE_CONFIG, PM_FILE_STATUS, PM_FILE_DATA } pm_file_t;

// Tham số cấu hình runtime cho libpm (có default)
typedef struct {
  const char* root;        // vd "/proc/pm" hoặc fake path từ test (NULL => "/proc/pm")
  int open_flags_extra;    // thêm O_CLOEXEC... (0 nếu không dùng)
  uint32_t timeout_ms;     // vd 200
  uint32_t backoff_init_us;// vd 200
  uint32_t backoff_max_us; // vd 20000
  uint32_t breaker_fail_n; // n lần lỗi liên tiếp => open circuit (vd 8)
  uint32_t breaker_half_ms;// thời gian half-open thử lại (vd 200)
} pm_cfg;

// Thống kê cơ bản cho tiêu chí Accept
typedef struct {
  uint64_t io_ok;
  uint64_t io_retry;
  uint64_t io_timeout;
  uint64_t io_err;
  uint64_t breaker_trips;
} pm_stats;

// Khởi tạo/thoát lib (stats reset)
void pm_init(const pm_cfg* opt_cfg);
void pm_get_stats(pm_stats* out);
void pm_reset_stats(void);

// Đọc STATUS vào buffer (text, kết thúc '\0' nếu đủ chỗ)
pm_rc pm_read_status(pm_node_t node, char* buf, size_t buflen);

// Đọc DATA (text/binary-safe; trả về số byte đọc qua out_nread)
pm_rc pm_read_data(pm_node_t node, uint8_t* buf, size_t buflen, size_t* out_nread);

// Gửi CONFIG (ví dụ "1 100\n" để bật polling 100ms, hoặc "0\n" để tắt)
pm_rc pm_set_config(pm_node_t node, const char* payload, uint32_t payload_len);

// Helper path mới (vd: "/proc/pm/mem/status"…)
const char* pm_node_str(pm_node_t n);

int pm_write_config(const char *node_path, const char *cmd);

#ifdef __cplusplus
}
#endif
