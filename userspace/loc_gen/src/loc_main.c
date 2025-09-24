#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "loc_api.h"
#include "loc_gen.h"

static void usage(void){
  fprintf(stderr,
    "Usage:\n"
    "  locgend --config <file> --apply            # Đồng bộ toàn bộ rooms từ XML vào VTS (upsert + start/stop)\n"
    "  locgend show <room_id> [--via vts|libpm]   # SHOW trạng thái room qua VTS hoặc trực tiếp Memdata\n"
    "  locgend create|edit <id> <type> <started|stopped>\n"
    "  locgend delete <id>\n");
}

static int apply_from_xml(const char *path){
  room_list_t rl={0};
  if(loc_load_rooms_xml(path,&rl)!=0){ fprintf(stderr,"Load XML fail\n"); return 1; }
  room_store_init();
  for(int i=0;i<rl.count;i++){
    room_store_put(&rl.items[i]);
    if(vts_apply_room(&rl.items[i])!=0){
      fprintf(stderr,"VTS apply fail for %s\n", rl.items[i].id);
      loc_free_rooms(&rl); room_store_deinit(); return 2;
    }
  }
  loc_free_rooms(&rl);
  room_store_deinit();
  return 0;
}

int main(int argc, char **argv){
  if(argc<2){ usage(); return 1; }

  if(strcmp(argv[1],"--config")==0){
    if(argc>=4 && strcmp(argv[3],"--apply")==0){
      return apply_from_xml(argv[2]);
    }
    usage(); return 1;
  }

  if(strcmp(argv[1],"show")==0 && (argc==3 || argc==5)){
    const char *id=argv[2];
    show_mode_t mode=LOC_VIA_VTS;
    if(argc==5 && strcmp(argv[3],"--via")==0){
      mode = (strcmp(argv[4],"libpm")==0)?LOC_VIA_LIBPM:LOC_VIA_VTS;
    }
    char out[LOC_MAX_OUT];
    if(mode==LOC_VIA_VTS){
      if(vts_show_room(id,out,sizeof(out))!=0){ fprintf(stderr,"VTS show fail\n"); return 2; }
      printf("%s\n",out); return 0;
    } else {
      // Find type from store? Fallback: infer from id suffix
      char type[32]="inf";
      if(strstr(id,"cpu")) strcpy(type,"cpu");
      else if(strstr(id,"mem")) strcpy(type,"mem");
      if(mem_show_snapshot(type,out,sizeof(out))!=0){ fprintf(stderr,"Memdata show fail\n"); return 2; }
      printf("%s\n",out); return 0;
    }
  }

  if((strcmp(argv[1],"create")==0 || strcmp(argv[1],"edit")==0) && argc==5){
    room_t r; memset(&r,0,sizeof(r));
    strncpy(r.id,argv[2],sizeof(r.id)-1);
    strncpy(r.type,argv[3],sizeof(r.type)-1);
    r.state = room_state_from_str(argv[4]);
    if(vts_apply_room(&r)!=0){ fprintf(stderr,"apply fail\n"); return 2; }
    puts("OK"); return 0;
  }

  if(strcmp(argv[1],"delete")==0 && argc==3){
    if(vts_delete_room(argv[2])!=0){ fprintf(stderr,"delete fail\n"); return 2; }
    puts("OK"); return 0;
  }

  usage(); return 1;
}
