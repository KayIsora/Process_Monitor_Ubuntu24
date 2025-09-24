#include <string.h>
#include <stdlib.h>
#include "room_store.h"

typedef struct node { room_t r; struct node *next; } node_t;
static node_t *g_head;

int room_store_init(void){ g_head=NULL; return 0; }
void room_store_deinit(void){ node_t *n=g_head; while(n){ node_t *t=n->next; free(n); n=t; } g_head=NULL; }

static node_t* find_node(const char *id){ for(node_t *n=g_head;n;n=n->next) if(strcmp(n->r.id,id)==0) return n; return NULL; }

int room_store_put(const room_t *r){
  node_t *n=find_node(r->id);
  if(!n){ n=calloc(1,sizeof(*n)); n->r=*r; n->next=g_head; g_head=n; }
  else n->r=*r;
  return 0;
}
int room_store_del(const char *id){
  node_t **pp=&g_head; for(node_t *n=g_head;n;n=n->next){
    if(strcmp(n->r.id,id)==0){ *pp=n->next; free(n); return 0; } pp=&n->next;
  } return -1;
}
int room_store_get(const char *id, room_t *out){
  node_t *n=find_node(id); if(!n) return -1; *out=n->r; return 0;
}
int room_store_all(room_list_t *out){
  int c=0; for(node_t *n=g_head;n;n=n->next) c++;
  room_t *arr=calloc(c?c:1,sizeof(room_t));
  int i=0; for(node_t *n=g_head;n;n=n->next) arr[i++]=n->r;
  out->items=arr; out->count=c; return 0;
}
