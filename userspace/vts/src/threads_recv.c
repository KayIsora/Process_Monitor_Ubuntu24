 #include "vts_fifo.h"
 #include <pthread.h>
 #include <fcntl.h>
 #include <sys/epoll.h>
 #include <unistd.h>
 #include <stdatomic.h>
 #include <stdio.h>
 #include <string.h>
 #include <errno.h>
#include <sys/stat.h>  // mkfifo

 extern vts_fifo_t g_q;
 extern _Atomic int g_stop;

 static int open_fifo_ro(const char *p){
   mkfifo(p, 0666);
   int fd = open(p, O_RDONLY | O_NONBLOCK);
   return fd;
 }

 static void handle_line(char *line){
   // one message per line
   size_t len = strlen(line);
   if(len && (line[len-1]=='\n')) line[len-1]=0;
   vts_fifo_push_spsc(&g_q, line);
 }

 static void *thr_recv(void *arg){
   const char *path = (const char*)arg;
   int fd = open_fifo_ro(path);
   if(fd < 0){ perror("open fifo"); return NULL; }

   int ep = epoll_create1(0);
   struct epoll_event ev = {.events=EPOLLIN, .data.fd=fd};
   epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev);

   char buf[65536];
   char line[4096]; size_t lpos=0;
   while(!atomic_load(&g_stop)){
     struct epoll_event out;
     int n = epoll_wait(ep, &out, 1, 100); // 100ms
     if(n<=0) continue;
     ssize_t r = read(fd, buf, sizeof(buf));
     if(r <= 0){
       if(errno==EAGAIN || errno==EINTR) continue;
       // writer closed -> reopen for new writers
       close(fd); fd = open_fifo_ro(path);
       continue;
     }
     for(ssize_t i=0;i<r;i){
       if(buf[i]=='\n' || lpos==sizeof(line)-1){
         line[lpos]=0; handle_line(line); lpos=0;
       } else {
         line[lpos]=buf[i];
       }
     }
   }
   close(fd); close(ep);
   return NULL;
 }

 pthread_t g_thr_recv;
 int vts_thread_spawn_recv(const char *fifo_path){
   return pthread_create(&g_thr_recv, NULL, thr_recv, (void*)fifo_path);
 }
