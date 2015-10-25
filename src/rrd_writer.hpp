#ifndef RRD_WRITER_HPP
#define RRD_WRITER_HPP


#include "config.h"
#include "rrd_writer.h"
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

#include <fcntl.h>   //for the open flags



//c++
#include <map>
#include <queue>
#include <iostream>
#include <vector>
#include <string>

#include "rrd_is_thread_safe.h"

#define THREAD_STACK_SIZE_BYTES 256*1024
#define CONST_STRING_SIZE 1024

using namespace std;

/**
 * \defgroup RRDwriter RRD_writer
 */
/*@{*/


class RRD_Writer;

///the class that actually writes data to the rrd files
class RRD_Writer_worker{
    friend class RRD_Writer;
  private:
    pthread_t thread_info;
    uint32_t id;
    RRD_Writer *parent;
  public: 
    int thread_start(); //must be made public for the c thread_start routine to access it
};


///Container of a single rrd update
class RRD_datum{
  public:
    double time;
    uint32_t filename_id;
    uint32_t template_id;
    uint32_t num_ds;
    long double ds[MAX_DS_PER_FILE];
  
 public:
     bool operator < ( RRD_datum &right) const{
        return filename_id<right.filename_id;
     }
    
};


class Compare_RRD_datum {
    public:
    bool operator()(RRD_datum& left, RRD_datum& right) // Returns true if t1 is earlier than t2
    {
/*
       if (t1.h < t2.h) return true;
       if (t1.h == t2.h && t1.m < t2.m) return true;
       if (t1.h == t2.h && t1.m == t2.m && t1.s < t2.s) return true;
       return false;
       
*/
 
       if ( left.filename_id<right.filename_id){
          return true;
       }
       //else{
       //   return false;
       //}
//       if(left.template_id<right.template_id){ 
//          return true;
//       }
       return left.time>right.time; //Yes, the oposite, lower times go to the top!       
    }
};

///data about single rrd file
class RRD_File_info{
  public:
   string filename;
   time_t last_flushed;
   time_t last_update;
};

///queues rrd updates (RRD_datums) safely.
class RRD_queue{
  public:
   //priority_queue<RRD_datum,deque<RRD_datum>,Compare_RRD_datum> datum_queue;
   queue<RRD_datum> datum_queue;
   sem_t semaphore;
   pthread_mutex_t lock;
};

///the main rrd writer class, pushes data into the worker's queues
class RRD_Writer{

  public:
   static int initialized; 
   static pthread_mutex_t lock; 

  public:
    int num_threads;
    int flush_seconds;   
    map<string, uint32_t> filename_id_map;
    map<string, uint32_t> template_id_map;
    vector<string> rrd_template;
    vector<RRD_Writer_worker> worker;
    vector<RRD_File_info> file_info;
    vector<RRD_queue> data_queue; 
    
  private:
    struct Writer_status local_status;
    pthread_mutex_t status_lock;  

  private:
    pthread_t stats_thread;
    int calculate_new_stats();

  public: //the functions -> externals
    int string_to_long_doubles(char *in_string, long double *vals);
    int initialize(Writer_config *config);   
    int push_data(double in_time,const char *filename,const char *in_rrd_template,const char *values);
    int get_writer_status(struct Writer_status *status);
    int writer_status_thread(); 
    ~RRD_Writer();       
};

/*\@}*/

#endif


