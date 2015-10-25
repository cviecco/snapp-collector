

#include "config.h"
#include "util.h"
#include "snapp_control.hpp"
#include "rrd_writer.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <pcre.h>
#include <assert.h>

#define CONTROL_BUF_SIZE 1024

//to syncronize with the pcre handler...
extern pthread_rwlock_t   pcre_log_lock ;
extern pcre *log_re;
extern pcre_extra *log_re_extra;

////////////
///static initializers
int Snapp_control::listen_port=9966;

////
void *Snapp_control_thread_func(void *in_class){
              Snapp_control *local_class;
              local_class=(Snapp_control *)in_class;
              local_class->thread_start();
             return NULL;
}
void *Snapp_control_worker_thread_func(void *in_class){
              Snapp_control_worker *local_class;
              local_class=(Snapp_control_worker *)in_class;
              local_class->thread_start();
             return NULL;
}


////////////////////////////

int Snapp_control_worker::send_help(){
   char send_buf[CONTROL_BUF_SIZE];
   int rvalue;
   int fvalue=0;

   snprintf(send_buf,CONTROL_BUF_SIZE-1,"Commands are: \r\n\r\n");
   rvalue=send(fd,send_buf,strlen(send_buf),0);
   if(rvalue!=strlen(send_buf)){
      fvalue=-1;
      goto done;
   }
   snprintf(send_buf,CONTROL_BUF_SIZE-1,"help  logregexp  pass    quit   reload    status   user  \r\n");
   rvalue=send(fd,send_buf,strlen(send_buf),0);
   if(rvalue!=strlen(send_buf)){
      fvalue=-1;
      goto done;
   }
done:
   return fvalue;
};

////////////////////////////////
int Snapp_control_worker::do_reload(){
   char send_buf[CONTROL_BUF_SIZE];
   int rvalue;
   int fvalue=0;
   int i;
   time_t begin_time,end_time;
   const int wait_secs=55;
   Snapp_config *current_config, *new_config;
   
   rvalue=pthread_mutex_lock(parent->config_mutex);
   assert(0==rvalue);
   current_config=(*(parent->current_config));
   rvalue=pthread_mutex_unlock(parent->config_mutex);
   assert(0==rvalue);
   
   new_config=current_config;
   begin_time=time(NULL);
   pthread_kill(parent->signal_thread,SIGHUP);
   for(i=0;i<wait_secs && current_config==new_config;i++){
       sleep(1);
       rvalue=pthread_mutex_lock(parent->config_mutex);
       assert(0==rvalue);
       new_config=(*(parent->current_config));
       rvalue=pthread_mutex_unlock(parent->config_mutex);
       assert(0==rvalue);

   }
   end_time=time(NULL);
   if(new_config==current_config){
       snprintf(send_buf,CONTROL_BUF_SIZE-1,"550 Reload failed afer %d secs\r\n",wait_secs);
   }else{
       snprintf(send_buf,CONTROL_BUF_SIZE-1,"200 Reload sucessful (%d secs for reload)\r\n",(int) (end_time-begin_time));
   }
   rvalue=send(fd,send_buf,strlen(send_buf),0);


done:
   return fvalue;
}

///////////////////////////////////////////////
int Snapp_control_worker::do_regexp(char *arg){
   int rvalue;
   const char *error;
   int erroffset;
   int cleared_reg_exp=0;
   
   rvalue= pthread_rwlock_wrlock(&pcre_log_lock);
   assert(0==rvalue);
   ///WARN CANNOT CALL print_log ith the write lock!!!
   if(log_re!=NULL){
      pcre_free(log_re);
      log_re=NULL;
   }
   if(log_re_extra!=NULL){
      pcre_free(log_re_extra);
      log_re_extra=NULL;
   }
   #if defined _GNU_SOURCE || _POSIX_C_SOURCE >= 200809L 
   if (strnlen(arg,2)>=2){
   #else
   if ( strlen(arg) >=2){
   #endif
      log_re = pcre_compile(
               arg,              // the pattern 
               0,                // default options 
               &error,           // for error message 
               &erroffset,       // for error offset 
               NULL);            // use default character tables 

       if(log_re!=NULL){
         log_re_extra=pcre_study(log_re,0,&error);  
       } 
   }
   else{
       cleared_reg_exp=1;
   }   

   rvalue= pthread_rwlock_unlock(&pcre_log_lock);

   assert(0==rvalue);
   if(cleared_reg_exp==0){
     print_log(LOG_NOTICE,"After setting logregexp\n");
   }
   else{
     print_log(LOG_NOTICE,"regexp disabled\n");
   }

   return 0;
}

///////////////////////////////////////////////
int Snapp_control_worker::send_status(){
   char send_buf[CONTROL_BUF_SIZE];
   int rvalue;
   int fvalue=0;
   int i;
   int j;
   int num_hosts, num_active_collection_classes,num_active_hosts;
   int num_collections=0;
   struct Writer_status writer_status;

   memset(&writer_status,0x00,sizeof(struct Writer_status));
   

   rvalue=pthread_mutex_lock(parent->config_mutex);
   assert(0==rvalue);
   num_active_collection_classes=(*(parent->current_config))->collection_class.size();
   num_hosts=(*(parent->current_config))->host_table.host.size();

   for(i=0;i<(*(parent->current_config))->collection_class.size();i++){
      for(j=0;j<(*(parent->current_config))->collection_class[i].host.size();j++){
          num_collections+=(*(parent->current_config))->collection_class[i].host[j].collection.size();
      }
   }   

   rvalue=pthread_mutex_unlock(parent->config_mutex);
   assert(0==rvalue);

   get_writer_status(&writer_status); //should check for errors here too



////////////////start of status sending
   //snprintf(send_buf,CONTROL_BUF_SIZE-1,"Number of running collection classes: %d\r\n",(*(parent->current_config))->collection_class.size());
   snprintf(send_buf,CONTROL_BUF_SIZE-1,"211 System Status\r\n");

   rvalue=send(fd,send_buf,strlen(send_buf),0);
   if(rvalue!=strlen(send_buf)){
      return -1;
   }
   snprintf(send_buf,CONTROL_BUF_SIZE-1,"Number of running collection classes: %d\r\n",num_active_collection_classes);

   rvalue=send(fd,send_buf,strlen(send_buf),0);
   if(rvalue!=strlen(send_buf)){
      return -1;
   }
   //snprintf(send_buf,CONTROL_BUF_SIZE-1,"Total number of collection hosts: %d\r\n",(*(parent->current_config))->host_table.host.size());
   snprintf(send_buf,CONTROL_BUF_SIZE-1,"Total number of collection hosts: %d\r\n",num_hosts);
   rvalue=send(fd,send_buf,strlen(send_buf),0);
   if(rvalue!=strlen(send_buf)){
      return -1; 
   }
   
   snprintf(send_buf,CONTROL_BUF_SIZE-1,"Total number of collections: %d\r\n",num_collections);
   rvalue=send(fd,send_buf,strlen(send_buf),0);
   if(rvalue!=strlen(send_buf)){
      return -1;
   }

   snprintf(send_buf,CONTROL_BUF_SIZE-1,"avg queue len 1 min: %lu\r\n",writer_status.avg_queue_len_1min);
   rvalue=send(fd,send_buf,strlen(send_buf),0);
   if(rvalue!=strlen(send_buf)){
      return -1;
   }

   snprintf(send_buf,CONTROL_BUF_SIZE-1,"avg queue len 5 min: %lu\r\n",writer_status.avg_queue_len_5min);
   rvalue=send(fd,send_buf,strlen(send_buf),0);
   if(rvalue!=strlen(send_buf)){
      return -1;
   }


/////////end of status sending!
done:
   //rvalue=pthread_mutex_unlock(parent->config_mutex);
   //assert(0==rvalue);
   return fvalue;


}

int Snapp_control_worker::thread_start(){
   //simple server loop
   int rvalue;

   assert(parent!=NULL);
   print_log(LOG_DEBUG,"Control staterd worker thread\n");
   while(1==1){
      sem_wait(&parent->pending_semaphore);
      //lock shared, pop queue
      print_log(LOG_DEBUG,"Control worker thread, woke up\n");
      rvalue=pthread_mutex_lock(&parent->queue_lock);
      assert(0==rvalue);     
      fd=parent->pending_fd.front();
      parent->pending_fd.pop();      
      rvalue=pthread_mutex_unlock(&parent->queue_lock);
      assert(0==rvalue);
      print_log(LOG_DEBUG,"Control worker thread, new connectionfd=%d\n",fd);
      handle_connection();   
      close(fd);
   }

   return 0;
};

int Snapp_control_worker::handle_connection(){
   char recv_buf[CONTROL_BUF_SIZE];
   char send_buf[CONTROL_BUF_SIZE];
   int done=0;
   int rvalue;   
   int command;
   char *arg;
   int i;   


   //set conn state
   connection_state=CONTROL_CONNECTION_STATE_CONNECTED;

   //send hello!
   snprintf(send_buf,CONTROL_BUF_SIZE-1,"220 SNAPP server (%s) ready\r\n",VERSION);
   rvalue=send(fd,send_buf,strlen(send_buf),0);
   if(rvalue!=strlen(send_buf)){
      return 0;
   }
   while(0==done){
       //write snapp shell line:
       snprintf(send_buf,CONTROL_BUF_SIZE-1,"snapp> ");
       rvalue=send(fd,send_buf,strlen(send_buf),0);
       if(rvalue!=strlen(send_buf)){
           return 0;
       } 

       //read command
       command=-1;
       memset(recv_buf,0x00,CONTROL_BUF_SIZE);
       rvalue=recv(fd,recv_buf,CONTROL_BUF_SIZE-1,0);
       if(rvalue<=0){
          return 0;
       }
       //remove trailing chars(\n and \r)?
       for(i=0;i<rvalue;i++){
         if (recv_buf[i]=='\n' || recv_buf[i]=='\r' ){
            recv_buf[i]=0x00;
         }
       }    

       //do a 'switch like' on the command.
       if(recv_buf==strcasestr(recv_buf,"quit")){
         command=CONTROL_COMMAND_QUIT;
       }
       else if(0==strcasecmp(recv_buf,"reload")) {
         command=CONTROL_COMMAND_RELOAD;
       }
       else if(recv_buf==strcasestr(recv_buf,"status")){
         command=CONTROL_COMMAND_STATUS;
       }
       else if(recv_buf==strcasestr(recv_buf,"help")){
         command=CONTROL_COMMAND_HELP;
       }
       else if(recv_buf==strcasestr(recv_buf,"user ")){
         if(strlen(recv_buf)>=8){
           command=CONTROL_COMMAND_USER;
           arg=&recv_buf[5];   
         }
       }
       else if(recv_buf==strcasestr(recv_buf,"pass ")){
         if(strlen(recv_buf)>=8){
           command=CONTROL_COMMAND_PASS;
           arg=&recv_buf[5];
         }
       }
       else if(recv_buf==strcasestr(recv_buf,"logregexp ")){
         if(strlen(recv_buf)>=10){
           command=CONTROL_COMMAND_LOGREGEXP;
           arg=&recv_buf[10];
         }
       }
       // now actually do work
       switch(command <<8 |connection_state){
          case CONTROL_COMMAND_QUIT<<8 | CONTROL_CONNECTION_STATE_CONNECTED:
          case CONTROL_COMMAND_QUIT<<8 | CONTROL_CONNECTION_STATE_USERNAME_RECEIVED:
          case CONTROL_COMMAND_QUIT<<8 | CONTROL_CONNECTION_STATE_AUTHENTICATED:
              snprintf(send_buf,CONTROL_BUF_SIZE-1,"200 Goodbye\r\n");
              rvalue=send(fd,send_buf,strlen(send_buf),0);
              return 0;
              break;
          case CONTROL_COMMAND_USER<<8 | CONTROL_CONNECTION_STATE_CONNECTED:
              connection_state=CONTROL_CONNECTION_STATE_USERNAME_RECEIVED;
              snprintf(send_buf,CONTROL_BUF_SIZE-1,"331 Please specify the password\r\n");
              rvalue=send(fd,send_buf,strlen(send_buf),0);
              break;
          case CONTROL_COMMAND_PASS<<8 | CONTROL_CONNECTION_STATE_USERNAME_RECEIVED:
              if(0==parent->enable_password.compare(arg)){
                 connection_state=CONTROL_CONNECTION_STATE_AUTHENTICATED;
                 snprintf(send_buf,CONTROL_BUF_SIZE-1,"230 Login Sucessful\r\n");
                 rvalue=send(fd,send_buf,strlen(send_buf),0);
              }
              else{
                 snprintf(send_buf,CONTROL_BUF_SIZE-1,"530 Login Failure\r\n");
                 rvalue=send(fd,send_buf,strlen(send_buf),0);
              }
              break;
          
          case CONTROL_COMMAND_RELOAD<<8 | CONTROL_CONNECTION_STATE_AUTHENTICATED:
              rvalue=do_reload();
              if(rvalue<0){
                 return 0;
              }
              break;
          case CONTROL_COMMAND_LOGREGEXP<<8 | CONTROL_CONNECTION_STATE_AUTHENTICATED:
              rvalue=do_regexp(arg);
              if(rvalue<0){
                 return 0;
              }
              else{
                 //send failure!
              }

              break;

          case CONTROL_COMMAND_STATUS<<8 | CONTROL_CONNECTION_STATE_CONNECTED:
          case CONTROL_COMMAND_STATUS<<8 | CONTROL_CONNECTION_STATE_USERNAME_RECEIVED:
          case CONTROL_COMMAND_STATUS<<8 | CONTROL_CONNECTION_STATE_AUTHENTICATED:
              rvalue=send_status();
              if(rvalue<0){
                 return 0;
              }
              break;
          case CONTROL_COMMAND_HELP<<8 | CONTROL_CONNECTION_STATE_CONNECTED:
          case CONTROL_COMMAND_HELP<<8 | CONTROL_CONNECTION_STATE_USERNAME_RECEIVED:
          case CONTROL_COMMAND_HELP<<8 | CONTROL_CONNECTION_STATE_AUTHENTICATED:
              rvalue=send_help();
              if(rvalue<0){
                 return 0;
              }
              break;


          case CONTROL_COMMAND_RELOAD<<8 | CONTROL_CONNECTION_STATE_CONNECTED:
          case CONTROL_COMMAND_RELOAD<<8 | CONTROL_CONNECTION_STATE_USERNAME_RECEIVED:
              snprintf(send_buf,CONTROL_BUF_SIZE-1,"530 Not logged in\r\n");
              rvalue=send(fd,send_buf,strlen(send_buf),0);
              break;


          default:
              snprintf(send_buf,CONTROL_BUF_SIZE-1,"500 Command not recognized\r\n");
              rvalue=send(fd,send_buf,strlen(send_buf),0);
              if(rvalue!=strlen(send_buf)){
                 return 0;
              }

       }
       
    }

   
   return 0;
};


/////////////////
int Snapp_control::initialize_socket(){
   //init vals AND start threads
   struct sockaddr_in my_addr;
   struct in_addr  local_addr;
   const int BACKLOG=5;
   int yes=1;
   int i;
   int rvalue;
   pthread_attr_t attr;



   //////////////////////////
   //init server fd
   if ((bind_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
     perror("socket");
     exit(EXIT_FAILURE);
   }
   if (setsockopt(bind_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
   }


   my_addr.sin_family = AF_INET;         // host byte order
   my_addr.sin_port = htons(listen_port); // short, network byte order
   inet_aton("127.0.0.1",&local_addr);


   my_addr.sin_addr.s_addr = local_addr.s_addr; //INADDR_ANY; // automatically fill with my IP

   //my_addr.sin_addr.s_addr = global_conf.local_ip;
   memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct
   if (bind(bind_fd, (struct sockaddr *)&my_addr,
           sizeof(struct sockaddr)) == -1) {
     perror("bind");
     exit(1);
   }

   if (listen(bind_fd, BACKLOG) == -1) {
        print_log(LOG_ERR,"Failure, listen");
        
        exit(EXIT_FAILURE);
   }

}
int Snapp_control::initialize(){
   const int BACKLOG=5;
   int i;
   int rvalue;
   pthread_attr_t attr;


   //////////   
   //init sync primitives
   rvalue=sem_init(&pending_semaphore, 0, 0);
   assert(0==rvalue);
   rvalue=pthread_mutex_init(&queue_lock, NULL);
   assert(0==rvalue);

   //now create threads


   worker.resize(10);
   pthread_attr_init(&attr);
   rvalue = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   //rvalue = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   rvalue = pthread_attr_setstacksize(&attr, 256*1024);

   worker.resize(5);
   for(i=0;i<worker.size();i++){
       worker[i].parent=this;

   
       rvalue=pthread_create(&(worker[i].thread),
                             &attr,
                             Snapp_control_worker_thread_func,
                             (void *) &worker[i]);

       assert(0==rvalue);
  } 
  //now create the main thread.  
  rvalue = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  rvalue=pthread_create(&listen_thread,
                             &attr,
                             Snapp_control_thread_func,
                             (void *) this);

   assert(0==rvalue);
   return 0;
};



int Snapp_control::thread_start(){
   //simple server loop
   int new_fd;
   socklen_t sin_size;
   struct sockaddr_storage their_addr;
   int rvalue; 

   print_log(LOG_DEBUG,"Control started listen thread\n");
   while(1==1){
        sin_size = sizeof their_addr;
        new_fd = accept(bind_fd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
            continue;
        }
        //push fd to pending queue..
        print_log(LOG_DEBUG,"Control, GOT new conection! fd=%d\n",new_fd);
        rvalue=pthread_mutex_lock(&queue_lock);
        assert(0==rvalue);
        pending_fd.push(new_fd);
        rvalue=pthread_mutex_unlock(&queue_lock);
        assert(0==rvalue);
        rvalue=sem_post(&pending_semaphore);
        assert(0==rvalue);
        //print_log(LOG_DEBUG,"Control, semvalue=%d\n",pending_semaphore->);
   }

   return 0;
};


