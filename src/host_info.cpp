
#include "config.h"
#include "sql-collector.hpp"
#include "collection_class.hpp"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

//#include <pthread.h>
#include <assert.h>
#include <pcre.h>

#define PCRE_EXEC_OVECCOUNT_2       30


//////////
Host_Info:: Host_Info(){
    thread_semaphore=NULL;
} 


//
int Host_Info::thread_start(){
    int rvalue;

    rvalue=load_oid_table();
    //rvalue=load_oid_table_get();
    if(rvalue>=0){
        mapping_done=1;
    }

    if(NULL!=thread_semaphore){
       rvalue=sem_post(thread_semaphore);
       assert(rvalue>=0);
    }
   

    return 0;
}
/////////////////////
string Host_Info::extract_value_from_raw(string raw){
   char pattern[]="[^:]+:\\s*\"?([^\"]*)\"?";
   static pcre *re=NULL;
   const char *error;
   int erroffset;
   static pthread_mutex_t re_lock = PTHREAD_MUTEX_INITIALIZER;
   int rvalue;

   //for the actual execution!
   char check_string[STD_STRING_SIZE];
   int ovector[PCRE_EXEC_OVECCOUNT_2];
   int rc;
   char *value;
   char empty_string[]="";
   string out_string;

   value=empty_string;   

   rvalue=pthread_mutex_lock(&re_lock);
   assert(0==rvalue);

   if(NULL==re){
      re = pcre_compile(
              pattern,          /* the pattern */
              0,                /* default options */
              &error,           /* for error message */
              &erroffset,       /* for error offset */
              NULL);            /* use default character tables */
      assert(NULL!=re);
   }
   rvalue=pthread_mutex_unlock(&re_lock);   
   assert(0==rvalue);

   assert(NULL!=re);
   bzero(check_string,STD_STRING_SIZE);
   strncpy(check_string,raw.c_str(),STD_STRING_SIZE-1);
   rc = pcre_exec(
           re,                   //result of pcre_compile() 
           NULL,                 // The studied the pattern 
           check_string,         // the subject string
           #if defined _GNU_SOURCE || _POSIX_C_SOURCE >= 200809L 
           strnlen(check_string,STD_STRING_SIZE), // the length of the subject string
           #else
           strlen(check_string),
           #endif 
           0,                    // start at offset 0 in the subject 
           0,                    // default options 
           ovector,              // vector of integers for substring information 
           PCRE_EXEC_OVECCOUNT_2); // number of elements (NOT size in bytes) 
    if(rc>=1 && ovector[2]!=-1){ 
       //we at least one match!
       value=check_string;
       value+=ovector[2]; 
       out_string=value;
    }
    if('"'==out_string[out_string.length()-1]){
      out_string.resize(out_string.length()-1);
    }
    print_log(LOG_DEBUG,"Value_extractor Raw='%s' value='%s'\n",check_string,out_string.c_str());
    return out_string;


}


/////////////////////////
int Host_Info::load_oid_table_get(){
    unsigned int thread_id;
    struct snmp_session session;

    map<string,OID_Value_Table>::iterator iter;
    int fvalue=0;

    struct snmp_pdu *req, *resp;



    void *sp;
//    struct snmp_session *sptr;
    oid             name[MAX_OID_LEN];
    size_t          name_length=MAX_OID_LEN;
    oid             root[MAX_OID_LEN];
    size_t          rootlen=MAX_OID_LEN;
    struct variable_list *vars;


    char snmp_err_str[SNMP_ERR_STR_SIZE];
    char buf[512];
    char *placeholder;
    char buf2[512];
///
    string temp_string1,temp_string2;
    int running;
    int             status;
    int i;

    thread_id=(unsigned int) ((uint64_t)pthread_self());
    snmp_sess_init(&session);

    session.version   = SNMP_VERSION_2c;
    session.peername  = (char *)   ip_address.c_str();
    session.community = (u_char *) community.c_str();
    session.community_len = strlen(community.c_str());


    for (iter = oid_table.begin(); iter != oid_table.end(); iter++){

        assert(0==iter->first.compare(iter->second.oid));
        print_log(LOG_DEBUG,"Doing host=%s for oid %s\n",ip_address.c_str(),iter->first.c_str());


       rootlen = MAX_OID_LEN;
       //rootlen = strlen(iter->first.c_str());
       if (!read_objid(iter->first.c_str(), root, &rootlen)) {
       //if (!Read_objid_ts(iter->first.c_str(), root, &rootlen)) {

            //snmp_perror(argv[arg]);
            //exit(1);
          print_log(LOG_ERR,"Cannot parse the oid='%s' rootlen=%d  oid table get?\n",iter->first.c_str(),rootlen);
          snmp_perror("read_objid");
          return -1;
       }
       if(!(sp = snmp_sess_open(&session))){
          snmp_perror("snmp_open");
          print_log(LOG_WARNING,"[Thread %u] SNMP table builder, failed to open session to %s\n",thread_id,ip_address.c_str());

          return -1;
       }
////////////

  req = snmp_pdu_create(SNMP_MSG_GETNEXT);
  snmp_add_null_var(req, root, rootlen);

  status = snmp_sess_synch_response(sp,req,&resp);

  if(status == STAT_SUCCESS && resp->errstat == SNMP_ERR_NOERROR){
    char buf1[512];
    char *placeholder;
    int j;
    oid tmp[MAX_OID_LEN];
    char temp[MAX_OID_LEN];
    char ifIndexstr[MAX_OID_LEN];
    string value;
    uint64_t local_index;
 

    vars = resp->variables;

    //first = malloc(sizeof(struct Value_index_mapping));
    //assert(first!=NULL);

    //memset(buf1,'\0',sizeof(buf1));
    //memset(first->value,'\0',sizeof(first->value));
    //int toosmall = 
    snprint_value(buf1, sizeof(buf1), vars->name, vars->name_length, vars);
    placeholder = strstr(buf1,":");
    if(placeholder != NULL)
      placeholder = placeholder +2;
    if(strstr(placeholder,"\"")){
      placeholder = placeholder +1;
      //strncpy(first->value,placeholder,strlen(placeholder)-1);
      value=placeholder;
      value.resize(value.size()-1);
    }else{
      //strcpy(first->value,placeholder);
      value=placeholder;
    }



    memcpy(tmp,vars->name,vars->name_length * sizeof(oid));
    for(j=0;j<vars->name_length-rootlen;j++){
      if(j>0){
        i = sprintf(temp, ".%d", (int) tmp[rootlen+j]);
        strcat(ifIndexstr,temp);
      }else{
        i = sprintf(ifIndexstr, "%d", (int) tmp[rootlen+j]);
      }
    }
    //strcpy(first->index,ifIndexstr);
    temp_string2=ifIndexstr;

    local_index=atoll(temp_string2.c_str());
    assert(0==1); //this should never get here! under rev 3.1.0 initial
    //iter->second.indexof[value]=local_index;
    //iter->second.valueof[local_index]=value;
 
    //first->next = NULL;
    //current = first;
  }else{
    snprintf(snmp_err_str,SNMP_ERR_STR_SIZE-1,"Failure buidling snmp_table Hostname: %s snmp_sync_response",ip_address.c_str());
    snmp_perror(snmp_err_str);
    if(resp)
      snmp_free_pdu(resp);
    snmp_sess_close(sp);

    return -1;
  }



////////////////

       running=1;

       while(running==1) {
/////////
    oid tmp[MAX_OID_LEN];

    req = snmp_pdu_create(SNMP_MSG_GETNEXT);
    snmp_add_null_var(req,vars->name,vars->name_length);
    if(resp)
      snmp_free_pdu(resp);
    status = snmp_sess_synch_response(sp,req,&resp);

    if(status == STAT_SUCCESS && resp->errstat == SNMP_ERR_NOERROR){



      struct Value_index_mapping *tempIndex = NULL;
      char buf[512];
      char *placeholder;
      char ifIndexstr[MAX_OID_LEN];
      int j;
      char temp[MAX_OID_LEN];
      oid tmp[MAX_OID_LEN];
      string value_string;
      string index_string;
      int64_t local_index;
 
      vars = resp->variables;
      //tempIndex = malloc(sizeof(struct Value_index_mapping));  //why allocate a != struct?  
      //assert(tempIndex!=NULL);

      //memset(buf,'\0',sizeof(buf));
      //memset(tempIndex->value,'\0',sizeof(tempIndex->value));
      //will add a few extra bytes later, ensure we are covered
      snprint_value(buf, sizeof(buf)-5, vars->name, vars->name_length, vars);

      //printf("Raw Value = %s\n",buf);
      placeholder = strstr(buf,":");
      if(placeholder != NULL)
        placeholder = placeholder +2;
      if(strstr(placeholder,"\"")){
        placeholder = placeholder +1;
        //you assert on the size of the dest, not the origin
        assert(strlen(placeholder)+1<STD_STRING_SIZE);
        //strncpy(tempIndex->value,placeholder,strlen(placeholder));
        value_string=placeholder;
      }else{
        //strncpy(tempIndex->value,placeholder,STD_STRING_SIZE-1);
        value_string=placeholder;
        value_string.resize(value_string.size()-1);
      }

      memcpy(tmp,vars->name,vars->name_length * sizeof(oid));
      for(j=0;j<vars->name_length-rootlen;j++){
        if(j>0){
          i = sprintf(temp, ".%d",(int) tmp[rootlen+j]);
          strcat(ifIndexstr,temp);
        }else{
          i = sprintf(ifIndexstr, "%d",(int) tmp[rootlen+j]);
        }
      }
      //strcpy(tempIndex->index,ifIndexstr);
      index_string=ifIndexstr;

      local_index=atoll(index_string.c_str());
      assert(1==0); //this should never reach this under 3.1.0
      //iter->second.indexof[value_string]=local_index;
      //iter->second.valueof[local_index]=value_string;


      //print_log(LOG_DEBUG,"[Thread %u] Index = %u , Value = %s\n",thread_id,local_index,value_string.c_str());
      //current->next = tempIndex;
      //current = tempIndex;
      //current->next = NULL;
    }else{
      //snmp_perror("snmp_synch_response");
      //snprintf(snmp_err_str,SNMP_ERR_STR_SIZE-1,"[Thread %u] Hostname: %-15s snmp_sync_response",thread_id, hostInfo.name);
      snmp_perror(snmp_err_str);

      if(resp)
        snmp_free_pdu(resp);
      snmp_sess_close(sp);
      return -1;
    }

    //oid tmp[MAX_OID_LEN];
    memcpy(tmp,vars->name,vars->name_length * sizeof(oid));

    if(tmp[rootlen-1] != root[rootlen-1]){
      if(resp)
        snmp_free_pdu(resp);
      snmp_sess_close(sp);
      running=0;
      //done?
    }



//////////
       }      //end while


    }//end for each host

}

///////////////////////////////////////////////////////////////
/// Given a host loads all the oid mappings for that host
/// Uses snmp bulk get to minimize the time required for each
/// mapping.
int Host_Info::load_oid_table(){
    unsigned int thread_id;
    struct snmp_session session;
    
    map<string,OID_Value_Table>::iterator iter;
    int fvalue=0;
////

    //netsnmp_session session, *ss;
    int             numprinted = 0;
    int             reps = 15, non_reps = 0;

    netsnmp_pdu    *pdu, *response=NULL;
    netsnmp_variable_list *vars;
    int             arg;
    oid             name[MAX_OID_LEN];
    size_t          name_length;
    oid             root[MAX_OID_LEN];
    size_t          rootlen;
    int             count;
    int             running;
    int             status;
    int             check;
    int             exitval = 0;

    struct snmp_pdu *req, *resp;
    void *sp;


    char snmp_err_str[SNMP_ERR_STR_SIZE];
    char buf[512];
    char *placeholder;
    char buf2[512];
///
    string temp_string1,temp_string2;
    int i;
////////////    


    thread_id=(unsigned int)((uint64_t) pthread_self());
    simple_snmp_sess_init(&session);

    session.retries=4;
    session.version   = SNMP_VERSION_2c;
    //ARE THE NEXT TWO LINES  VALID???!
    session.peername  = (char *)   ip_address.c_str();
    session.community = (u_char *) community.c_str();
    session.community_len = strlen(community.c_str());
   
    
    for (iter = oid_table.begin(); iter != oid_table.end(); iter++){
        /// 
        assert(0==iter->first.compare(iter->second.oid));
        print_log(LOG_INFO,"Doing host=%s for oid %s\n",ip_address.c_str(),iter->first.c_str());  
 

       rootlen = MAX_OID_LEN;
       //rootlen = strlen(iter->first.c_str());
       if (!read_objid(iter->first.c_str(), root, &rootlen)) {
       //if (!Read_objid_ts(iter->first.c_str(), root, &rootlen)) {

            //snmp_perror(argv[arg]);
            //exit(1);
          print_log(LOG_ERR,"Cannot parse the oid='%s' rootlen=%d host=%s\n",iter->first.c_str(),rootlen,ip_address.c_str());
          snmp_perror("read_objid");
          return -1;
       }
       if(!(sp = snmp_sess_open(&session))){
          snmp_perror("snmp_open");
          print_log(LOG_WARNING,"[Thread %u] SNMP table builder, failed to open session to %s\n",thread_id,ip_address.c_str());

          return -1;
       }
       running=1;
   
       //why memmove and not memcpy? ask the netsmpbulk writers
       memmove(name, root, rootlen * sizeof(oid));
       name_length = rootlen;
       //should be a while
       while(running==1){
            /*
            * create PDU for GETBULK request and add object name to request 
             */
            pdu = snmp_pdu_create(SNMP_MSG_GETBULK);
            pdu->non_repeaters = non_reps;
            pdu->max_repetitions = reps;    /* fill the packet */
            snmp_add_null_var(pdu, name, name_length);

            /*
            * do the request 
            */
            //status = snmp_synch_response(sp, pdu, &response);
            status = snmp_sess_synch_response(sp,pdu,&response);
            switch (status){
                case STAT_SUCCESS:
                      if (response->errstat == SNMP_ERR_NOERROR) {
                          //Yay success!
                          
//////////////
                for (vars = response->variables; vars;
                     vars = vars->next_variable) {
                    if ((vars->name_length < rootlen)
                        || (memcmp(root, vars->name, rootlen * sizeof(oid))
                            != 0)) {
                        /*
                         * not part of this subtree 
                         */
                        running = 0;
                        continue;
                    }
                    numprinted++;
                    //print_log(LOG_DEBUG,"num_printed=%d\n",numprinted);
                    //print_variable(vars->name, vars->name_length, vars);
                    if ((vars->type != SNMP_ENDOFMIBVIEW) &&
                        (vars->type != SNMP_NOSUCHOBJECT) &&
                        (vars->type != SNMP_NOSUCHINSTANCE)) {
                        /*
                         * not an exception value 
                         */
/*
                        if (check
                            && snmp_oid_compare(name, name_length,
                                                vars->name,
                                                vars->name_length) >= 0) {
                            fprintf(stderr, "Error: OID not increasing: ");
                            fprint_objid(stderr, name, name_length);
                            fprintf(stderr, " >= ");
                            fprint_objid(stderr, vars->name,
                                         vars->name_length);
                            fprintf(stderr, "\n");
                            running = 0;
                            exitval = 1;
                        }
*/
                        snmp_err_str[0]=0;
                        snprint_objid(snmp_err_str,SNMP_ERR_STR_SIZE-1,vars->name,vars->name_length);
                        snprint_value(buf, sizeof(buf)-5, vars->name, vars->name_length, vars);
                        
                        //print_log(LOG_DEBUG,"[Thread %u] '%s'='%s'\n",thread_id,snmp_err_str,buf);
                       
                        temp_string1=snmp_err_str;
                        size_t found;
                        found=temp_string1.find_last_of("."); 
                        temp_string2=temp_string1.substr(found+1);
                  
                        string search_string;
                        found=iter->first.find_last_not_of(".0123456789");
                        if(found==iter->first.npos){
                          //not found the search string is the whole thing!
                          search_string=iter->first;  
                        } 
                        else{
                          search_string=iter->first.substr(found);
                        }
                        search_string=iter->first.substr((iter->first.length()*2)/3);
                        string suffix_str;
                        //iterate over the data.
                        found=temp_string1.find(iter->first);
                        found=temp_string1.find(search_string);
                        print_log(LOG_DEBUG,"[Thread %u] [host %s] found=%u first=%s temp_string1=%s search_str=%s\n" , 
                              thread_id,ip_address.c_str(), found,iter->first.c_str(),temp_string1.c_str(),search_string.c_str());
                        if(temp_string1.npos!=found){
                           //print_log(LOG_INFO,"[Thread %u] [host %s] found!\n",thread_id,ip_address.c_str());
                           //suffix_str=temp_string1.substr(found+iter->first.length()+1); 
                           suffix_str=temp_string1.substr(found+search_string.length()+1);    
                           //print_log(LOG_INFO,"[Thread %u] found=%u first=%s temp_string1=%s\n" , thread_id,found,iter->first.c_str(),temp_string1.c_str());             
                           print_log(LOG_DEBUG,"[Thread %u] [host %s] found =%s!\n",thread_id,ip_address.c_str(),suffix_str.c_str());
                        }
                        else{
                           print_log(LOG_DEBUG,"[Thread %u] [host %s] NOT found!\n",thread_id,ip_address.c_str());
                           found=temp_string1.find_last_of(".");
                           suffix_str=temp_string1.substr(found+1);
                        }

                        
                        //print_log(LOG_DEBUG,"[Thread %u] index='%s'\n",thread_id,temp_string2.c_str());
                        uint64_t local_index;
                        local_index=atoll(temp_string2.c_str());

                        //printf("Raw Value = %s\n",buf);

                        temp_string2=extract_value_from_raw(buf);
                        //print_log(LOG_DEBUG,"[Thread %u] index=%lu value='%s' \n",thread_id,local_index,buf2);
                        //iter->second.indexof[temp_string2]=local_index;
                        //iter->second.valueof[local_index]=temp_string2; 
                        iter->second.suffix_of[temp_string2]=suffix_str;
                        iter->second.value_of[suffix_str]=temp_string2; 
                        print_log(LOG_DEBUG,"[Thread %u] [host %s] suffix_of[%s]='%s' \n",thread_id,ip_address.c_str(),temp_string2.c_str(),suffix_str.c_str());
                        

                        /*
                         * Check if last variable, and if so, save for next request.  
                         */
                        if (vars->next_variable == NULL) {
                            memmove(name, vars->name,
                                    vars->name_length * sizeof(oid));
                            name_length = vars->name_length;
                        }
                    } else {
                        /*
                         * an exception value, so stop 
                         */
                        running = 0;
                    }
                }
 

////////////////
                      }
                      else{
                        ///Error in response, report and exit loop
                        running=0;
                        snprintf(snmp_err_str,SNMP_ERR_STR_SIZE-1,"[Thread %u] Hostname: %-15s snmp_sync_response",thread_id, ip_address.c_str());
                        snmp_perror(snmp_err_str);

                      }
                      break;
                case STAT_TIMEOUT:
                        print_log(LOG_NOTICE,"[Thread %u] SNMP timeout(building table), host=%15s \n",thread_id, ip_address.c_str() );
                        running=0;
                      break;
                default:
                      //other error!
                         print_log(LOG_ERR,"SNMP MISC error\n");
                        running=0;
                      break;
            }
            if (response){
               snmp_free_pdu(response);
               response=NULL;
            }
            
            if(0==iter->second.suffix_of.size()){
              print_log(LOG_WARNING,"[Thread %u][host %s] no data inserted for %s\n",thread_id,ip_address.c_str(),iter->first.c_str());
              //fvalue=-1;
            }            

            //print_log(LOG_DEBUG,"[Thread %u] inserted %d values\n" ,thread_id,iter->second.indexof.size());
         
       }//end while

       if (response){
            snmp_free_pdu(response);
            response=NULL;
       }



       //is this the best place to clse it?
       snmp_sess_close(sp);
 
    }//end for
    return fvalue;
}

void *Host_Info_worker_thread_func(void *in_class){
              Host_Info *local_class;
              local_class=(Host_Info *)in_class;
              local_class->thread_start();
             return NULL;
}






//////////////////////////////////////////////////-----------
pthread_mutex_t Host_Info_Table::lock        = PTHREAD_MUTEX_INITIALIZER;

Host_Info * Host_Info_Table::get_host_for_db_id(uint32_t db_id){
    int rvalue=-1;
    map<int32_t,uint32_t>::iterator location;
    Host_Info *host_loc=NULL;  
 
    rvalue=pthread_mutex_lock(&lock);
    assert(0==rvalue);

    location=db_id_to_index.find(db_id);
    if(db_id_to_index.end()!=location){
       host_loc=&host[location->second]; 
    }    


    rvalue=pthread_mutex_unlock(&lock);
    assert(0==rvalue);


   return host_loc;
}

///assumes in class has sane values!
int  Host_Info_Table::insert_new_host(class Host_Info &inclass ){
    int rvalue=-1; 
    int position=-1;   
    map<int32_t,uint32_t>::iterator location;    

    rvalue=pthread_mutex_lock(&lock);
    assert(0==rvalue);
    
    //if not found
    location=db_id_to_index.find(inclass.db_id);
    if(db_id_to_index.end()==location){
       position=host.size();
       host.push_back(inclass);
       db_id_to_index[inclass.db_id]=position;
    }    
    else{
       position=location->second;
    }
    

    rvalue=pthread_mutex_unlock(&lock);
    assert(0==rvalue);
 
    return position;
}


//////////
int  Host_Info_Table::load_host_tables(sem_t *thread_semaphore){
    ///
    int i;
    int rvalue;
    pthread_attr_t attr;
    const int thread_stack_size_bytes=256*1024;
    vector<pthread_t> tvector; 
    int num_loaded=0;

    for(i=0;i<host.size();i++){
       host[i].thread_semaphore=thread_semaphore;
    }
    //prepare thread stuff
    rvalue=pthread_attr_init(&attr);
    assert(0==rvalue);
    rvalue = pthread_attr_setstacksize(&attr, thread_stack_size_bytes);
    assert(0==rvalue);
    //rvalue = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    rvalue = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    assert(0==rvalue);
   
    tvector.reserve(host.size());

    for(i=0;i<host.size();i++){
       rvalue=sem_wait(thread_semaphore);
       assert(0==rvalue);
       rvalue=pthread_create(&tvector[i],
                             &attr,
                             Host_Info_worker_thread_func,
                             (void *) &host[i]);
       assert(0==rvalue);
         
    }
    print_log(LOG_DEBUG,"Done creating\n");
    //actually I should join them all!
    for(i=0;i<host.size();i++){
        rvalue=pthread_join(tvector[i],NULL);
    }
    for(i=0;i<host.size();i++){
        if(host[i].mapping_done==1){
           num_loaded++;
        }
    }

    print_log(LOG_INFO,"Done loading tables, loaded %d out of %d\n", num_loaded,host.size());

   return 0;
}

