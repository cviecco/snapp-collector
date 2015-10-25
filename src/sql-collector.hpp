/*

  SNAPP 2.0
  
  Copyright(C) 2008 The Trustees of Indiana University

  --------------------------------------------------

  $HeadURL: svn+grnocssh://svn.grnoc.iu.edu/grnoc/SNAPP/snapp-collector/branches/sqlcollector/sql-collector.h $
  
  $Id: sql-collector.h 7227 2010-04-05 13:48:45Z cviecco $

  Description: 
  Data Collector for SNAPP, uses RRD and the
  NEt-SNMP libraries to get data and store it in a RRD database


*/

#ifndef SNAPP_COLLECTOR_HPP
#define SNAPP_COLLECTOR_HPP

#include "config.h"
#include "common.h"

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

//#include <net-snmp/types.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/types.h>
#include <syslog.h>


///now c++ includes
#include <string>

using namespace std;

//#include <net-snmp/net-snmp-includes.h>

///Stores all device specific information.

///A place to hold our OIDs!
/*
struct Oid_name_oid {
  char Name[255];  //final oid NAME to query
  oid Oid[MAX_OID_LEN]; //final oid to query (as '(.\d+)+' format)
  size_t OidLen;
};
*/
class Oid_blob{
   public:
     oid Oid[MAX_OID_LEN]; //final oid to query (as '(.\d+)+' format)
     size_t OidLen;

};

///Stores RRA definitions
class RRA_definition{
  public:
    uint32_t steps;
    string cf;
    uint32_t numdays; ///< gets transformed into numrows
    double xff;
};

///stores DB OID definitions
class Oid_collection{
  public:
    string oid_prefix;
    string ds_datatype;
    string ds_name; 
};

/// \brief Holds the global runtime variables
///
/// Contains the session semaphore
struct Global_vars{
   sem_t session_semaphore;
   pthread_mutex_t net_snmp_mutex;

};


/////////----------

/////////////////////////////////////////
//Now the extrenal functions

#endif
