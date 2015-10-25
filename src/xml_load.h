/*

  SNAPP 3.0

  Copyright(C) 2008 The Trustees of Indiana University

  --------------------------------------------------

  $HeadURL: svn+ssh://svn.grnoc.iu.edu/grnoc/SNAPP/snapp-collector/tags/3.0.7/src/xml_load.h $

  $Id: xml_load.h 9662 2010-11-10 22:08:57Z cviecco $

  $LastChangedRevisions$

  Description:
  Parses the collections file and the base config file 
  and builds the monitored varaible that goes along with it
  used in SNAPP 2.0


*/

#ifndef XML_LOAD_H
#define XML_LOAD_H

#include "config.h"
#include "common.h"

/**
 * \defgroup SNAPPConfig SNAPP_Config
 */
/*@{*/


///How to connect to a db, including driver.
struct Db_config{
    char username[STD_STRING_SIZE];
    char password[STD_STRING_SIZE];
    char dbname[STD_STRING_SIZE];
    char sqlite3_dbdir[STD_STRING_SIZE];
    char host[STD_STRING_SIZE];
    char  port[STD_STRING_SIZE];
    char driver[STD_STRING_SIZE]; //dbi:DriverName:database=database_name;host=hostname;port=port
};

/// What are the properties of the control port
struct Control_config{
    int port;
    char enable_password[STD_STRING_SIZE];
};

/// A single struct to contain all data extracted from the xml file
struct XML_config{
    char collector_name[STD_STRING_SIZE];
    struct Control_config control;
    struct Db_config db_config;
};


#define DB_TYPE_MYSQL  1
#define DB_TYPE_SQLITE 2




#ifdef __cplusplus
extern "C" {
#endif

//int load_xml_config(const char *filename, struct Db_config *db_config, struct Control_config *control_config);
int load_xml_config(const char *filename, struct XML_config *xml_config);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

/*@}*/

#endif

