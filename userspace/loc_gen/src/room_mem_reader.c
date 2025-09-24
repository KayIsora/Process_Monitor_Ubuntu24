#include <stdio.h>
#include <string.h>
#include "libpm.h"
#include "parser.h"
#include "loc_api.h"

int loc_read_rooms(const char* root, char* out, unsigned out_sz){
  char path[512], buf[1024], v[128];
  //pm_io_policy_t pol = {.retries=3,.initial_backoff_ms=5,.max_backoff_ms=50,.timeout_ms=200};

  int off=0;
  snprintf(path,sizeof(path),"%s/Memdata/cpu/data",root);
  if (pm_read_status(PM_NODE_MEM, buf, sizeof(buf))==PM_OK &&
      pm_parse_kv(buf,"cpu.usage",v,sizeof(v))==0){
    off += snprintf(out+off, out_sz-off, "cpu.usage=%s\n", v);
  }
  snprintf(path,sizeof(path),"%s/Memdata/mem/data",root);
  if (pm_read_status(PM_NODE_MEM, buf, sizeof(buf))==PM_OK &&
      pm_parse_kv(buf,"mem.free",v,sizeof(v))==0){
    off += snprintf(out+off, out_sz-off, "mem.free=%s\n", v);
  }
  return 0;
}
int mem_show_snapshot(const char *room_type, char *buf, size_t buflen){
  // Map room_type -> procfs node (the contract is in docs/procfs_contract.md)
  const char *node = NULL;
  if(strcmp(room_type,"cpu")==0) node="/proc/pm/cpu/data";
  else if(strcmp(room_type,"mem")==0) node="/proc/pm/mem/data";
  else if(strcmp(room_type,"inf")==0) node="/proc/pm/inf/data";
  else node="/proc/pm/inf/data";

  // Use libpm convenience: pm_read_status(path, out, outlen) already has backoff/timeout
  int rc = pm_read_status(node, buf, buflen); // returns 0 on success
  if(rc!=0) return -1;

  // Optionally validate simple KV presence
  // Example expected keys: "ts", "cpu.usage", "mem.used" depending on type
  return 0;
}