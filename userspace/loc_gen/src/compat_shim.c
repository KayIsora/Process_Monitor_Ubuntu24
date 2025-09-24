#include <stdio.h>
#include <string.h>
#include "loc_api.h"

// pmcli mong đợi loc_read_rooms(...) đổ chuỗi mô tả rooms vào buffer out.
// Ta load XML rồi serialize dạng text đơn giản: "id=<id> type=<type> state=<state>\n"
int loc_read_rooms(const char *path, char *out, size_t outlen) {
  if (!out || outlen == 0) return -1;
  out[0] = '\0';

  room_list_t rl = {0};
  if (loc_load_rooms_xml(path, &rl) != 0) {
    return -1;
  }

  size_t off = 0;
  for (int i = 0; i < rl.count; i++) {
    const room_t *r = &rl.items[i];
    int wrote = snprintf(out + off, (off < outlen ? outlen - off : 0),
                         "id=%s type=%s state=%s\n",
                         r->id, r->type, room_state_str(r->state));
    if (wrote < 0) { loc_free_rooms(&rl); return -1; }
    if ((size_t)wrote >= (outlen - off)) { // hết chỗ -> cắt bớt, nhưng coi như OK
      off = outlen - 1;
      out[off] = '\0';
      break;
    }
    off += (size_t)wrote;
  }

  loc_free_rooms(&rl);
  return 0;
}
