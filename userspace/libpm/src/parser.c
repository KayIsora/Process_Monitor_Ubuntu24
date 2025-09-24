#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"

static void trim(char* s){
  char* p=s; while(isspace((unsigned char)*p)) p++;
  size_t len=strlen(p);
  while(len>0 && isspace((unsigned char)p[len-1])) p[--len]='\0';
  if (p!=s) memmove(s,p,len+1);
}

int pm_parse_kv(const char* text, const char* key, char* val, size_t val_sz){
  const char* line = text;
  size_t klen = strlen(key);
  while (*line){
    const char* eol = strchr(line, '\n'); if (!eol) eol = line + strlen(line);
    if ((size_t)(eol - line) > klen+1 && strncmp(line, key, klen)==0 && line[klen]=='='){
      size_t copy = (size_t)(eol - (line + klen + 1));
      if (copy >= val_sz) copy = val_sz-1;
      memcpy(val, line + klen + 1, copy); val[copy]='\0';
      trim(val);
      return 0;
    }
    line = (*eol) ? eol+1 : eol;
  }
  return -1;
}
