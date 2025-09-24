#ifndef REQ_HANDLER_H
#define REQ_HANDLER_H
#include <stdint.h>

int  req_handler_start(void);
void req_queue_push(uint8_t *payload, uint32_t len, int peer_fd);

#endif
