/*

  SNAPP 3.0

  Copyright(C) 2008 The Trustees of Indiana University

  --------------------------------------------------

  $HeadURL: svn+grnocssh://svn.grnoc.iu.edu/grnoc/SNAPP/snapp-collector/branches/sqlcollector/db_load.h $

  $Id: db_load.h 7274 2010-04-08 15:27:42Z cviecco $

  $LastChangedRevisions$

  Description:
  Parses the collections file and the base config file 
  and builds the monitored varaible that goes along with it
  used in SNAPP 2.0


*/

#ifndef SNAPP_CONFIG_HPP
#define SNAPP_CONFIG_HPP

#include "config.h"
#include "xml_load.h"
//#include "sql-collector.hpp"
#include "collection_class.hpp"


#include <dbi/dbi.h>
#include <string>

/**
 * \defgroup SNAPPConfig SNAPP_Config
 */
/*@{*/


/// \brief A running(or loading) snapp config.
///
/// Contains all the classes necesary to load and
/// start collection of a configuration as known
/// from the db.
/// Provides cleanup for all objects created (including threads).
class Snapp_config{
  public:
    //the host info must appear before other structs to
    //ensure it is destroyed last!
    Host_Info_Table host_table;
    OID_mapping_sequence_db oid_mapping_sequence_db;
    vector<SQL_Collection_Class> collection_class;
    struct Global_vars *globals;

    char rrd_Dir[CONFIG_SYS_STR_SIZE];
    //struct Control_config control_config;
    struct XML_config xml_config;  //is there a better way to make it private? 

    //will make private
  private:
    //struct Db_config db_config;   
    dbi_conn conn;
    string config_filename;
    string rrd_dir;

  public:
    int load_config(const char *filename);
    int init_config();
    int start_config();   
    static int recursive_dir_create(string current_dir);
    string get_rrd_dir(){return rrd_dir;};
    string get_control_enable_password(){return xml_config.control.enable_password;};
    int    get_control_port(){return xml_config.control.port;};
    string get_collector_name(){return xml_config.collector_name;};
 
  private:
    //load helpers
    int  connect_to_db(struct Db_config *db_config );
    int  list_drivers();
    int  load_host_mappings();
    int  load_collection_class_block(int *last_db_id, int *last_host_id, int *host_index,int limit, int offset);
    int  load_collection_classes();
    int  load_colection_classes_oid();
    int  load_colection_classes_rra();
    int  load_globals();

    int  load_hosts();
    int  load_mapping_sequences();

    //init helpers
    int  create_missing_rrd_files();
    int  map_collection_suffixes();  

};

/*@}*/


#endif

