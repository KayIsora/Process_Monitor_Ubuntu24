#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/time.h>


// Error codes aligned with VTS
#define E_OK 0
#define E_BADREQ 1001
#define E_NOTFOUND 1002
#define E_EXISTS 1003
#define E_IO 1100
#define E_TIMEOUT 1101
static const char *USAGE =
"system-monitor create|delete|start|stop|edit|show|help [ARGS]\n"
"Examples:\n"
" system-monitor create room cpu_room\n"
" system-monitor show room cpu_room --via vts\n";


static void print_help(void){ puts(USAGE); }
static int send_req(const char *host, const char *port, const char *payload,
int retry_max, int backoff_ms, int timeout_ms, char *resp, size_t rspsz) {
int attempt=0; int last_err=E_IO;
while (attempt++ <= retry_max) {
int fd=-1; struct addrinfo hints={0}, *ai=NULL;
hints.ai_socktype = SOCK_STREAM; hints.ai_family=AF_UNSPEC;
if (getaddrinfo(host, port, &hints, &ai)!=0) { last_err=E_IO; goto retry; }
fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
if (fd<0) { last_err=E_IO; freeaddrinfo(ai); goto retry; }


struct timeval tv; 
tv.tv_sec = timeout_ms/1000; tv.tv_usec = (timeout_ms%1000)*1000;
setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));


if (connect(fd, ai->ai_addr, ai->ai_addrlen)<0) { last_err=E_IO; close(fd); freeaddrinfo(ai); goto retry; }
freeaddrinfo(ai);


uint32_t len = (uint32_t)strlen(payload);
uint32_t nlen = htonl(len);
if (write(fd, &nlen, 4)!=4 || write(fd, payload, len)!= (ssize_t)len) { last_err=E_IO; close(fd); goto retry; }


// read length-prefixed reply
uint32_t rlen=0; ssize_t n = read(fd, &rlen, 4);
if (n!=4) { last_err=E_TIMEOUT; close(fd); goto retry; }
rlen = ntohl(rlen);
if (rlen >= rspsz) rlen = (uint32_t)rspsz-1;
size_t off=0; while (off<rlen) {
ssize_t m = read(fd, resp+off, rlen-off);
if (m<=0) { last_err=E_TIMEOUT; break; }
off += (size_t)m;
}
close(fd);
if (off==rlen) { resp[rlen]='\0'; return E_OK; }
retry:
if (attempt<=retry_max) sleep(backoff_ms*1000);
}
return last_err;
}
static int build_payload(int argc, char **argv, char *out, size_t outsz) {
  if (argc < 2) return -1;
  const char *cmd = argv[1];

  // help: không cần payload
  if (!strcmp(cmd, "help")) return 1;

  // Mặc định yêu cầu target + name cho các lệnh dưới
  // create|delete|start|stop|show: cần >= 4 tham số: <cmd> <target> <name>
  // edit: cần >= 7 tham số: <cmd> <target> <name> --set <key> <val>
  if (!strcmp(cmd, "create") || !strcmp(cmd, "delete") ||
      !strcmp(cmd, "start")  || !strcmp(cmd, "stop")   ||
      !strcmp(cmd, "show")) {
    if (argc < 4) return -1; // thiếu target/name
    const char *target = argv[2];
    const char *name   = argv[3];
    snprintf(out, outsz,
      "{\"cmd\":\"%s\",\"target\":\"%s\",\"name\":\"%s\"}",
      cmd, target, name);
    return 0;
  }

  if (!strcmp(cmd, "edit")) {
    if (argc < 7) return -1; // thiếu --set key val
    const char *target = argv[2];
    const char *name   = argv[3];
    if (strcmp(argv[4], "--set") != 0) return -1;
    const char *key = argv[5];
    const char *val = argv[6];
    snprintf(out, outsz,
      "{\"cmd\":\"%s\",\"target\":\"%s\",\"name\":\"%s\",\"key\":\"%s\",\"val\":\"%s\"}",
      cmd, target, name, key, val);
    return 0;
  }

  // Lệnh không hỗ trợ
  return -1;
}
int main(int argc, char **argv) {
if (argc<2) { print_help(); return 2; }
if (!strcmp(argv[1], "help")) { print_help(); return 0; }


const char *host = getenv("VTS_HOST"); if (!host) host = "127.0.0.1";
const char *port = getenv("VTS_PORT"); if (!port) port = "9000";
int retry_max = getenv("VTS_RETRY_MAX")? atoi(getenv("VTS_RETRY_MAX")) : 3;
int backoff = getenv("VTS_BACKOFF_MS")? atoi(getenv("VTS_BACKOFF_MS")) : 50;
int timeout = getenv("VTS_TIMEOUT_MS")? atoi(getenv("VTS_TIMEOUT_MS")) : 3000;


char payload[512];
int r = build_payload(argc, argv, payload, sizeof(payload));
if (r==1) { print_help(); return 0; }
if (r<0) { fprintf(stderr, "ERROR %d: bad arguments.\n", E_BADREQ); print_help(); return 3; }


char resp[1024];
int rc = send_req(host, port, payload, retry_max, backoff, timeout, resp, sizeof(resp));
if (rc!=E_OK) {
fprintf(stderr, "ERROR %d: %s\n", rc, (rc==E_TIMEOUT?"timeout": "io"));
return 4;
}


// Expect JSON {code:int, msg:string, data?:string}
// For golden tests we just print whatever we got
puts(resp);
return 0;
}