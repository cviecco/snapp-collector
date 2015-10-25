/*

*/

/*
The primary data structure for everything!
*/

#ifndef HOST_INFO_HPP
#define HIST_INFO_HPP

//local includes
#include "config.h"
#include "util.h"
#include "common.h"
#include "sql-collector.hpp"

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


/// \brief Oid mappings container
///
///contains the table of all the value-suffix mappings
///for a single oid inside a single host!
class OID_Value_Table{
  public:
     string oid;
     //map<string,int64_t> indexof;
     //map<int64_t,string> valueof;
     map<string,string> suffix_of;
     map<string,string> value_of;

};

///\brief Per host SNMP settings and oid mappings
///
///The information about a host, incluing SNMP connection
///information and oid mapping tables
class Host_Info{
   public:
     int32_t db_id;
     string ip_address;
     string community;
     volatile int32_t mapping_done;
     map<string,OID_Value_Table> oid_table; 
     sem_t *thread_semaphore; 
 
   public:
     int load_oid_table();
     int load_oid_table_get();
     int thread_start();
     Host_Info();
     static string extract_value_from_raw(string raw);
};

/// \brief Database of host_infos
///
///A database of host info, mapped by the db index of each host in
/// the sql database
class Host_Info_Table{
  public:
   vector<Host_Info> host;
   map<int32_t,uint32_t> db_id_to_index;

  private:
    static pthread_mutex_t lock;

  public:
//    Host_Info_Table();
    Host_Info *get_host_for_db_id(uint32_t db_id);
    int insert_new_host(class Host_Info &inclass );
    //int print_table_to_log();
    int load_host_tables(sem_t *thread_semaphore);
    
};

/*@}*/

#endif

