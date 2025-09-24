#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>     // errno, EINTR
#include <stdint.h>    // uint32_t
#include <sys/types.h> // ssize_t (an toàn)

static ssize_t write_full(int fd, const void *buf, size_t len) {
    const unsigned char *p = buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, p + off, len - off);
        if (n > 0) { off += n; continue; }
        if (n < 0 && errno == EINTR) continue;
        return -1; // error
    }
    return (ssize_t)off;
}

static ssize_t read_full(int fd, void *buf, size_t len) {
    unsigned char *p = buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = read(fd, p + off, len - off);
        if (n > 0) { off += n; continue; }
        if (n < 0 && errno == EINTR) continue;
        return -1; // error
    }
    return (ssize_t)off;
}

#define REQ_PER_THREAD 2000
#define THREADS 8
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9000

static void *thr_fn(void *arg){
    int id = (intptr_t)arg;
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("connect"); return NULL; }

    for (int i = 0; i < REQ_PER_THREAD; i++){
        char payload[256];
        snprintf(payload, sizeof(payload),
                 "{\"req_id\":\"t%d-%d\",\"op\":\"WRITE\",\"body\":\"hello\"}", id, i);
        uint32_t len = htonl(strlen(payload));
        (void)write_full(s, &len, 4);
        (void)write_full(s, payload, strlen(payload));

        uint32_t rlen;
        if (read_full(s, &rlen, 4) != 4) break;
        rlen = ntohl(rlen);
        char *buf = malloc(rlen+1);
        if (read_full(s, buf, rlen) != (ssize_t)rlen) { free(buf); break; }
        buf[rlen] = 0;
        // (tắt printf để nhanh)
        free(buf);
    }

    close(s);
    return NULL;
}


int main(){
    pthread_t t[THREADS];
    for(int i=0;i<THREADS;i++){
        pthread_create(&t[i], NULL, thr_fn, (void*)(intptr_t)i);
    }
    for(int i=0;i<THREADS;i++) pthread_join(t[i], NULL);
    printf("done\n");
    return 0;
}
