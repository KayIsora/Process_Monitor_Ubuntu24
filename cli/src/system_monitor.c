#include <stdio.h>
#include <string.h>
#include "loc_api.h"
#include "vts_log.h"

int main(int argc, char** argv){
  const char* root="/tmp/fake_procfs";
  if (argc>=3 && strcmp(argv[1],"scan")==0 && strcmp(argv[2],"--root")==0 && argc>=4){
    root = argv[3];
  } else if (argc>=2 && strcmp(argv[1],"scan")==0){
    // use default
  } else {
    fprintf(stderr,"usage: pmcli scan --root <fake_procfs_root>\n");
    return 2;
  }

  char snap[1024]={0};
  if (loc_read_rooms(root, snap, sizeof(snap))!=0){
    fprintf(stderr,"[err] failed to build snapshot\n"); return 1;
  }
  fputs(snap, stdout);
  if (vts_log_write_line(snap)!=0){
    fprintf(stderr,"[warn] failed to write vts.log\n");
  }
  return 0;
}
