#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" { 
#endif

typedef enum {
  ROOM_STOPPED = 0,
  ROOM_STARTED = 1
} room_state_t;

typedef struct {
  char id[64];
  char type[32];
  room_state_t state;
} room_t;

typedef struct {
  room_t *items;
  int count;
} room_list_t;

/* XML */
int loc_load_rooms_xml(const char *path, room_list_t *out);    // alloc items
void loc_free_rooms(room_list_t *rl);

/* ROOM STORE (in-memory desired state) */
int room_store_init(void);
void room_store_deinit(void);
int room_store_put(const room_t *r);          // CREATE/EDIT upsert
int room_store_del(const char *id);           // DELETE
int room_store_get(const char *id, room_t *out);
int room_store_all(room_list_t *out);

/* VTS adapter */
typedef enum {
  LOC_VIA_VTS = 0,
  LOC_VIA_LIBPM = 1
} show_mode_t;

int vts_apply_room(const room_t *r);          // reflect CREATE/EDIT + state
int vts_delete_room(const char *id);
int vts_show_room(const char *id, char *buf, size_t buflen); // SHOW via VTS

/* Memdata direct (libpm) */
int mem_show_snapshot(const char *room_type, char *buf, size_t buflen);

/* Utilities */
const char* room_state_str(room_state_t s);
room_state_t room_state_from_str(const char *s);

#ifdef __cplusplus
}
#endif
