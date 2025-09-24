#ifndef VTS_CONFIG_H
#define VTS_CONFIG_H
#include <stdint.h>
#include <stddef.h>


#define VTS_CFG_MAX_PATH 256


typedef struct {
char bind_addr[64]; // default: "0.0.0.0"
uint16_t port; // default: 9000
unsigned short udp_port;            // <--- thêm
int accept_backlog; // default: 128
int io_timeout_ms; // default: 3000 (recv/send)
int client_pool_size; // default: 8
int retry_max; // default: 3
int retry_backoff_ms; // default: 50
int dedup_ttl_ms; // default: 60000
char log_path[VTS_CFG_MAX_PATH]; // default: "logs/vts.log"
char dedup_warm_file[256];          // <--- thêm
} vts_config_t;


// Load config from XML path. Returns 0 on success, <0 on error.
int vts_config_load(const char *xml_path, vts_config_t *out);


// Populate with defaults.
void vts_config_default(vts_config_t *out);


#endif // VTS_CONFIG_H