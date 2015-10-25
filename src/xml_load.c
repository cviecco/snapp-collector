/*

  SNAPP 3.0

  Copyright(C) 2008 The Trustees of Indiana University

  --------------------------------------------------

  $HeadURL: svn+ssh://svn.grnoc.iu.edu/grnoc/SNAPP/snapp-collector/tags/3.0.7/src/xml_load.c $

  $Id: xml_load.c 9662 2010-11-10 22:08:57Z cviecco $

  $LastChangedRevisions$

  Description:
  Parses the collections file and the base config file 
  and builds the monitored varaible that goes along with it
  used in SNAPP 2.0


*/

#include "config.h"
#include "xml_load.h"
#include "common.h"
#include "util.h"

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<libxml2/libxml/xmlmemory.h>
#include<libxml2/libxml/parser.h>

#include <assert.h>



///Loads the base snapp config values into the db struct so tha the db can be loaded
int load_xml_config(const char *filename, struct XML_config *xml_config){
   
   //belt and suspenders
   assert(NULL!=xml_config);
   assert(NULL!=filename); 

   struct Db_config *db_config;
   struct Control_config *control_config;

   xmlNodePtr cur=NULL,cur2=NULL;;
   xmlNodePtr root=NULL;
   xmlDocPtr doc=NULL;
   int rvalue=-1;
   xmlChar *temp_char;
   int db_type=0;

   db_config=&(xml_config->db_config);
   control_config=&(xml_config->control);

   doc = xmlParseFile(filename);   
   if(NULL==doc){
      print_log(LOG_DEBUG,"Faile to load filename");
      goto cleanup;
   }
   ///add validation here!

   //

   root  = xmlDocGetRootElement(doc);
   if(root == NULL){
      print_log(LOG_DEBUG,"empty config file\n");
      goto cleanup;       
   }


  //move along to the next node
   cur = root->xmlChildrenNode;


   while(cur != NULL){
    if((!xmlStrcmp(cur->name, (const xmlChar *)"name")))
      {
         temp_char =xmlNodeGetContent(cur);
         assert(temp_char!=NULL); //this should be a goto
         print_log(LOG_INFO,"\tThe snapp name is: '%s'\n",temp_char);
         // strncpy(control_config->enable_password,temp_char,STD_STRING_SIZE-1);
         strncpy(xml_config->collector_name,temp_char,STD_STRING_SIZE-1);
         xmlFree(temp_char);
      }
    else if((!xmlStrcmp(cur->name, (const xmlChar *)"db")))
      {
         temp_char = xmlGetProp(cur,"type");
         assert(temp_char!=NULL); //this should be a goto
         print_log(LOG_DEBUG,"\tThe db type is: '%s'\n",temp_char);
         if((!xmlStrcmp(temp_char, (const xmlChar *)"mysql"))){
           db_type=DB_TYPE_MYSQL;
           strcpy(db_config->driver,"mysql");
           strcpy(db_config->port,"3306");
           strcpy(db_config->host,"localhost");
           strcpy(db_config->username,"");
           strcpy(db_config->password,"");
           strcpy(db_config->dbname,"snapp");
         }      
         if((!xmlStrcmp(temp_char, (const xmlChar *)"sqlite"))){
           db_type=DB_TYPE_SQLITE;
           strcpy(db_config->driver,"sqlite3"); //IT is sqlite!
           strcpy(db_config->dbname,"snapp.sqlite");
         }
         xmlFree(temp_char);
         switch(db_type){   
            case DB_TYPE_MYSQL:
               temp_char =xmlGetProp(cur,"port");
               if (temp_char!=NULL){
                   print_log(LOG_DEBUG,"\tThe db port is: '%s'\n",temp_char);
                   strncpy(db_config->port,temp_char,STD_STRING_SIZE-1);
                   xmlFree(temp_char);
               }
               temp_char =xmlGetProp(cur,"host");
               if (temp_char!=NULL){
                   print_log(LOG_DEBUG,"\tThe db host is: '%s'\n",temp_char);
                   strncpy(db_config->host,temp_char,STD_STRING_SIZE-1);
                   xmlFree(temp_char);
               }
               temp_char =xmlGetProp(cur,"password");
               if (temp_char!=NULL){
                   print_log(LOG_DEBUG,"\tThe db password is present\n");
                   strncpy(db_config->password,temp_char,STD_STRING_SIZE-1);
                   xmlFree(temp_char);
               }
               temp_char =xmlGetProp(cur,"username");
               if (temp_char!=NULL){
                   print_log(LOG_DEBUG,"\tThe db username is: '%s'\n",temp_char);
                   strncpy(db_config->username,temp_char,STD_STRING_SIZE-1);
                   xmlFree(temp_char);
               }
               temp_char =xmlGetProp(cur,"name");
               if (temp_char!=NULL){
                   print_log(LOG_DEBUG,"\tThe db name is: '%s'\n",temp_char);
                   strncpy(db_config->dbname,temp_char,STD_STRING_SIZE-1);
                   xmlFree(temp_char);
               }
               break;
            
         }

      }     
    else if((!xmlStrcmp(cur->name, (const xmlChar *)"control")))
      {
         temp_char = xmlGetProp(cur,"port");
         assert(temp_char!=NULL); //this should be a goto
         print_log(LOG_DEBUG,"\tThe control port is: '%s'\n",temp_char);
         control_config->port=atoi(temp_char);
         xmlFree(temp_char);
         
         temp_char = xmlGetProp(cur,"enable_password");
         assert(temp_char!=NULL); //this should be a goto
         print_log(LOG_DEBUG,"\tThe enable is: '%s'\n",temp_char);
         strncpy(control_config->enable_password,temp_char,STD_STRING_SIZE-1);
         xmlFree(temp_char);



      }

    cur = cur->next;
   }


   rvalue=0;
   //creanup
cleanup:
   if(NULL!=cur){
      
   }

   if(NULL!=doc){
      xmlFreeDoc(doc);
   }


   return rvalue;
}



