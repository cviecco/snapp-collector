
#include "config.h"
#include "rrd_writer.h"
#include "rrd_writer.hpp"

#include "util.h"
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include <string.h>
#include <rrd.h>
#include <stdio.h>
#include <time.h>
#include <fnv.h>
#include <stdlib.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>

#include <fcntl.h>   //for the open flags



//c++
#include <map>
#include <queue>
#include <iostream>
#include <vector>
#include <string>
#include <limits>

///////////////
//For the load calculations, lifted from wikipedia
#define FSHIFT   11		/* nr of bits of precision */
#define FIXED_1  (1<<FSHIFT)	/* 1.0 as fixed-point */
#define LOAD_FREQ (5*HZ)	/* 5 sec intervals */
#define EXP_1  1884		/* 1/exp(5sec/1min) as fixed-point */
#define EXP_5  2014		/* 1/exp(5sec/5min) */
#define EXP_15 2037		/* 1/exp(5sec/15min) */
 
#define CALC_LOAD(load,exp,n) \
   load *= exp; \
   load += n*(FIXED_1-exp); \
   load >>= FSHIFT;


/////////



///////////////////////////////////////////////////////////////////////////////////
//static member initialization
int             RRD_Writer::initialized = 0;
pthread_mutex_t RRD_Writer::lock        = PTHREAD_MUTEX_INITIALIZER;

//the base class
RRD_Writer __base_rrd_writer;

////////////////////////////////////////////////////////

//ordered by filename id,time,template_id
/*
bool operator< (RRD_datum &left, RRD_datum &right)
{
   if (left.filename_id<right.filename_id){
      return true;
   }
   if (left.time < right.time){
      return true;
   }
   return (left.template_id < right.template_id);
}
*/

///////////////////////////////////////////////////////////////////////////////////
////////////Now define the functions!


int RRD_Writer_worker::thread_start(){
   int rvalue;
   RRD_datum rrd_datum;
   RRD_File_info file_info;
   string rrd_template;
   char filename[CONST_STRING_SIZE];
   int rrd_argc;
   char **rrd_argv;
   char value_str[CONST_STRING_SIZE];
   char template_str[CONST_STRING_SIZE];
   char temp_str[CONST_STRING_SIZE];
   int rrd_status;
   int i;
   int require_flush;
   int index;
   time_t last_file_update;
   unsigned int seed;
   int count;
   int queue_size;
   string to_be_flushed;   
   const int delay_value=5;
   struct timeval current_time;
   struct timespec timeout_time;


   rrd_argc=1;
   rrd_argv = (char**)calloc(rrd_argc, sizeof(char**));
   rrd_argv[0]=value_str;
   
    
   index=(int)((long int) pthread_self());
   seed=index;


   //per thread library inits
   rrd_get_context();


   while(1==1){
      //prepare for timeout stuff
      rvalue=gettimeofday(&current_time,NULL);
      assert(0==rvalue);
      //set timeout secs=current+delay+(0..delay);
      timeout_time.tv_sec =current_time.tv_sec+delay_value+(time_t)(1.0*(delay_value)*(rand_r(&seed)/(RAND_MAX+1.0)));
      timeout_time.tv_nsec=current_time.tv_usec*1000; //convert uset to nsec

      sem_retry:
      rvalue=sem_timedwait(&(parent->data_queue[id].semaphore),&timeout_time);
      if(0!=rvalue && ETIMEDOUT==errno){
         //got a timeout!
         print_log(LOG_DEBUG,"[thread %i] Writer, got a timeout! ",index);
         if(0!=to_be_flushed.size()){
            print_log(LOG_INFO,"[thread %i] About to  flush %s",index,to_be_flushed.c_str());
            rvalue=file_datasync(to_be_flushed.c_str());
            //rvalue=file_drop_from_caches(to_be_flushed.c_str());
         }
         to_be_flushed.clear();
         continue;//no need to do anything else, go to top of loop!
      }
      else{
         // 
         if(EINTR==errno){
            goto sem_retry;
         }
         assert(0==rvalue);
      }     

      //print_log(LOG_INFO,"[thread %d] writer awake id=%d\n",index,id);

      require_flush=0;
    

      // remove val from queue (safely)
      //rvalue=pthread_mutex_lock(&(parent->lock));
      rvalue=pthread_mutex_lock(&(parent->data_queue[id].lock));
      assert(0==rvalue);

      rrd_datum=parent->data_queue[id].datum_queue.front();
      //rrd_datum=parent->data_queue[id].datum_queue.top();
      parent->data_queue[id].datum_queue.pop();

      //rvalue=pthread_mutex_unlock(&(parent->lock));
      rvalue=pthread_mutex_unlock(&(parent->data_queue[id].lock));
      assert(0==rvalue);



      //if(count%1024==333){
      //   queue_size=parent->data_queue[id].datum_queue.size();
      //}

      rvalue=pthread_mutex_lock(&(parent->lock));
      //rvalue=pthread_mutex_lock(&(parent->data_queue[id].lock));
      assert(0==rvalue);

      file_info    =parent->file_info[rrd_datum.filename_id];
      rrd_template =parent->rrd_template[rrd_datum.template_id];

   
      rvalue=pthread_mutex_unlock(&(parent->lock));
      //rvalue=pthread_mutex_unlock(&(parent->data_queue[id].lock));
      assert(0==rvalue);

      //if(count%1024==333){
        // print_log(LOG_INFO,"[thread %d] writer queue[%d].size=%d\n",index,id,queue_size);
      //}


      //print_log(LOG_DEBUG,"afer critical section\n"); 
      

      //check for flush
      last_file_update=rrd_last_r(filename);

      if(last_file_update+7200<time(NULL) && 0!=last_file_update){
          require_flush=1;
      }  

      
      if(1.0*(rand_r(&seed)/(RAND_MAX+1.0))<(1.0/(3600.0*2.0)) ) {
            print_log(LOG_INFO,"[thread %i] Random selection, requesting flush %s \n",index,filename);
            require_flush=1;
      }


      //write to disk
      rrd_clear_error();

      strncpy(template_str, rrd_template.c_str(),sizeof(template_str)-1);
      strncpy(filename, file_info.filename.c_str(),sizeof(template_str)-1);
      sprintf(value_str,"%llu",(unsigned long long)rrd_datum.time);       
    
      print_log(LOG_DEBUG,"[thread %i] num_ds=%d time=%f\n",index,rrd_datum.num_ds,rrd_datum.time);

      for(i=0;i<rrd_datum.num_ds;i++){
         if(isnan(rrd_datum.ds[i])){
             sprintf(temp_str,":U");
         }else{
             sprintf(temp_str,":%llu",(unsigned long long)rrd_datum.ds[i]);  //unsigned long longs are at least 64bit
         }
         strncat(value_str,temp_str,CONST_STRING_SIZE-strlen(value_str));
         
         //print_log(LOG_DEBUG,"value_str=%s temp_str=%s\n",value_str,temp_str);
      } 
      //rrd_argv[0]=value_str;
      print_log(LOG_DEBUG,"[thread %i] filename=%s,template=%s,values=%s \n",index,filename,template_str,value_str);


      rrd_status = rrd_update_r(filename,template_str,1,(const char **)rrd_argv);   
      //rrd_status=0;

      //print_log(LOG_INFO,"[thread %i] RRD STATUS: %d\n",index,rrd_status);
      if(0!=rrd_status){
           int log_level=LOG_ERR;
           //usually log as error, but if dup update only log on verbose or debug
           if(NULL!=strstr(rrd_get_error(),"minimum one second step")){
              log_level=LOG_INFO;
           }
           print_log(log_level,"[thread %i] Failed to update , RRD Error: %s\n",index,rrd_get_error());
           //print_log(LOG_WARNING,"[thread %i] filename=%s,template=%s,values=%s \n",index,filename,template_str,value_str);
           continue;
      }

      //require_flush=0;
      if(0!=require_flush){
          to_be_flushed=filename;
      }

      require_flush=0;
      if(0!=require_flush){
           int fd;

           print_log(LOG_INFO,"[thread %i]  flushing %s \n",index,filename);
           fd=open(filename, O_RDWR);
           if(fd>0){
                 //rvalue=pthread_mutex_lock(&sequential_disk_mutex);
                 //assert(0==rvalue);
                 #if _POSIX_C_SOURCE >= 199309L
                 fdatasync(fd);
                 #else
                 //fsync is much more expensive!
                 fsync(fd);
                 #endif

                 //rvalue=pthread_mutex_unlock(&sequential_disk_mutex);
                 //assert(0==rvalue);
                 print_log(LOG_DEBUG,"[thread %i]  flushing %s of sucess\n",index,filename);
                 close(fd);
                 //update file info for next iteration
                 time_t now_time=time(NULL);
                 rvalue=pthread_mutex_lock(&(parent->lock));
                 assert(0==rvalue);  
                 parent->file_info[rrd_datum.filename_id].last_flushed=now_time;
                 parent->file_info[rrd_datum.filename_id].last_update=now_time;
                 rvalue=pthread_mutex_unlock(&(parent->lock));
                 assert(0==rvalue);
               

           }
      } 
      else{
          //update file info for next iteration
/*
          time_t now_time=time(NULL);
          rvalue=pthread_mutex_lock(&(parent->lock));
          assert(0==rvalue);
          parent->file_info[rrd_datum.filename_id].last_update=now_time;
          rvalue=pthread_mutex_unlock(&(parent->lock));
          assert(0==rvalue);
*/
      } 

      count++;

      
   }

   return 0;
}

void *RRD_Writer_worker_thread_func(void *in_class){
              RRD_Writer_worker *local_class;
              local_class=(RRD_Writer_worker *)in_class;
              local_class->thread_start();
             return NULL;
}

///////////////
void *RRD_Writer_status_thread_func(void *in_class){
              RRD_Writer *local_class;
              local_class=(RRD_Writer *)in_class;
              local_class->writer_status_thread();
             return NULL;
}



//////////////////////////////////////



int RRD_Writer::string_to_long_doubles(char *in_string, long double *vals){
   char *str1,  *token;
   char *saveptr1;
   int i=0;
   uint32_t num_ds=0;

   str1=in_string;
   saveptr1=in_string;

   //print_log(LOG_DEBUG,"in conversion: in='%s'\n",str1);

 
   for (str1 = token; i<MAX_DS_PER_FILE ; i++,str1 = NULL) {
      token = strtok_r(str1, ":", &saveptr1);
      if (token == NULL)
             break;
      //printf(" --> %s\n", subtoken);
      //vals[i]=atof(token);
      vals[i]=strtold(token,NULL);
      ///should probably check for error and if found set to 
      //             std::numeric_limits<double>::quiet_NaN()
      //print_log(LOG_DEBUG," --> %s  \tval[%d]=%f\n", token,i,vals[i]);
      
   }
   if(i>=MAX_DS_PER_FILE){
     return -1;
   } 
   num_ds=i;

   //print_log(LOG_DEBUG,"conversion done : num_ds='%d'\n",num_ds);

   return num_ds;
}

int RRD_Writer::push_data(double in_time,const char *filename,const char *in_rrd_template,const char *values){
   int rvalue=0;
   RRD_datum new_datum;
   RRD_File_info  new_file_info;
   char local_values[CONST_STRING_SIZE]; 
   map<string, uint32_t>::iterator filename_iter,template_iter;
   Fnv32_t queue_index;
   static int count=0;
   

   //print_log(LOG_INFO,"In push data time=%f\n",in_time);

   strncpy(local_values,values,CONST_STRING_SIZE-1);
   queue_index=fnv_32_str((char *)filename, FNV1_32);
   queue_index=queue_index%data_queue.size();




   rvalue=pthread_mutex_lock(&lock);
   assert(0==rvalue);
  
   //do stuff
   new_datum.time=in_time;

 
   //Do filename tranforms 
   filename_iter=filename_id_map.find(filename);  
   if(filename_id_map.end()==filename_iter){
       //insert a new!
       //max_filename_id=file_info.size()+1;
       filename_id_map[filename]= file_info.size();
       new_file_info.filename=filename;
       new_file_info.last_flushed=0;
       new_file_info.last_update=0;
       file_info.push_back(new_file_info);
   } 
   new_datum.filename_id=filename_id_map[filename];

   //do rrd_template transforms.
   template_iter=template_id_map.find(in_rrd_template);
   if(template_id_map.end()==template_iter){
       //insert a new!
       //max_template_id=rrd_template.size()+1;
       template_id_map[in_rrd_template]= rrd_template.size();
       rrd_template.push_back(in_rrd_template);
   }
   new_datum.template_id=template_id_map[in_rrd_template];

   //do values transforms
   new_datum.num_ds=string_to_long_doubles(local_values, new_datum.ds);
   assert(new_datum.num_ds>0);

   //print_log(LOG_DEBUG,"Before push\n");
   //push the value into the queue.


   rvalue=pthread_mutex_lock(&(data_queue[queue_index].lock));
   assert(0==rvalue);

   data_queue[queue_index].datum_queue.push(new_datum);   

   //print_log(LOG_DEBUG,"Before post\n");
   //post the new value to the semaphore

   rvalue=sem_post(&(data_queue[queue_index].semaphore));
   assert(0==rvalue);

   rvalue=pthread_mutex_unlock(&(data_queue[queue_index].lock));
   assert(0==rvalue);

   count++;
   if(0==count%500 ){
      print_log(LOG_INFO,"Push count=%d queue[%d].size=%d\n",count,queue_index,data_queue[queue_index].datum_queue.size() );
   }

   rvalue=pthread_mutex_unlock(&lock);
   assert(0==rvalue);
  

   //print_log(LOG_DEBUG,"push done, queue_index=%d\n",queue_index); 
   return rvalue;
}

int RRD_Writer::initialize(Writer_config *config){
   int rvalue=-1;
   int i;
   pthread_attr_t attr;
   RRD_Writer_worker *current_worker;
   Writer_config default_config;  

   default_config.flush_seconds=7200;
   default_config.num_writers=12;
 
   print_log(LOG_INFO,"Initializing writers (begin)\n");
   

   if (NULL==config){
      config=&default_config;
   }

   rvalue=pthread_mutex_lock(&lock);
   assert(0==rvalue);

   //set the local vals
   flush_seconds=config->flush_seconds;
   if(flush_seconds<=0 || flush_seconds>1000000){
     flush_seconds=1000000;
   }
   num_threads=config->num_writers;
   if(num_threads<=0 || num_threads>400){
     num_threads=400;
   }
   
   //initalize the workers    
   //rvalue=sem_init(&queue_semaphore, 0, 0);
   //assert(0==rvalue); 
   try{
      worker.resize(num_threads);
      data_queue.resize(num_threads);
   }
   catch(...){
      print_log(LOG_ERR,"Failure in the STL code\n");
      exit(EXIT_FAILURE);
   }

   /* Initialize and set thread detached attribute */
   pthread_attr_init(&attr);
   //rvalue = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   rvalue = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   rvalue = pthread_attr_setstacksize(&attr, THREAD_STACK_SIZE_BYTES);


   for(i=0;i<num_threads;i++){
       worker[i].parent=this;
       worker[i].id=i;
       current_worker=&worker[i];
       rvalue=sem_init(&(data_queue[i].semaphore),0,0);
       assert(0==rvalue);
       rvalue=pthread_mutex_init(&(data_queue[i].lock),NULL);
       assert(0==rvalue);

       rvalue=pthread_create(&(worker[i].thread_info),
                             &attr,
                             RRD_Writer_worker_thread_func,
                             (void *) current_worker);

       assert(0==rvalue);
      
   }

   //now the writer_status
   //change the attributes.
   memset(&local_status,0x00,sizeof(struct Writer_status));
   rvalue=pthread_create(&stats_thread,
                         &attr,
                         RRD_Writer_status_thread_func,
                         this);
   assert(0==rvalue);

   rvalue=pthread_mutex_unlock(&lock);
   assert(0==rvalue);
  

   print_log(LOG_INFO,"writers init complete\n");

   return rvalue;
}

RRD_Writer::~RRD_Writer(){
   int i;
   if(worker.size()>0){
      pthread_cancel(stats_thread);
      for(i=0;i<worker.size();i++){
         pthread_cancel(worker[i].thread_info);
      }
   }
}




int RRD_Writer::writer_status_thread(){
   struct timespec sleep_val;
   int rvalue;
   while(1==1){
      pthread_testcancel();
      calculate_new_stats();
      sleep_val.tv_sec=5;
      sleep_val.tv_nsec=0;
      nanosleep(&sleep_val,NULL);      

   }
  

}


int RRD_Writer::calculate_new_stats(){
   //expected to run every 5 seconds!    
   uint64_t avg_queue_len=0;
   int failed_files_count; 
   int i;
   int rvalue;

   //rvalue=pthread_mutex_lock(&lock);
   //assert(0==rvalue);
   for(i=0;i<worker.size();i++){
       rvalue=pthread_mutex_lock(&data_queue[i].lock);
       assert(0==rvalue);
       avg_queue_len+=data_queue[i].datum_queue.size();     
       rvalue=pthread_mutex_unlock(&data_queue[i].lock);
       assert(0==rvalue);

   }
   //avg_queue_len/=worker.size();
   
   rvalue=pthread_mutex_lock(&status_lock);
   assert(0==rvalue);
 
   CALC_LOAD(local_status.avg_queue_len_1min, EXP_1, avg_queue_len);
   CALC_LOAD(local_status.avg_queue_len_5min, EXP_5, avg_queue_len);

   //rvalue=pthread_mutex_unlock(&lock);
   //assert(0==rvalue);

   print_log(LOG_DEBUG,"write status 1minlen=%lu 5minlen=%lu last_val=%lu\n",
            local_status.avg_queue_len_1min,local_status.avg_queue_len_5min,avg_queue_len);

   rvalue=pthread_mutex_unlock(&status_lock);
   assert(0==rvalue);


}

int RRD_Writer::get_writer_status(struct Writer_status *writer_status){
   int rvalue;

   rvalue=pthread_mutex_lock(&status_lock);
   assert(0==rvalue);
   memcpy(writer_status,&local_status,sizeof(struct Writer_status));
   rvalue=pthread_mutex_unlock(&status_lock);
   assert(0==rvalue);
   return 0;
}
 

///////////////////////////////////////////////////////////////////////////////////
/// And the externals

extern "C" int init_writers(struct Writer_config *config){
          return __base_rrd_writer.initialize(config);
         };

extern "C" int push_data(double time,const char *filename,const char *rrd_template,const char *values){
         return __base_rrd_writer.push_data(time,filename,rrd_template,values);
        };

extern "C" int get_writer_status(struct Writer_status *writer_status){
          return __base_rrd_writer.get_writer_status(writer_status);
        };


