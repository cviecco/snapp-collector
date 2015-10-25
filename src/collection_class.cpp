
#include "config.h"
#include "rrd_writer.h"
#include "sql-collector.hpp"
#include "collection_class.hpp"

#include <ctype.h>
#include <assert.h>

//////////////////////////////////////////////////-----------
////////////////////////////////////////////////////////////
pthread_mutex_t OID_mapping_sequence_db::lock        = PTHREAD_MUTEX_INITIALIZER;


OID_mapping_sequence *OID_mapping_sequence_db::get_sequence_by_db_id(uint32_t db_id){
    int rvalue=-1;
    map<int,int>::iterator location;
    OID_mapping_sequence *mapping_loc=NULL;

    rvalue=pthread_mutex_lock(&lock);
    assert(0==rvalue);
    
    //begin critical section
    location=db_id_to_index.find(db_id);
    if(db_id_to_index.end()!=location){
       mapping_loc=&mapping[location->second];
    }
    //end critical section

    rvalue=pthread_mutex_unlock(&lock);
    assert(0==rvalue);

   return mapping_loc;
}


OID_mapping_sequence *OID_mapping_sequence_db::get_sequence_by_name(string name){
    int rvalue=-1;
    map<string,int>::iterator location;
    OID_mapping_sequence *mapping_loc=NULL;

    rvalue=pthread_mutex_lock(&lock);
    assert(0==rvalue);

    //begin critical section
    location=name_to_index.find(name);
    if(name_to_index.end()!=location){
       mapping_loc=&mapping[location->second];
    }

    //end critical section

    rvalue=pthread_mutex_unlock(&lock);
    assert(0==rvalue);

   return mapping_loc;
}

///assumes in class has sane values!
int  OID_mapping_sequence_db::insert_new_sequence(class OID_mapping_sequence &inclass ){
    int rvalue=-1;
    int position=-1;
    map<int,int>::iterator location;

    rvalue=pthread_mutex_lock(&lock);
    assert(0==rvalue);

    //if not found
    location=db_id_to_index.find(inclass.db_id);
    if(db_id_to_index.end()==location){
        position=mapping.size();
        mapping.push_back(inclass);
        db_id_to_index[inclass.db_id]=position;
        name_to_index[inclass.name]=position;
    }
    else{
       position=location->second;
    }


    rvalue=pthread_mutex_unlock(&lock);
    assert(0==rvalue);

    return position;


}

/////////////////////////////////////////////
int SQL_Host::report_mappings(){
   unsigned int i;
   uint32_t failure_count=0;
   const uint32_t max_failures_reported=10;    


   for(i=0;i<collection.size();i++){
        if( 0==collection[i].post_mapping_oid_suffix.length() && -1!=collection[i].oid_mapping_sequence_db_id ){
          failure_count++;
      }
   }
   if(failure_count==collection.size()){
       print_log(LOG_WARNING,"[host %s] All mappings failed!\n", host->ip_address.c_str());
       return failure_count;
   }

   failure_count=0; 
   for(i=0;i<collection.size();i++){
        if( 0==collection[i].post_mapping_oid_suffix.length() && -1!=collection[i].oid_mapping_sequence_db_id && failure_count<max_failures_reported){
            print_log(LOG_WARNING,"[host %s] collection[%i] premapping=%s failure (mapping db_id=%d)\n",
                     host->ip_address.c_str(),i,collection[i].premaping_oid_suffix.c_str(),collection[i].oid_mapping_sequence_db_id);
            failure_count++;
        }
   }
   return failure_count;
}


//////////////////////////////
//'lifted from the old snapp collector:



/**
  Should return a string with the value to be inserted into an rrd_string.
  That is it should be a numeric string!.
*/
string get_clean_snmpvalue(char *snmp_value_string){
    char *buf;
    char *the_value=NULL;    
    char minus_one_str[]="-1";
    string output;
    //uint64_t fvalue;


    buf=snmp_value_string;

     
    //print_log(LOG_DEBUG,"[thread %i] RAW VALUE=%s RAW NAME=%s\n",tindex,buf,buf2);

    if(strstr(buf,"Timeticks") != NULL){
       //if we get a Timeticks variable (like uptime),
       // grab the numbers between (...)
       char *saveptr;
       the_value = strtok_r(buf,"(",&saveptr);
       the_value = strtok_r(NULL,")",&saveptr);
       if(the_value != NULL){
            if(strstr(the_value,"null")){
              the_value = minus_one_str;
            }
       }else{
            the_value = minus_one_str;
       }
    }else if(strstr(buf,"No Such Instance") != NULL) {
          strcpy(buf,"0");
          the_value = buf;
    }else if(strstr(buf,"No Such Object") != NULL){
          strcpy(buf,"0");
          the_value = buf;
    }else if(strstr(buf,":") == NULL) {
          print_log(LOG_DEBUG,"GOT A 'nocolon': %s\n",buf);
          the_value = strstr(buf,"=");
          the_value = the_value + 2;
    }else{
          the_value = strstr(buf,":");
          if(the_value != NULL)
            the_value = the_value + 2;
    }
    output=the_value;
    
    //now do the second chance matchings.
    if(0==isdigit(*the_value)){
       //is not a digit?
       if(strstr(the_value, "up(1)") != NULL){
         output="1";
       }
       else if(  strstr(the_value, "down(2)") != NULL ||
                 strstr(the_value, "lowerLayerDown(7)") != NULL || 
                 strstr(the_value, "No Such Instance currently exists at this OID") != NULL){
         output="0";
       }
       else {
         output="U";
       }
      
    } 

    return output;
   

   
}





//////////////

/**
** assumes mappings have been resolved.
*/

int Collection_worker::get_collect(int host_index){
    unsigned int i,j;
    SQL_Host *host;
    unsigned int thread_id;
    //struct snmp_session session;
    int fvalue=0;
    int rvalue;   
 

////////
    void *sp;

    struct snmp_pdu *req=NULL, *response=NULL;
    oid             root[MAX_OID_LEN];
    size_t          rootlen;
    oid timeticks[] = {1,3,6,1,2,1,1,3,0};
    string          oid_string;
    
    int             status;
    char snmp_err_str[SNMP_ERR_STR_SIZE];
    netsnmp_variable_list *vars;
    int numprinted=0;
    char buf[512];
    char buf2[512];

     string temp_string1,temp_string2;
     vector<string> snmp_val;
//

    //inits
    host=&parent->host[host_index];
    thread_id=(unsigned int) ((uint64_t) pthread_self());
    //snmp_sess_init(&session);
   
    session.version   = SNMP_VERSION_2c;
    //ARE THE NEXT TWO LINES  VALID???!
    session.peername  = (char *)   host->host->ip_address.c_str();
    session.community = (u_char *) host->host->community.c_str();
    session.community_len = strlen(host->host->community.c_str());
    

    if(!(sp = snmp_sess_open(&session))){
        snmp_perror("snmp_open");
        print_log(LOG_WARNING,"[Thread %u] SNMP table builder, failed to open session to %s\n",thread_id,host->host->ip_address.c_str());

        return -1;
    }

    //iterate over each collection
    for(i=0;i<host->collection.size();i++){
        assert(response==NULL);

        //step 0.1 check if mapping exists!
        if(0==host->collection[i].post_mapping_oid_suffix.length() && -1!=host->collection[i].oid_mapping_sequence_db_id){
           print_log(LOG_INFO,"Failed mappings for host %s, suffix_len=%d mapping_sequence_id=%d collection[%d].premap=%s total_collections=%d skipping collection\n",host->host->ip_address.c_str(),host->collection[i].post_mapping_oid_suffix.length(),(int) host->collection[i].oid_mapping_sequence_db_id,i,host->collection[i].premaping_oid_suffix.c_str(),host->collection.size() );
           //goto done;
           goto end_of_single_collection;
        }    
        print_log(LOG_DEBUG,"[thread %u] [host %s]  suffix='%s' mapping_sequence_id=%d collection[%d].premap=%s total_collections=%d mapping sucess\n",thread_id,host->host->ip_address.c_str(),host->collection[i].post_mapping_oid_suffix.c_str(),(int) host->collection[i].oid_mapping_sequence_db_id,i,host->collection[i].premaping_oid_suffix.c_str(),host->collection.size() );


        //step 0: create pdu
        req =NULL;  
        req = snmp_pdu_create(SNMP_MSG_GET);//who deallocates this one?
        assert(NULL!=req);


 
        //step 1: fill the pdu
        for(j=0;j<parent->oid.size();j++){
            oid_string=parent->oid[j].oid_prefix;
            if(0!=host->collection[i].post_mapping_oid_suffix.length()){
              oid_string.append(".");
              oid_string.append(host->collection[i].post_mapping_oid_suffix);
            }

            rootlen = MAX_OID_LEN;
            if (!read_objid(oid_string.c_str(), root, &rootlen)) {
               print_log(LOG_ERR,"Cannot parse the oid='%s' rootlen=%d host=%s premap_suffix=%s\n",
                             oid_string.c_str(),rootlen,host->host->ip_address.c_str(),host->collection[i].premaping_oid_suffix.c_str());
               snmp_perror("read_objid");
  
               snmp_free_pdu(req);
               req=NULL;
               fvalue= -1;

               goto end_of_single_collection;
               //goto done;
            }
            //print_log(LOG_DEBUG,"[thread %u][host %s] new oid val=%s\n",
            //                      thread_id,host->host->ip_address.c_str(),oid_string.c_str());
            snmp_add_null_var(req, root, rootlen);  //need to check for errors on this one           
            //snmp_add_null_var(req, op->Oid, op->OidLen);
           
        }
        //add the timeticks  
        snmp_add_null_var(req,timeticks,9);

 
        //step 2: do the request.        
        status = snmp_sess_synch_response(sp,req,&response);

        //step 3: handle the request
        numprinted=0;
        snmp_val.clear();
        switch (status){
            case STAT_SUCCESS:
                //yay we got something!
                
                if (response->errstat == SNMP_ERR_NOERROR) {
                   //we can now actually do some processing!                  
 
                    //////////////////////////////////
                    //steb 3b. stringigy the the response variables
                    for (vars = response->variables; vars;
                         vars = vars->next_variable) {
                    
                        //print_log(LOG_DEBUG,"num_printed=%d\n",numprinted);
                        //print_variable(vars->name, vars->name_length, vars);
                        if ((vars->type != SNMP_ENDOFMIBVIEW) &&
                            (vars->type != SNMP_NOSUCHOBJECT) 
                            //&& (vars->type != SNMP_NOSUCHINSTANCE)
                            ) {
                             /*
                             * not an exception value 
                             */
                            snmp_err_str[0]=0;
                            snprint_objid(snmp_err_str,SNMP_ERR_STR_SIZE-1,vars->name,vars->name_length);
                            snprint_value(buf, sizeof(buf)-5, vars->name, vars->name_length, vars);
                            strncpy(buf2,buf,sizeof(buf2));                   
                      
                            if(vars->type != SNMP_NOSUCHINSTANCE){
                               temp_string1= get_clean_snmpvalue(buf);
                            }
                            else{ 
                               temp_string1= "NAN";//uknown!
                            }
                            snmp_val.push_back(temp_string1);
                            
                            //print_log(LOG_DEBUG,"[Thread %u][host %15s] '%s'='%s' clean='%s'\n",
                            //                   thread_id,host->host->ip_address.c_str(),
                            //                   snmp_err_str,buf2,temp_string1.c_str());



                        } else {
                            /*
                             * an exception value, so stop 
                             */
                            //running = 0;
                            fvalue=-1;
                            print_log(LOG_INFO,"[Thread %u][host %15s] GOT  SNMP failure! (type=%d) \n", 
                                                thread_id,host->host->ip_address.c_str(),vars->type);
                            goto end_of_single_collection;
                            //goto done;
                        }

                    }


                    ////////////////////////////
                    //step 4 build rrd insertion string if things seem OK
                    if(snmp_val.size() ==  parent->oid.size()+1 ){
                        unsigned int tt = atoi(snmp_val[parent->oid.size()].c_str());
                        unsigned int totaltime;
                        string rrd_template,rrd_values;                        
                        double finaltime;

                        if(tt == -1){
                           print_log(LOG_ERR,"[thread %d] YIKES!! TimeTicks can't be NULL bad manufacturer\n",thread_id);
                           goto done;
                        }
                        //time ticks is an int

                        ///////////////
                        //print_log(LOG_DEBUG,"[thread %d] OldTimeTicks = %d TimeTicks = %d\n",thread_id,host->timeticks,tt);
                        //if the timeticks have been reset or are 0
                        if(tt <= host->timeticks || host->timeticks == 0 || (tt - host->timeticks) > 6000){
                            host->timeticks = tt;
                            host->mytime = time(NULL);
                            //print_log(LOG_DEBUG,"[thread %d] Localtime = %d TimeTicks = %d\n",host->mytime,host->timeticks);
                        }


                        totaltime = tt - host->timeticks;
                        totaltime = totaltime / 100;

                        //print_log(LOG_DEBUG,"[thread %d] timeticks since localtime reset = %d\n",thread_id,totaltime);
                        //sprintf(value_str,"%d",(host->mytime + totaltime));
 
                         //////////
                        rrd_values  =snmp_val[0];
                        rrd_template=parent->oid[0].ds_name;
                        for(j=1;j<parent->oid.size();j++){
                           rrd_values.append(":");
                           rrd_values.append(snmp_val[j]);
                           rrd_template.append(":");
                           rrd_template.append(parent->oid[j].ds_name);
                        }

                        //step 5 do rrd update.
                        string full_filename=parent->rrd_dir;
                        full_filename.append(host->collection[i].rrd_filename);
                       // print_log(LOG_DEBUG,"[thread %u] before push data, time=%f, mytime=%d \n",thread_id,host->mytime*1.0 + totaltime*1.0,host->mytime);

                        finaltime=((host->mytime*1.0 + totaltime*1.0));
                        print_log(LOG_DEBUG,"[thread %u] before push data, time=%f, mytime=%u \n",thread_id,finaltime,host->mytime);
                        rvalue=push_data(finaltime,full_filename.c_str(),rrd_template.c_str(), rrd_values.c_str());
                        if(rvalue<0){
                           print_log(LOG_ERR,"[thread %u][host %15s] Error pushing data to queue! ",thread_id,host->host->ip_address.c_str());
                        }
                    }
                    else{
                        print_log(LOG_WARNING,"[thread %u][host %15s] The matched variables do not match the request size<F9> ",thread_id,host->host->ip_address.c_str());
                    }


                }
                else{
                  //error in response, report and end 
                   snprintf(snmp_err_str,SNMP_ERR_STR_SIZE-1,"[Thread %u] Hostname: %-15s snmp_sync_response",thread_id, host->host->ip_address.c_str());
                   snmp_perror(snmp_err_str);
                }
                break;
            case STAT_TIMEOUT:
                print_log(LOG_INFO,"[Thread %u] SNMP timeout, host=%15s \n",thread_id,host->host->ip_address.c_str() );
                fvalue=-1;
                goto done;
                break;

            default:
                //other error!
                print_log(LOG_WARNING,"SNMP MISC error\n");
                fvalue=-1;
                goto done;
                break;
        }
end_of_single_collection:
       if (response){
            snmp_free_pdu(response);
            response=NULL;
       }


        
    }//end for each collection
   
    //cleanup
done:
    if (response){
       snmp_free_pdu(response);
       response=NULL;
    }


    snmp_sess_close(sp);

    return fvalue;
}


int Collection_worker::thread_start(){
    int collection_index;
    int rvalue;

    assert(parent!=NULL);
    assert(parent->session_semaphore!=NULL);
    simple_snmp_sess_init(&session);

    while(1==1){
        //wait for semaphore
        rvalue=sem_wait(&parent->worker_semaphore);
        assert(0==rvalue);
        
        //
        rvalue=pthread_mutex_lock(&(parent->lock));
        assert(0==rvalue);
 
        collection_index=parent->pending_host_index_queue.front();
        parent->pending_host_index_queue.pop();

        rvalue=pthread_mutex_unlock(&(parent->lock));
        assert(0==rvalue);

        if(-1==collection_index){
          return 0;
        }
        //else we have some work to to!
      
        rvalue=sem_wait(parent->session_semaphore);
        assert(0==rvalue);
        get_collect(collection_index);
        rvalue=sem_post(parent->session_semaphore);
        assert(0==rvalue);

    }

    return 0; 
}


void *Collection_class_worker_thread_func(void *in_class){
              Collection_worker *local_class;
              local_class=(Collection_worker *)in_class;
              local_class->thread_start();
             return NULL;
}

////////////////////////////////////////////////////////////////
void *Collection_class_main_thread_func(void *in_class){
              SQL_Collection_Class *local_class;
              local_class=(SQL_Collection_Class *)in_class;
              local_class->thread_start();
             return NULL;
}




///////////////////////////////////////////////////////////
int SQL_Collection_Class::initialize(){
   int rvalue=-1;
   const int max_num_workers=200; 
   int num_workers; 
   unsigned int i;
   //uint32_t interval=10;
   Collection_worker *current_worker;
   pthread_attr_t attr;
   const int thread_stack_size_bytes=512*1024;


   print_log(LOG_DEBUG,"[Thread %d] initializing collection_class %p\n ",0,this);

   if(1==initialized){
      print_log(LOG_DEBUG,"[Thread %d] Double initialization? collection_class %p\n ",0,this);
      return 0;
   }
   num_workers=max_num_workers;
   if(num_workers>host.size()){
      num_workers=host.size();
   }

   //do worker status
   for(i=0;i<host.size();i++){
      host[i].report_mappings();
   }


   /* Initialize and set thread detached attribute */
   rvalue=pthread_attr_init(&attr);
   assert(0==rvalue);
   rvalue = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   //rvalue = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   assert(0==rvalue);
   rvalue = pthread_attr_setstacksize(&attr, thread_stack_size_bytes);
   assert(0==rvalue);

   //
   assert(session_semaphore!=NULL);

   //setup numworkers.
   worker.resize(num_workers);
   for(i=0;i<worker.size();i++){
      worker[i].parent=this;
      //create the threads...
   

       current_worker=&worker[i];

       rvalue=pthread_create(&(worker[i].thread_info),
                             &attr,
                             Collection_class_worker_thread_func,
                             (void *) current_worker);

       assert(0==rvalue);
       
   
   }
   //now create the main worker!
   rvalue = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   assert(0==rvalue);
   rvalue=pthread_create(&main_thread,
                             &attr,
                             Collection_class_main_thread_func,
                             (void *) this);

   assert(0==rvalue);

   print_log(LOG_DEBUG,"[Thread %d] initialized collection_class %p\n ",0,this);

   initialized=1;
};

int SQL_Collection_Class::thread_start(){
   struct timespec delay,leftover;  
   struct timeval hr_time[3];
   unsigned int i;
   int rvalue;
   double adjust=1.0001;

   while(1){
        print_log(LOG_INFO,"[thread %u] Starting new iteration of collection_class\n",pthread_self());
        rvalue=pthread_mutex_lock(&lock);
        assert(0==rvalue);

        if(!pending_host_index_queue.empty()){
            print_log(LOG_WARNING,"[thread %u] WTF?: queue not empty!\n",pthread_self());
            goto end_critical_section;
        }
        for(i=0;i<host.size();i++){
            pending_host_index_queue.push(i);
            rvalue=sem_post(&worker_semaphore);
            assert(0==rvalue);
        }
/*
        collection_index=parent->pending_host_index_queue.front();
        parent->pending_host_index_queue.pop();
*/
end_critical_section:
        rvalue=pthread_mutex_unlock(&lock);
        assert(0==rvalue);
       
       print_log(LOG_INFO,"[thread %u] after sending suff (collection class) interval=%d\n",pthread_self(),interval);
      
       rvalue=gettimeofday(&hr_time[1],NULL);

       //Yes thi is wrong!, I need to actally calculate the time spent on this function.
       delay.tv_sec=interval*adjust;
       delay.tv_nsec=0;
       rvalue=nanosleep(&delay,&leftover); 
       //sleep(interval); 
       print_log(LOG_INFO,"[thread %u] adter the call rvalue=%d los=%d\n",pthread_self(),rvalue,leftover.tv_sec);
       rvalue=gettimeofday(&hr_time[2],NULL);
       if (labs((hr_time[2].tv_sec-hr_time[1].tv_sec)-interval)>2){
           adjust=(interval*1.0)/(1.0*(hr_time[2].tv_sec-hr_time[1].tv_sec));
           if(adjust<0.1 || adjust>1.001){
              print_log(LOG_WARNING,"[thread %u] Absurd adjust calculation got=%f\n",pthread_self(),adjust);
              adjust=1.001;
           }
           print_log(LOG_WARNING,"[thread %u] Nanosleep requrires adjusting new adjust=%f\n",pthread_self(),adjust);
       }
   }
}

int SQL_Collection_Class::finalize(){
   int rvalue;
   unsigned int i;

   if (0==initialized){
      
      return 0;
   }
   print_log(LOG_DEBUG,"Finalizing collection class[%s] %p\n",name.c_str(),this);

   rvalue=pthread_mutex_lock(&lock);
   assert(0==rvalue);
   
   //guess if there is a main thread and kill it
   if(0!=main_thread){
       print_log(LOG_DEBUG,"Canceling main thread\n");
       rvalue=pthread_cancel(main_thread);
       assert(0==rvalue);
   }
   else{
      print_log(LOG_DEBUG,"No main thread to cancel\n");
   }
   print_log(LOG_DEBUG,"Finalizing collection class[%s] (main thread finalized) %p\n",name.c_str(),this);
   while(! pending_host_index_queue.empty()){
      pending_host_index_queue.pop();
   }
   for(i=0;i<worker.size();i++){  
       //rvalue=pthread_cancel(worker[i].thread_info);
        pending_host_index_queue.push(-1);
        rvalue=sem_post(&worker_semaphore);
        assert(0==rvalue);

       //assert(0==rvalue);
   } 
   rvalue=pthread_mutex_unlock(&lock);
   assert(0==rvalue);

   print_log(LOG_DEBUG,"Finalizing collection , joining helpers\n");
   for(i=0;i<worker.size();i++){ 
       rvalue=pthread_join(worker[i].thread_info, NULL);
       // pending_host_index_queue.push(-1);
       assert(0==rvalue);
   }

   print_log(LOG_DEBUG,"Finalizing collection classp[%s] (worker threads finalized) %p\n",name.c_str(),this);
   //worker.clear();
   initialized=0;
//   rvalue=pthread_mutex_unlock(&lock);
//   assert(0==rvalue);

   return 0;
}

SQL_Collection_Class::SQL_Collection_Class(){
   int rvalue;
   
   initialized=0;
   main_thread=0;

   //init syncronization structures
   print_log(LOG_DEBUG,"Calling collection class constructor\n");
   rvalue=sem_init(&worker_semaphore, 0, 0);
   assert(0==rvalue);
   rvalue=pthread_mutex_init(&lock, NULL);
   assert(0==rvalue);

}

SQL_Collection_Class::~SQL_Collection_Class(){
   print_log(LOG_DEBUG,"Calling collection class descructor\n");
   finalize();
}


