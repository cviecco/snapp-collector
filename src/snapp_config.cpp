/*

  SNAPP 3.0

  Copyright(C) 2008 The Trustees of Indiana University

  --------------------------------------------------

  $HeadURL: svn+grnocssh://svn.grnoc.iu.edu/grnoc/SNAPP/snapp-collector/branches/sqlcollector/db_load.c $

  $Id: db_load.c 7227 2010-04-05 13:48:45Z cviecco $

  $LastChangedRevisions$

  Description:
  Parses the collections file and the base config file 
  and builds the monitored varaible that goes along with it
  used in SNAPP 2.0


*/

#include "config.h"
#include "snapp_config.hpp"
#include "collection_class.hpp"
#include "xml_load.h"
#include "sql-collector.hpp"
#include "util.h"

#include <dbi/dbi.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

//#include<libxml2/libxml/xmlmemory.h>
//#include<libxml2/libxml/parser.h>

#include <rrd.h>
#include "rrd_is_thread_safe.h"

#include <assert.h>



int Snapp_config::connect_to_db(struct Db_config *db_config ){
   dbi_result result;

  //const int MySQL_hash=fnv_32_str("MySQL", FNV1_32);   

  assert(NULL!=db_config);

  conn=dbi_conn_new(db_config->driver);
  if(NULL==conn){
     print_log(LOG_ERR,"Failed to load dbi driver, please check db and/or dbi settings\n");
     exit(EXIT_FAILURE);
  }

  //all db have db_names
  dbi_conn_set_option(conn, "dbname", db_config->dbname);
  if(0==strncmp("mysql",db_config->driver,15)){
     dbi_conn_set_option(conn,"host",db_config->host);
     dbi_conn_set_option(conn,"port",db_config->port);
     dbi_conn_set_option(conn, "username", db_config->username);
     dbi_conn_set_option(conn, "password", db_config->password);
  }
  if(0==strncmp("sqlite3",db_config->driver,15)){
     dbi_conn_set_option(conn, "sqlite3db_dir", db_config->sqlite3_dbdir);
     dbi_conn_set_option(conn, "sqlite3_timeout", "100");
  }

  if(0>dbi_conn_connect(conn)){
      print_log(LOG_ERR,"Config loader: Failed to connect to database!\n");
      //exit(EXIT_FAILURE);
      return -1;
      }
   print_log(LOG_DEBUG,"Connection to db -> success!\n");

/*
   result=dbi_conn_query(*conn,"START TRANSACTION;"); //notice SQLITE uses "BEGIN TRANSACTION"
   if(!result){
       print_log(LOG_DEBUG,"failed to start transaction (in)\n");
       return -1;
   }
*/

   return 0;
}

int Snapp_config::list_drivers(){
   dbi_driver current;
   char *driver_name;
   current= dbi_driver_list(NULL);
   while(current!=NULL){
       driver_name=(char *) dbi_driver_get_name(current);
       print_log(LOG_DEBUG,"dbi available driver %s\n",driver_name);
       current= dbi_driver_list(current);
   }
   
}
/////

int Snapp_config::load_collection_classes(){
   const int row_limit=5000;
   int affected_rows;
   int num_rows_done=0;
   
   int last_db_id=-1;
   int last_host_id=-1;
   int host_index=-1;

   //preset values!
   collection_class.reserve(130000);
   assert(0==collection_class.size());

   do{
     affected_rows=load_collection_class_block(&last_db_id, &last_host_id, &host_index,row_limit, num_rows_done);
     if(affected_rows<0){
        return -1;
     }
     num_rows_done+=affected_rows;
   }while(row_limit==affected_rows);

   print_log(LOG_NOTICE,"collection_classes loaded=%d, collections_loaded=%d\n",collection_class.size(),num_rows_done);

   return 0;
}


///This function loads the collection classes AND the related collections associated with a class.
/// Assumes that the host_table has been already loaded.
int Snapp_config::load_collection_class_block(int *last_db_id, int *last_host_id,int *host_index, int limit, int offset){
   char query[CONFIG_SYS_STR_SIZE];
   dbi_result result;
   class SQL_Collection collection;
   class SQL_Host host;
   class SQL_Collection_Class local_collection_class;
   class OID_Value_Table oid_table;
   class OID_mapping_sequence *local_oid_seq;


   //int last_db_id=-1;
   int collection_class_index=collection_class.size()-1;
   //int last_host_id=-1;
   //int host_index=-1;
   int host_id;
   int i,j=0;
   int affected_rows;

   print_log(LOG_DEBUG,"Start of colection class block\n");
   memset(query,0x00,CONFIG_SYS_STR_SIZE);
   
   //Amazing, reading iterating over the result set is superlinear! 
   snprintf(query,CONFIG_SYS_STR_SIZE-1," select collection.collection_class_id,collection_interval,collection_class.name, host_id,oid_suffix_mapping_id"
         ",rrdfile,premap_oid_suffix "
       " from collection,collection_class,collection_instantiation "
       " where collection_class.collection_class_id=collection.collection_class_id and collection_instantiation.collection_id=collection.collection_id and collection_instantiation.end_epoch=-1 "
       "order by collection_class_id,host_id limit %d offset %d ",limit,offset);

   //print_log(LOG_DEBUG,"before query of colection class block, query='%s'\n",query);
   result=dbi_conn_query(conn,query);
   //print_log(LOG_DEBUG,"after query of colection class block\n");
   if(!result){
       print_log(LOG_DEBUG,"get results\n");
       return -1;
   }
   //collection_class.reserve(110000);
   //assert(0==collection_class.size());
   print_log(LOG_DEBUG,"Starting loading of classes numrows=%lu offset=%d\n", dbi_result_get_numrows(result),offset);
   affected_rows= dbi_result_get_numrows(result);
   while (dbi_result_next_row(result)) {
      //printf("%d\n ",j);
      //j++;

      local_collection_class.db_id = dbi_result_get_int(result, "collection_class_id"); 

      //print_log(LOG_DEBUG,"before last_db check\n");
      if(*last_db_id!=local_collection_class.db_id){
           //new collection class, do something!
          //local_collection_class.initialized=0;
          local_collection_class.name     = dbi_result_get_string(result, "name");
          local_collection_class.interval = dbi_result_get_int(result, "collection_interval");
          
          print_log(LOG_DEBUG,"%6d:\t%4d\t%s\n",local_collection_class.db_id,local_collection_class.interval,local_collection_class.name.c_str());

          *last_db_id=local_collection_class.db_id;
          collection_class.push_back(local_collection_class);
          collection_class_index=collection_class.size()-1;
          *last_host_id=-1;
          *host_index=-1;
 

      }   
     
      host_id= dbi_result_get_int(result, "host_id");
      //print_log(LOG_DEBUG,"before host_id check\n");
      if(*last_host_id!=host_id){
           host.host=host_table.get_host_for_db_id(host_id);
           if(NULL==host.host){
               print_log(LOG_ERR,"Cannot find loaded host for host_id=%d\n",host_id);
               return -1;
           }
           collection_class[collection_class_index].host.reserve(1000);
           collection_class[collection_class_index].host.push_back(host);
           *host_index=collection_class[collection_class_index].host.size()-1;
           *last_host_id=host_id;
      }

  
      collection.rrd_filename              = dbi_result_get_string(result, "rrdfile");
      collection.premaping_oid_suffix      = dbi_result_get_string(result, "premap_oid_suffix");

      if(dbi_result_field_is_null(result, "oid_suffix_mapping_id")){
          collection.oid_mapping_sequence_db_id=-1;
      }
      else{
          collection.oid_mapping_sequence_db_id= dbi_result_get_int(result, "oid_suffix_mapping_id");
          //oid_table.oid
          local_oid_seq= oid_mapping_sequence_db.get_sequence_by_db_id(collection.oid_mapping_sequence_db_id);
          if(NULL==local_oid_seq){
               print_log(LOG_ERR,"Cannot mapping sequnce for id=%d\n",collection.oid_mapping_sequence_db_id);
               return -1;
          }
          ///now we insert emply oid mapping tables for each host.
          for(i=0;i<(*local_oid_seq).oid.size();i++){
              oid_table.oid=(*local_oid_seq).oid[i];
              collection_class[collection_class_index].host[*host_index].host->oid_table[oid_table.oid]=oid_table;
          }
      }

      //print_log(LOG_DEBUG,"collection_class_index=%d host_index=%d\n",collection_class_index,*host_index);
      //print_log(LOG_DEBUG,"%6d\t%d\t%d\t%10s\t%s\n",collection_class[collection_class_index].host[*host_index].collection.size(),host_id,collection.oid_mapping_sequence_db_id,collection.premaping_oid_suffix.c_str(),collection.rrd_filename.c_str());
      collection_class[collection_class_index].host[*host_index].collection.push_back(collection);


   }
   dbi_result_free(result);

   print_log(LOG_DEBUG,"Number of collection classes  so far loaded=%d\n",collection_class.size());

   return affected_rows; 


}

int Snapp_config::load_colection_classes_oid(){
   char query[CONFIG_SYS_STR_SIZE];
   dbi_result result;
   Oid_collection local_oid;

   int i;
   for(i=0;i<collection_class.size();i++){
      //int db_id=collection_class[i].db_id;
      memset(query,0x00,CONFIG_SYS_STR_SIZE);
      snprintf(query,CONFIG_SYS_STR_SIZE-1,"select ds_name,OID_prefix,datatype from oid_collection_class_map natural join oid_collection where collection_class_id=%d order by order_val",collection_class[i].db_id);

      result=dbi_conn_query(conn,query);
      if(!result){
         print_log(LOG_DEBUG,"get results oid\n");
         return -1;
      }

      while (dbi_result_next_row(result)) {
          assert(NULL!=dbi_result_get_string(result, "ds_name"));
          assert(NULL!=dbi_result_get_string(result, "datatype"));
          assert(NULL!=dbi_result_get_string(result, "OID_prefix"));
          local_oid.oid_prefix  =  dbi_result_get_string(result, "OID_prefix");
          local_oid.ds_name     =  dbi_result_get_string(result, "ds_name");
          local_oid.ds_datatype =  dbi_result_get_string(result, "datatype");
          print_log(LOG_DEBUG,"%4d\t%20s %20s\t'%s'\n",collection_class[i].db_id,local_oid.ds_name.c_str(),local_oid.ds_datatype.c_str(),local_oid.oid_prefix.c_str());
          collection_class[i].oid.push_back(local_oid);
      }  
      dbi_result_free(result);

 
   }

   return 0;
};



int Snapp_config::load_colection_classes_rra(){
   char query[CONFIG_SYS_STR_SIZE];
   dbi_result result;
   RRA_definition local_rra;
   

   int i;
   for(i=0;i<collection_class.size();i++){
      //int db_id=collection_class[i].db_id;
      memset(query,0x00,CONFIG_SYS_STR_SIZE);
      snprintf(query,CONFIG_SYS_STR_SIZE-1,"select step,cf,num_days,xff from rra where collection_class_id=%d order by step",collection_class[i].db_id);

      result=dbi_conn_query(conn,query);
      if(!result){
         print_log(LOG_DEBUG,"get results oid\n");
         return -1;
      }

      while (dbi_result_next_row(result)) {
          local_rra.steps       =  dbi_result_get_int(result, "step");
          local_rra.numdays     =  dbi_result_get_int(result, "num_days");
          local_rra.xff         =  dbi_result_get_double(result, "xff");          
          local_rra.cf          =  dbi_result_get_string(result, "cf");
          print_log(LOG_DEBUG,"%4d\t%6d %6d %4f\t%s\n",collection_class[i].db_id,local_rra.steps,local_rra.numdays,local_rra.xff,local_rra.cf.c_str());

/*
          local_oid.oid_prefix  =  dbi_result_get_string(result, "OID_prefix");
          local_oid.ds_name     =  dbi_result_get_string(result, "ds_name");
          local_oid.ds_datatype =  dbi_result_get_string(result, "datatype");
          print_log(LOG_DEBUG,"%4d\t%20s %20s\t'%s'\n",collection_class[i].db_id,local_oid.ds_name.c_str(),local_oid.ds_datatype.c_str(),local_oid.oid_prefix.c_str());
*/
          collection_class[i].rra.push_back(local_rra);

      } 
      dbi_result_free(result);


   }

   return 0;
};



int Snapp_config::load_globals(){
   char query[CONFIG_SYS_STR_SIZE];
   dbi_result result;

   memset(query,0x00,CONFIG_SYS_STR_SIZE);
   snprintf(query,CONFIG_SYS_STR_SIZE-1,"select name,value from global ");

   result=dbi_conn_query(conn,query);
   if(!result){
       print_log(LOG_DEBUG,"get results\n");
       return -1;
   }
  while (dbi_result_next_row(result)) {
      string name,value;
      name  =  dbi_result_get_string(result, "name");
      value =  dbi_result_get_string(result, "value");
      //oid_table.prefi
      //print_log(LOG_DEBUG,"%6i:\t%s\t%s \n", host.db_id, host.ip_address.c_str(),host.community.c_str());
      //and add it to the db!
      //host_table.insert_new_host(host);
      if(0==name.compare("rrddir")){
         rrd_dir=value;
         rrd_dir.append("/");
         print_log(LOG_DEBUG,"rrd_dir=%s \n", rrd_dir.c_str());  
      }
   }
   dbi_result_free(result);

   //FIXME... To change back after devel is done!
   //rrd_dir="/array/SNMP/testing-sql/db/";

   return 0;


}


int Snapp_config::load_hosts(){
   char query[CONFIG_SYS_STR_SIZE];
   dbi_result result;
   Host_Info host;
   OID_Value_Table oid_table;

   //stage 0 ... will NOT take care of hiearchical tables...

   memset(query,0x00,CONFIG_SYS_STR_SIZE);
   snprintf(query,CONFIG_SYS_STR_SIZE-1,"select host.host_id,ip_address,community,OID from host,collection, oid_suffix_mapping,oid_suffix_mapping_value where host.host_id=collection.host_id and collection.oid_suffix_mapping_id=oid_suffix_mapping.oid_suffix_mapping_id and oid_suffix_mapping.oid_suffix_mapping_value_id=oid_suffix_mapping_value.oid_suffix_mapping_value_id group by host_id,collection.oid_suffix_mapping_id ;");
   snprintf(query,CONFIG_SYS_STR_SIZE-1,"select host.host_id,ip_address,community,OID,collection.oid_suffix_mapping_id from host,collection left join  oid_suffix_mapping on collection.oid_suffix_mapping_id=oid_suffix_mapping.oid_suffix_mapping_id left join  oid_suffix_mapping_value on  oid_suffix_mapping.oid_suffix_mapping_value_id=oid_suffix_mapping_value.oid_suffix_mapping_value_id  where  host.host_id=collection.host_id group by host_id,collection.oid_suffix_mapping_id desc;");

   result=dbi_conn_query(conn,query);
   if(!result){
       print_log(LOG_DEBUG,"Failure loading hosts from dbi (get results)\n");
       return -1;
   }
  while (dbi_result_next_row(result)) {
      host.db_id       = dbi_result_get_int(result, "host_id");
      host.ip_address  = dbi_result_get_string(result, "ip_address");
      host.community   = dbi_result_get_string(result, "community");
      host.oid_table.clear();
      oid_table.suffix_of.clear();
      oid_table.value_of.clear();
      if(0==dbi_result_field_is_null(result,"OID")){
         oid_table.oid=dbi_result_get_string(result, "OID");
         host.oid_table[oid_table.oid]=oid_table;
      }
      //oid_table.prefi
      print_log(LOG_DEBUG,"%6i:\t%s\t%s \n", host.db_id, host.ip_address.c_str(),host.community.c_str());
      //and add it to the db!
      host_table.insert_new_host(host); 

   }
   dbi_result_free(result);
   return 0;

}

int Snapp_config::load_mapping_sequences(){
   char query[CONFIG_SYS_STR_SIZE];
   dbi_result result;
   OID_mapping_sequence oid_sequence;
   int i;
   const int max_sequence_length=10;


   //stage 0 ... will NOT take care of hiearchical tables...

   memset(query,0x00,CONFIG_SYS_STR_SIZE);
   snprintf(query,CONFIG_SYS_STR_SIZE-1,"select oid_suffix_mapping_id,name,OID,next_oid_suffix_mapping_value_id from oid_suffix_mapping,oid_suffix_mapping_value where oid_suffix_mapping.oid_suffix_mapping_value_id=oid_suffix_mapping_value.oid_suffix_mapping_value_id ;");

   result=dbi_conn_query(conn,query);
   if(!result){
       print_log(LOG_DEBUG,"get results\n");
       return -1;
   }
   while (dbi_result_next_row(result)) {
      oid_sequence.db_id    = dbi_result_get_int(result, "oid_suffix_mapping_id");
      oid_sequence.name     = dbi_result_get_string(result, "name");
      oid_sequence.oid.clear();
      oid_sequence.oid.push_back(dbi_result_get_string(result, "OID"));

      print_log(LOG_INFO,"%6i:\t%12s\t%s \n", oid_sequence.db_id, oid_sequence.name.c_str(),oid_sequence.oid[0].c_str());
      if(0==dbi_result_field_is_null(result,"next_oid_suffix_mapping_value_id")){
         //print_log(LOG_DEBUG,"need to traverse\n");
         int next_db_id;
         int done=0;
         char query2[CONFIG_SYS_STR_SIZE];
         dbi_result result2;

         next_db_id=dbi_result_get_int(result,"next_oid_suffix_mapping_value_id");
         
         for(i=0;i<max_sequence_length && 0==done;i++){ //no no go more than 10 steps depth
             memset(query2,0x00,CONFIG_SYS_STR_SIZE);
             snprintf(query2,CONFIG_SYS_STR_SIZE-1,"select OID,next_oid_suffix_mapping_value_id from oid_suffix_mapping_value where oid_suffix_mapping_value_id=%d",next_db_id);


             result2=dbi_conn_query(conn,query2);
             if(!result2){
                 //print_log(LOG_DEBUG,"get results (result2)\n");
                 done=1;
                 break;
             }
             if(dbi_result_next_row(result2)){
                 oid_sequence.oid.push_back(dbi_result_get_string(result2, "OID"));
                 print_log(LOG_INFO,"%6i:\t%12s\t%s \n", oid_sequence.db_id, oid_sequence.name.c_str(),oid_sequence.oid[i+1].c_str());

                 if(0==dbi_result_field_is_null(result2,"next_oid_suffix_mapping_value_id")){
                     next_db_id=dbi_result_get_int(result2,"next_oid_suffix_mapping_value_id");
                 }
                 else{
                     //print_log(LOG_DEBUG,"done \n");
                     done=1;
                 }
             }
             else{
                 //print_log(LOG_DEBUG,"no results in result2?\n");
                 done=1;
                
             } 
             dbi_result_free(result2);

         }
      }
       

      oid_mapping_sequence_db.insert_new_sequence(oid_sequence);
   
   }
   dbi_result_free(result);
   return 0;

}

int Snapp_config::load_config(const char *filename){
   

   //dbi_conn conn;
   //struct Db_config db_config;
   int rvalue=-1;
   dbi_result result;
   

   ///better to move somewhere else!
   dbi_initialize(NULL);
   list_drivers();
   //rvalue=load_xml_config(filename, &xml_config.db_config,&xml_config.control);
   rvalue=load_xml_config(filename,&xml_config);
   if(rvalue<0){
     return rvalue;
   }
   rvalue=connect_to_db(&xml_config.db_config);
   if(rvalue<0){
     return rvalue;
   }

   print_log(LOG_INFO,"Connected to the db \n");

   ///lock the db!  
   result=dbi_conn_query(conn,"START TRANSACTION;"); //notice SQLITE uses "BEGIN TRANSACTION"
   if(!result){
       print_log(LOG_DEBUG,"failed to start transaction\n");
       return -1;
   } 

   ///now the db is locked proceed
   rvalue = load_hosts();
   if(rvalue<0){
     print_log(LOG_ERR,"Could not load hosts from db\n");
     goto end_transaction;
   }
   rvalue = load_mapping_sequences();
   if(rvalue<0){
     print_log(LOG_ERR,"Could not load mapping sequences from db\n");
     goto end_transaction;
   }
   print_log(LOG_INFO,"Mapping sequences loaded \n");

   rvalue = load_collection_classes();
   if(rvalue<0){
     print_log(LOG_ERR,"Could not load collection_classes/collections from db\n");
     goto end_transaction;
   }
   print_log(LOG_INFO,"Classes/collection_classes loaded \n");

   rvalue = load_colection_classes_oid();
   if(rvalue<0){
     print_log(LOG_ERR,"Could not load oids from db\n");
     goto end_transaction;
   }
   rvalue = load_colection_classes_rra();   
   if(rvalue<0){
     print_log(LOG_ERR,"Could not load rras from db\n");
     goto end_transaction;
   }
   rvalue = load_globals();
   if(rvalue<0){
     print_log(LOG_ERR,"Could not load globals from db\n");
     goto end_transaction;
   }



end_transaction:
   result=dbi_conn_query(conn,"COMMIT;");
   if(!result){
       print_log(LOG_DEBUG,"failed to end transaction\n");
       return -1;
   }
   print_log(LOG_DEBUG,"Closing the DB connection\n");
   dbi_conn_close(conn); 

   return rvalue;
}

int Snapp_config::recursive_dir_create(string current_dir){
   int rvalue;
   size_t found;

   print_log(LOG_DEBUG,"Creating a new dir %s\n",current_dir.c_str());

   rvalue=mkdir(current_dir.c_str(),00755);
   if (0==rvalue){
      return 0;
   }
   switch(errno){
      case EEXIST:
              return 0;
      case ENOENT:
      //case ENOTDIR: //recurse
              found=current_dir.rfind('/');
              rvalue=recursive_dir_create(current_dir.substr(0,found));
      default:
              print_log(LOG_ERR,"Failed to create dir %s errno=%d",current_dir.c_str(),errno); 
              return -1;
   }
   //try again!
   rvalue=mkdir(current_dir.c_str(),00755);
   if (0==rvalue){
      return 0;
   }
   return -1;
   
} 



/**
* Create missing rrdfiles, should also create the dirs if missing!
*/
int Snapp_config::create_missing_rrd_files(){
   int i,j,k,l;
   int rvalue;   
   struct stat rrd_stat;
   int stat_val;
   //now we are 'copy' from the old snapp
   int rrd_status;
   int rrd_argc=0;
   char **rrd_argv;
   char rrd_str[CONFIG_SYS_STR_SIZE], rra_str[CONFIG_SYS_STR_SIZE];
   char buf[128];
   char *save_ptr;
   char *tmp_ptr;
   char *token;
   time_t last_time;
   string dir_location;

   int collection_size;

   for(i=0;i<collection_class.size();i++){
       for(j=0;j<collection_class[i].host.size();j++){
          collection_size=collection_class[i].host[j].collection.size();
          for(k=0;k<collection_size;k++){
              string full_filename;
              //print_log(LOG_DEBUG,"host[%d].collection[%d]/%d\n",j,k,collection_class[i].host[j].collection.size());

              full_filename=rrd_dir;
              full_filename.append(collection_class[i].host[j].collection[k].rrd_filename);

              stat_val=stat(full_filename.c_str(),&rrd_stat);
              if(stat_val>=0 && S_ISREG(rrd_stat.st_mode)){
                 //check_rrdfile(config, filename,  classIndex,i);
                 continue;
              }              
              //oops there is no file, we need to create it...


      print_log(LOG_WARNING, "file %s not found, attempting to create\n",full_filename.c_str());
      //print_log(LOG_DEBUG,"host[%d].collection[%d]/%d\n",j,k,collection_class[i].host[j].collection.size());
      //there was a problem or not a regular file, attempt to create

      //create the dir structure:
      dir_location=full_filename.substr(0,full_filename.rfind('/'));
      print_log(LOG_WARNING, "file  attempting to create dir '%s'\n",dir_location.c_str());
      recursive_dir_create(dir_location);      

      //now create the rrd
      rrd_str[0]=0;
      rra_str[0]=0;
      rrd_argc=0;

      //base sanity check
      if(collection_class[i].oid.size()==0){
         print_log(LOG_ERR,"Collection class for %s has no ds!\n");
         return -1;
      }
      if(collection_class[i].rra.size()==0){
         print_log(LOG_ERR,"Collection class for %s has no rras defined!\n");
         return -1;
      }


      //fill the rrds
      for(l=0;l<collection_class[i].oid.size();l++){
         if(NULL==strstr(collection_class[i].oid[l].ds_datatype.c_str(),"COMPUTE")){
            snprintf(buf,sizeof(buf)-1,"DS:%s:%s:%d:0:1e11 ",
                                         collection_class[i].oid[l].ds_name.c_str(),
                                         collection_class[i].oid[l].ds_datatype.c_str(),
                                         collection_class[i].interval*2);
         }
         else{
           snprintf(buf,sizeof(buf)-1,"DS:%s:%s ",
                                         collection_class[i].oid[l].ds_name.c_str(),
                                         collection_class[i].oid[l].ds_datatype.c_str());
         }
         strncat(rra_str,buf,CONFIG_SYS_STR_SIZE-1);
         rrd_argc++;
      }
      for(l=0;l<collection_class[i].rra.size();l++){
         int numrows=collection_class[i].rra[l].numdays*3600*24/
                     (collection_class[i].interval*collection_class[i].rra[l].steps);
         snprintf(buf,sizeof(buf)-1,"RRA:%s:%f:%d:%d ",
                                    collection_class[i].rra[l].cf.c_str(),
                                    collection_class[i].rra[l].xff,
                                    collection_class[i].rra[l].steps,
                                    numrows
                                     );
         strncat(rra_str,buf,CONFIG_SYS_STR_SIZE-1);
         rrd_argc++;
      }

      //now tokenize!
      //argv = (char**)calloc(argc, sizeof(char**));
      assert(rrd_argc>0);
      rrd_argv=(char **) calloc(rrd_argc+1,sizeof(char *));
      assert(rrd_argv!=NULL);
      save_ptr=rra_str;
      tmp_ptr=rra_str;
      for(l=0;l<rrd_argc;l++, tmp_ptr=NULL){
         token=strtok_r(tmp_ptr," ",&save_ptr);
         assert(token!=NULL);
         //strtok_r()
         rrd_argv[l]=token;
         print_log(LOG_DEBUG," %d:%d %s;",rrd_argc, l,token);
      }
      rrd_argv[l]=NULL;

      rrd_clear_error();
/* thread-safe (hopefully) 
    int       rrd_create_r(
    const char *filename,
    unsigned long pdp_step,
    time_t last_up,
    int argc,
    const char **argv);
*/
       last_time=time(NULL)-10;
       rrd_status=0;
       rrd_status = rrd_create_r(full_filename.c_str(),
                                  collection_class[i].interval,
                                  last_time,
                                  rrd_argc,
                                  (const char **) rrd_argv);
                                  //argc,
                                 //(const char **) argv);

        print_log(LOG_INFO," RRD STATUS: %d\n",rrd_status);
        if(0!=rrd_status){
           print_log(LOG_ERR," Could not create file %s RRD Error: %s\n",full_filename.c_str(),rrd_get_error());
        }
        else{
          file_drop_from_caches(full_filename.c_str());
        }
        free(rrd_argv);
        //print_log(LOG_DEBUG,"iteration complete!\n");

////
              
          }
       }
   }

   return 0;
}

/**
* Does the per collection mappings of the suffixes given a premap suffix and the
* host's oid mapping tables.
*/
int Snapp_config::map_collection_suffixes(){
   int i,j,k,l;
   int collection_size;
   SQL_Collection *current_collection;   
   string last_val;
   OID_mapping_sequence *current_mapping_sequence;
   SQL_Host *current_host;
   Host_Info *current_host_info;
   char empty_string[] ="";
   

   for(i=0;i<collection_class.size();i++){
       for(j=0;j<collection_class[i].host.size();j++){
          current_host=&collection_class[i].host[j];
          current_host_info=current_host->host;

          if(collection_class[i].host[j].host->mapping_done!=1){
            continue;
          }
          collection_size=collection_class[i].host[j].collection.size();
          for(k=0;k<collection_size;k++){
             current_collection=&collection_class[i].host[j].collection[k];
             if(current_collection->oid_mapping_sequence_db_id==-1){
                //no mapping
                current_collection->post_mapping_oid_suffix=current_collection->premaping_oid_suffix;
             }
             else{
                //there is a mapping, first get the maping
                current_mapping_sequence=oid_mapping_sequence_db.get_sequence_by_db_id(current_collection->oid_mapping_sequence_db_id);
                assert(current_mapping_sequence!=NULL);
                //now iterate over the host mapping to ge the value.
                last_val=current_collection->premaping_oid_suffix;
                for(l=0;l<current_mapping_sequence->oid.size();l++){
                   //if there is no reverse mapping then, make the last_val empty!
                   if(current_host_info->oid_table[ current_mapping_sequence->oid[l]].suffix_of.find(last_val) ==
                      current_host_info->oid_table[ current_mapping_sequence->oid[l]].suffix_of.end()){
                      last_val="";
                      break;
                   }

                   //unfornutatelly there is no clean way to assing the decimal representation
                   //of a number into a string, so we have to use a tmp string for the trick
                   //char tmp_str[30];
                   //snprintf(tmp_str,29,"%lu",current_host_info->oid_table[ current_mapping_sequence->oid[l]].indexof[last_val]);
                   //last_val=tmp_str;
                   last_val=current_host_info->oid_table[ current_mapping_sequence->oid[l]].suffix_of[last_val];
                }
                current_collection->post_mapping_oid_suffix=last_val;
             }
             //print_log(LOG_DEBUG," %s => %s \n",current_collection->premaping_oid_suffix.c_str(),current_collection->post_mapping_oid_suffix.c_str());
          }
       }
   }
   print_log(LOG_DEBUG,"collections mappings done\n");

   return 0;
}


int Snapp_config::init_config(){
   int rvalue;
   int fvalue=0;   
   int i;

   //check for preconditions here!

   //now do work
   rvalue=create_missing_rrd_files();
   if (rvalue<0){
      print_log(LOG_ERR,"Error creating rrd files\n");
      return -1;
   }     
   //do just one host loading for now
/*
   for(i=0;i<host_table.host.size();i++){
      host_table.host[i].load_oid_table();
   }
*/
   rvalue=host_table.load_host_tables(&globals->session_semaphore);
   if(rvalue<0){
      print_log(LOG_ERR,"Error building host tables\n");
      return -1;
   }
   rvalue=map_collection_suffixes();
   if(rvalue<0){
      print_log(LOG_ERR,"Error building collection suffixes\n");
      return -1;
   }
   //collection_class[0]. 
   for(i=0;i<collection_class.size();i++){
      collection_class[i].session_semaphore=&globals->session_semaphore;
      collection_class[i].rrd_dir=rrd_dir;
      collection_class[i].initialize();
   }
   //silly test!
   for(i=0;i<3;i++){
      //collection_class[0].worker[0].get_collect(0); 
      //sleep(60);
   }
   //print_log(LOG_DEBUG,"done!?\n");
   //sleep(10);

   return fvalue;
}
