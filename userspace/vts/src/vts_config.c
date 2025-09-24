#include "vts_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static int parse_text(const char *line, const char *tag, char *dst, size_t dstsz) {
// very small XML reader: expects <tag>value</tag> on one line
char open[64], close[64];
snprintf(open, sizeof(open), "<%s>", tag);
snprintf(close, sizeof(close), "</%s>", tag);
const char *p = strstr(line, open);
if (!p) return -1;
p += strlen(open);
const char *q = strstr(p, close);
if (!q) return -1;
size_t n = (size_t)(q - p);
if (n >= dstsz) n = dstsz - 1;
memcpy(dst, p, n); dst[n] = '\0';
return 0;
}
static int parse_int(const char *line, const char *tag, int *out) {
char buf[64];
if (parse_text(line, tag, buf, sizeof(buf))==0) { *out = atoi(buf); return 0; }
return -1;
}


static int parse_u16(const char *line, const char *tag, unsigned short *out) {
char buf[64];
if (parse_text(line, tag, buf, sizeof(buf))==0) { *out = (unsigned short)atoi(buf); return 0; }
return -1;
}
void vts_config_default(vts_config_t *c) {
memset(c, 0, sizeof(*c));
strncpy(c->bind_addr, "0.0.0.0", sizeof(c->bind_addr)-1);
c->port = 9000;
c->accept_backlog = 128;
c->io_timeout_ms = 3000;
c->client_pool_size = 8;
c->retry_max = 3;
c->retry_backoff_ms = 50;
c->dedup_ttl_ms = 60000;
strncpy(c->log_path, "logs/vts.log", sizeof(c->log_path)-1);
}
int vts_config_load(const char *path, vts_config_t *out) {
if (!out) return -2;
vts_config_default(out);
if (!path) return 0; // defaults only


FILE *f = fopen(path, "r");
if (!f) return -3; // ENOENT or permission


char line[512];
while (fgets(line, sizeof(line), f)) {
(void)parse_text(line, "bind_addr", out->bind_addr, sizeof(out->bind_addr));
(void)parse_u16(line, "port", &out->port);
(void)parse_int(line, "accept_backlog", &out->accept_backlog);
(void)parse_int(line, "io_timeout_ms", &out->io_timeout_ms);
(void)parse_int(line, "client_pool_size", &out->client_pool_size);
(void)parse_int(line, "retry_max", &out->retry_max);
(void)parse_int(line, "retry_backoff_ms", &out->retry_backoff_ms);
(void)parse_int(line, "dedup_ttl_ms", &out->dedup_ttl_ms);
(void)parse_text(line, "log_path", out->log_path, sizeof(out->log_path));
}
fclose(f);
return 0;
}