#ifndef VTS_PROTOCOL_H
#define VTS_PROTOCOL_H

#include <stdint.h>
#include <sys/types.h>
#include <arpa/inet.h>   // <-- THÊM DÒNG NÀY

/* Frame: 4-byte BE length, then payload bytes... */

static inline uint32_t be32(uint32_t x){ return htonl(x); }
static inline uint32_t le32_to_host(uint32_t x){ return ntohl(x); }

#define VTS_MAX_FRAME (1<<20)

#endif
