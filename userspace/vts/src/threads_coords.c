 #include "vts_fifo.h"
 #include "vts_log.h"
 #include <pthread.h>
 #include <stdatomic.h>
 #include <stdio.h>
 #include <unistd.h>
 #include <stdlib.h>  // free
 
 extern vts_fifo_t g_q;
 extern _Atomic int g_stop;

 static void *thr_coords(void *arg){
   (void)arg;
   while(!atomic_load(&g_stop)){
     char *msg = vts_fifo_pop_spsc(&g_q);
     if(!msg){ struct timespec ts={0, 500000}; nanosleep(&ts,NULL); continue; } // 0.5ms
     vts_log_write("%s\n", msg);
     free(msg);
   }
   // drain quickly on shutdown (bounded)
   for(int i=0;i<1000;i){
     char *m = vts_fifo_pop_spsc(&g_q);
     if(!m) break; vts_log_write("%s\n", m); free(m);
   }
   return NULL;
 }

 pthread_t g_thr_coords;
 int vts_thread_spawn_coords(void){
   return pthread_create(&g_thr_coords, NULL, thr_coords, NULL);
 }
