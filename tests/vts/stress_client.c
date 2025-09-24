// SPDX-License-Identifier: MIT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>     // errno, EINTR
#include <stdint.h>    // uint32_t
#include <sys/types.h> // ssize_t
#include <netinet/in.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>

static ssize_t write_full(int fd, const void *buf, size_t len) {
    const unsigned char *p = (const unsigned char *)buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, p + off, len - off);
        if (n > 0) { off += (size_t)n; continue; }
        if (n < 0 && errno == EINTR) continue;
        return -1;
    }
    return (ssize_t)off;
}

static ssize_t read_full(int fd, void *buf, size_t len) {
    unsigned char *p = (unsigned char *)buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = read(fd, p + off, len - off);
        if (n > 0) { off += (size_t)n; continue; }
        if (n < 0 && errno == EINTR) continue;
        return -1;
    }
    return (ssize_t)off;
}

typedef struct {
    const char *host;
    int port;
    int use_udp;           // 0 = TCP, 1 = UDP
    int req_per_thread;
    int thread_id;
} worker_cfg_t;

static uint64_t now_ms(void){
    struct timespec ts; timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec*1000ull + ts.tv_nsec/1000000ull;
}

static void *thr_fn_tcp(void *arg){
    worker_cfg_t *cfg = (worker_cfg_t*)arg;

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return NULL; }

    struct sockaddr_in addr; memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)cfg->port);
    if (inet_pton(AF_INET, cfg->host, &addr.sin_addr) != 1) {
        perror("inet_pton"); close(s); return NULL;
    }
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect"); close(s); return NULL;
    }
    struct timeval tv; tv.tv_sec = 2; tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char payload[256];
    for (int i = 0; i < cfg->req_per_thread; i++){
        uint64_t rid = now_ms();
        // idempotent-ish echo; server hiểu JSON là đủ
        int plen = snprintf(payload, sizeof(payload),
        "{\"req_id\":\"t%d-%d-%" PRIu64 "\",\"op\":\"PING\",\"body\":\"hello\"}",
        cfg->thread_id, i, rid);
        if (plen < 0) break;
        if (plen > (int)sizeof(payload)) plen = (int)sizeof(payload);

        uint32_t nlen = htonl((uint32_t)plen);
        if (write_full(s, &nlen, 4) != 4) break;
        if (write_full(s, payload, (size_t)plen) != plen) break;

        uint32_t rlen_be;
        if (read_full(s, &rlen_be, 4) != 4) break;
        uint32_t rlen = ntohl(rlen_be);
        if (rlen > (1024*1024)) { // sanity
            fprintf(stderr, "resp too big: %u\n", rlen);
            break;
        }
        char *buf = (char*)malloc(rlen+1);
        if (!buf) break;
        if (read_full(s, buf, rlen) != (ssize_t)rlen) { free(buf); break; }
        buf[rlen] = 0;
        // tắt printf để nhanh
        free(buf);
    }

    close(s);
    return NULL;
}

static void *thr_fn_udp(void *arg){
    worker_cfg_t *cfg = (worker_cfg_t*)arg;

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) { perror("socket"); return NULL; }

    struct sockaddr_in addr; memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)cfg->port);
    if (inet_pton(AF_INET, cfg->host, &addr.sin_addr) != 1) {
        perror("inet_pton"); close(s); return NULL;
    }

    char payload[256];
    for (int i = 0; i < cfg->req_per_thread; i++){
        uint64_t rid = now_ms();
        int plen = snprintf(payload, sizeof(payload),
                 "{\"req_id\":\"t%d-%d-%" PRIu64 "\",\"op\":\"GET\",\"via\":\"udp\"}",
                 cfg->thread_id, i, rid);
        if (plen < 0) break;
        if (sendto(s, payload, (size_t)plen, 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            // UDP fire-and-forget: chỉ log lỗi, tiếp tục
            // perror("sendto");
        }
        // không chờ phản hồi để đạt throughput tối đa
    }

    close(s);
    return NULL;
}

static void usage(const char *prog){
    fprintf(stderr,
      "Usage: %s [-H host] [-p port] [-n req_per_thread] [-c threads] [-u]\n"
      "  -H host (default 127.0.0.1)\n"
      "  -p port (default 9000 TCP; nếu -u thì là UDP port, mặc định 9001)\n"
      "  -n requests per thread (default 2000)\n"
      "  -c threads (default 8)\n"
      "  -u use UDP (default TCP)\n", prog);
}

int main(int argc, char **argv){
    const char *host = "127.0.0.1";
    int port = 9000;
    int udp_port_default = 9001;
    int req_per_thread = 2000;
    int threads = 8;
    int use_udp = 0;

    int opt;
    while ((opt = getopt(argc, argv, "H:p:n:c:uh")) != -1){
        switch (opt){
            case 'H': host = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 'n': req_per_thread = atoi(optarg); break;
            case 'c': threads = atoi(optarg); break;
            case 'u': use_udp = 1; break;
            case 'h': default: usage(argv[0]); return 2;
        }
    }
    if (use_udp && port == 9000) port = udp_port_default; // nếu user không set -p khi -u

    pthread_t *t = (pthread_t*)calloc((size_t)threads, sizeof(pthread_t));
    worker_cfg_t cfg = {
        .host = host,
        .port = port,
        .use_udp = use_udp,
        .req_per_thread = req_per_thread,
        .thread_id = 0
    };

    for(int i=0;i<threads;i++){
        worker_cfg_t *c = (worker_cfg_t*)malloc(sizeof(worker_cfg_t));
        *c = cfg; c->thread_id = i;
        if(use_udp)
            pthread_create(&t[i], NULL, thr_fn_udp, c);
        else
            pthread_create(&t[i], NULL, thr_fn_tcp, c);
    }
    for(int i=0;i<threads;i++){
        pthread_join(t[i], NULL);
    }
    free(t);
    printf("done\n");
    return 0;
}
