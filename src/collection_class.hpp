/*

*/

/*
The primary data structure for everything!
*/

#ifndef COLLECTION_CLASS_HPP
#define COLLECTION_CLASS_HPP

//local includes
#include "config.h"
#include "util.h"
#include "common.h"
#include "sql-collector.hpp"
#include "host_info.hpp"

//c includes
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

//c++ includes
#include <vector>
#include <queue>
#include <map>
#include <string>

using namespace std;

/**
 * \defgroup SNMP_collector SNMP Collector
 */
/*@{*/


#define THREADS_PER_CLASS 200



///contains a single mapping sequence from the db.
class OID_mapping_sequence{
  public:
    string name;
    int32_t db_id;
    vector<string> oid;
};

///contains the table of all mapping sequences.
class OID_mapping_sequence_db{
   public:
     vector<OID_mapping_sequence> mapping;
     map<string,int> name_to_index;
     map<int,int>    db_id_to_index;
   private:
    static pthread_mutex_t lock;
   public:
     OID_mapping_sequence *get_sequence_by_db_id(uint32_t db_id);
     OID_mapping_sequence *get_sequence_by_name(string name);  
     int insert_new_sequence(class OID_mapping_sequence &inclass );

};

///contains all the collection specific information about a collection.
class SQL_Collection{
   public:
      double last_secs_to_complete;
      vector <string> oid_to_collect;
      string premaping_oid_suffix;
      string post_mapping_oid_suffix; 
      int64_t oid_mapping_sequence_db_id; 
      string rrd_filename;  
};


/// contains all the collections for a host inside a collection class
class SQL_Host{
   public:
     int last_competed;
     vector<SQL_Collection> collection;    
     class Host_Info *host;

     //one per thread to avoid locks.
     int timeticks; //used at collection time
     int mytime;    //used at collection time

   
     int report_mappings(); //run report on mapping states!
};


class SQL_Collection_Class;

/// Thread speficic information for an SQL collector_class thread
class Collection_worker{
    public:
     pthread_t thread_info;
     SQL_Collection_Class *parent;
     struct snmp_session session;
   public:
     int thread_start();
   public:
     int get_collect(int host_index);
};

/// Conatains all the information about a single db collection class.
class SQL_Collection_Class{
   public:
     uint32_t db_id;
     uint32_t interval;
     string name;
     pthread_t main_thread;
     vector<Oid_collection> oid;
     vector<RRA_definition> rra;
     vector<SQL_Host> host;
     vector<Collection_worker> worker;

     //next two control the delivery of the queue
     queue<int> pending_host_index_queue;
     sem_t worker_semaphore;
     pthread_mutex_t lock;
     

     //global snmp_session_semaphore
     sem_t *session_semaphore;
     string rrd_dir; //redundant?
  private:
     int initialized;

  public:
     SQL_Collection_Class();
     int initialize();
     int thread_start();
     int finalize();
     ~SQL_Collection_Class();     
};

/*@}*/

#endif

