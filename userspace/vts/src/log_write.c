 #include "vts_log.h"
 #include <stdio.h>
 #include <stdarg.h>
 #include <sys/stat.h>
 #include <unistd.h>
#include <time.h>
#include <pthread.h>

 static const char *LOG0="logs/vts.log";
 static const char *LOG1="logs/vts.log.1";
 static const char *LOG2="logs/vts.log.2";
 static size_t MAXB = 10*1024*1024; // 10MB default (override via config if muốn)
static FILE *g_logf = NULL;
static pthread_mutex_t g_log_lock = PTHREAD_MUTEX_INITIALIZER;

 static FILE *fp=NULL;
 void vts_log_set_maxbytes(size_t b){ MAXB=b; }

 int vts_log_write_line(const char *line){
  if(!line) line="";
  vts_log_write("%s\n", line);
  return 0;
 }

 static void rotate(){
   // log2 <- log1 <- log0
   rename(LOG1, LOG2);
   rename(LOG0, LOG1);
 }

 static void ensure_open(){
   if(!fp){ fp = fopen(LOG0, "a"); }
 }

 void vts_log_write(const char *fmt, ...){
   ensure_open();
   va_list ap; va_start(ap, fmt);
   vfprintf(fp, fmt, ap);
   va_end(ap);
   fflush(fp);
   struct stat st;
   if(fstat(fileno(fp), &st)==0 && (size_t)st.st_size > MAXB){
     fclose(fp); fp=NULL;
     rotate();
     ensure_open();
   }
 }
static void ensure_log_open(void){
    if (g_logf) return;
    g_logf = fopen("logs/vts.log", "a");
    if (g_logf) setvbuf(g_logf, NULL, _IOLBF, 0); // line-buffered
}

void log_write_req_result(const char *req_id, int success) {
    pthread_mutex_lock(&g_log_lock);
    ensure_log_open();
    if (g_logf) {
        time_t t = time(NULL); struct tm tm; localtime_r(&t, &tm);
        char ts[32]; strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", &tm);
        fprintf(g_logf, "%s req_id=%s result=%s\n", ts, req_id?req_id:"-", success?"OK":"ERR");
        // fflush(g_logf); // line-buffer đã đủ; gọi thêm nếu muốn chắc ăn
    }
    pthread_mutex_unlock(&g_log_lock);
}