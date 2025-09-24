// userspace/loc_gen/src/room_mem_reader.c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "loc_api.h"
#include "libpm.h"
#include "pm_nodes.h"
#include <stdlib.h>
static const char* get_proc_root(void){
  // 1) nếu /proc/pm tồn tại → dùng layout mới
  if (access("/proc/pm", R_OK) == 0) return "/proc/pm";
  // 2) nếu set env PM_PROC_ROOT → ưu tiên
  const char *env = getenv("PM_PROC_ROOT");
  if (env && access(env, R_OK) == 0) return env;
  // 3) fallback layout cũ pha 1b
  if (access("/proc/pm_drv", R_OK) == 0) return "/proc/pm_drv";
  // 4) cuối cùng, trả về layout mới (sẽ fail, nhưng rõ ràng)
  return "/proc/pm";
}

static int try_read_file(const char *path, char *buf, size_t buflen){
  int fd = open(path, O_RDONLY);
  if (fd < 0) return -1;
  ssize_t n = read(fd, buf, buflen - 1);
  close(fd);
  if (n <= 0) return -1;
  buf[n] = '\0';
  return 0;
}

static pm_node_t map_room_type_to_node(const char *room_type){
  if(strcmp(room_type,"cpu")==0) return PM_NODE_CPU;
  if(strcmp(room_type,"mem")==0) return PM_NODE_MEM;
  if(strcmp(room_type,"inf")==0) return PM_NODE_INF;
  return PM_NODE_INF;
}

int mem_show_snapshot(const char *room_type, char *buf, size_t buflen){
  // A) thử libpm (đọc "status")
  pm_node_t node = map_room_type_to_node(room_type);
  if (pm_read_status(node, buf, buflen) == PM_OK) return 0;

  // B) đọc trực tiếp procfs theo nhiều phương án
  const char *root = get_proc_root();
  char path[128];

  // layout mới có per-type + data/status
  snprintf(path, sizeof(path), "%s/%s/data", root, room_type);
  if (access(path, R_OK)==0 && try_read_file(path, buf, buflen)==0) return 0;

  snprintf(path, sizeof(path), "%s/%s/status", root, room_type);
  if (access(path, R_OK)==0 && try_read_file(path, buf, buflen)==0) return 0;

  // layout cũ: chỉ có /proc/pm_drv/{data?,status}
  snprintf(path, sizeof(path), "%s/data", root);
  if (access(path, R_OK)==0 && try_read_file(path, buf, buflen)==0) return 0;

  snprintf(path, sizeof(path), "%s/status", root);
  if (access(path, R_OK)==0 && try_read_file(path, buf, buflen)==0) return 0;

  return -1;
}
