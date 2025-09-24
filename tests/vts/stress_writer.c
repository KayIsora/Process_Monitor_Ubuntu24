#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/uio.h>   // writev
#include <sys/time.h>
#include <sys/types.h>

#ifndef F_SETPIPE_SZ
#define F_SETPIPE_SZ (1024 + 7) // fallback, trên Linux giá trị thực đã có trong fcntl.h
#endif

static inline long nsec_per_rate(int rate){
  if (rate <= 0) return 0;
  long ns = 1000000000L / rate;
  if (ns < 1000) ns = 1000; // clamp ~1us
  return ns;
}

int main(int argc, char**argv){
  const char *fifo = (argc>1)?argv[1]:"logs/vts_input.fifo";
  int rate = (argc>2)?atoi(argv[2]):100000; // lines per second
  if(rate <= 0) rate = 100000;

  // Đảm bảo FIFO tồn tại
  mkfifo(fifo, 0666); // idempotent

  // Mở non-blocking để không bao giờ treo khi pipe đầy
  int fd = open(fifo, O_WRONLY | O_NONBLOCK);
  if (fd < 0) { perror("open fifo"); return 1; }

  // (Tùy chọn) Tăng dung lượng pipe để giảm tắc (có thể bị giới hạn bởi sysctl)
  (void)fcntl(fd, F_SETPIPE_SZ, 1<<20); // thử set ~1MB, bỏ qua lỗi nếu không được

  // Nhịp gửi gần đúng theo rate (không cần quá chính xác)
  struct timespec ts = {.tv_sec=0, .tv_nsec=nsec_per_rate(rate)};

  // Gửi theo batch để giảm syscall; chấp nhận drop nếu EAGAIN
  enum { BATCH = 64 };
  char   buf[BATCH][64];
  struct iovec iov[BATCH];

  unsigned long long seq = 0;
  time_t t0 = time(NULL);

  while (time(NULL) - t0 < 60) {
    int k = 0;
    for (; k < BATCH && time(NULL) - t0 < 60; ++k) {
      int n = snprintf(buf[k], sizeof(buf[k]), "msg seq=%llu ts=%ld\n",
                       seq++, (long)time(NULL));
      if (n < 0) n = 0;
      if (n > (int)sizeof(buf[k])) n = (int)sizeof(buf[k]);
      iov[k].iov_base = buf[k];
      iov[k].iov_len  = (size_t)n;
    }

    ssize_t w = writev(fd, iov, k);
    if (w < 0) {
      if (errno == EAGAIN) {
        // Pipe đầy → DROP batch này theo policy, không chờ
        nanosleep(&ts, NULL);
        continue;
      }
      perror("writev");
      break;
    }
    // Nhịp đều đều để không spam CPU
    nanosleep(&ts, NULL);
  }

  close(fd);
  return 0;
}
