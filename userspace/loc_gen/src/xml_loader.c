#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "loc_api.h"

static void trim(char *s){
  size_t n=strlen(s);
  while(n && isspace((unsigned char)s[n-1])) s[--n]=0;
  size_t i=0; while(s[i] && isspace((unsigned char)s[i])) i++;
  if(i) memmove(s, s+i, strlen(s+i)+1);
}

room_state_t room_state_from_str(const char *s){
  if(!s) return ROOM_STOPPED;
  return (strcmp(s,"started")==0) ? ROOM_STARTED : ROOM_STOPPED;
}

const char* room_state_str(room_state_t s){ return s==ROOM_STARTED?"started":"stopped"; }

int loc_load_rooms_xml(const char *path, room_list_t *out){
  FILE *f=fopen(path,"r"); if(!f) return -1;
  char *buf=NULL; size_t cap=0, len=0; char line[1024];
  while(fgets(line,sizeof(line),f)){ size_t L=strlen(line);
    if(len+L+1>cap){ cap=(cap?cap*2:4096); buf=realloc(buf,cap);}
    memcpy(buf+len,line,L); len+=L;
  }
  fclose(f); if(!buf){ return -2; }
  buf[len]=0;

  // conservative parse: find <room ... />
  room_t *items=NULL; int cnt=0, allocd=0;
  char *p=buf;
  while((p=strstr(p,"<room"))){
    char *e=strchr(p,'>'); if(!e) break;
    char tag[512]={0}; size_t L=(size_t)(e-p+1); if(L>sizeof(tag)-1)L=sizeof(tag)-1;
    memcpy(tag,p,L); tag[L]=0;

    room_t r; memset(&r,0,sizeof(r));
    // id
    char *id=strstr(tag,"id=\""); 
    if(id){ id+=4; char *q=strchr(id,'\"'); if(q){ size_t n=(size_t)(q-id); if(n>63)n=63; memcpy(r.id,id,n); r.id[n]=0; } }
    // type
    char *ty=strstr(tag,"type=\""); 
    if(ty){ ty+=6; char *q=strchr(ty,'\"'); if(q){ size_t n=(size_t)(q-ty); if(n>31)n=31; memcpy(r.type,ty,n); r.type[n]=0; } }
    // state
    char *st=strstr(tag,"state=\"");
    if(st){ st+=7; char *q=strchr(st,'\"'); if(q){ char tmp[32]={0}; size_t n=(size_t)(q-st); if(n>31)n=31; memcpy(tmp,st,n); r.state=room_state_from_str(tmp);} }
    if(r.id[0]==0||r.type[0]==0) { p=e+1; continue; }

    if(cnt==allocd){ allocd=allocd?allocd*2:8; items=realloc(items,allocd*sizeof(room_t)); }
    items[cnt++]=r;

    p=e+1;
  }
  free(buf);
  out->items=items; out->count=cnt;
  return 0;
}

void loc_free_rooms(room_list_t *rl){
  if(!rl) return;
  free(rl->items); rl->items=NULL; rl->count=0;
}
