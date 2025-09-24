@@
 #pragma once
 #include <stdint.h>
#include <stdatomic.h>
 
 typedef struct {
   int tcp_listen_fd;
   int port;
 } vts_tcp_cfg_t;
 
typedef struct {
  int udp_fd;
  int port;
} vts_udp_cfg_t;

 typedef struct {
   int max_clients;
   int client_timeout_ms;
 } vts_net_limits_t;
 
 typedef struct {
   vts_tcp_cfg_t tcp;
  vts_udp_cfg_t udp;
   vts_net_limits_t limits;
 } vts_net_cfg_t;
 
 int vts_net_server_start(const vts_net_cfg_t *cfg);
 int vts_net_server_stop(void);

// Warm start dedup LRU across restarts
int vts_dedup_warm_load(const char *path);
int vts_dedup_warm_dump(const char *path);
