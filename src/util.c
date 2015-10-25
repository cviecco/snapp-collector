
#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <pcre.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <assert.h>

#define LOG_RE_MATCH_STRING_SIZE 512
#define PCRE_EXEC_OVECCOUNT       30

int syslog_ready=0;
int log_level=LOG_NOTICE;
pthread_mutex_t log_level_mutex    =PTHREAD_MUTEX_INITIALIZER;               

pthread_rwlock_t        pcre_log_lock = PTHREAD_RWLOCK_INITIALIZER;
pcre *log_re=NULL;
pcre_extra *log_re_extra=NULL;

/**
* call this to do the logging, logs to syslog or stderr depending
* on runtime parameters.
*/
void print_log(int priority, const char *string, ...){
 
  va_list arg;
  va_start(arg,string);
  int rvalue;
  char check_string[LOG_RE_MATCH_STRING_SIZE];
  int ovector[PCRE_EXEC_OVECCOUNT];
  int rc;


  check_string[LOG_RE_MATCH_STRING_SIZE-1]=0;
  vsnprintf(check_string, LOG_RE_MATCH_STRING_SIZE-1, string, arg);

  //get a rd lock on to check the pcre stuff
  rvalue=pthread_rwlock_rdlock(&pcre_log_lock);
  assert(0==rvalue);
  if(log_re!=NULL){
     //check_string[LOG_RE_MATCH_STRING_SIZE-1]=0; //make sure the string ends.
     //memset(check_string,0x00,LOG_RE_MATCH_STRING_SIZE); //clear the string?
     //vsnprintf(check_string, LOG_RE_MATCH_STRING_SIZE-1, string, arg); 

     rc = pcre_exec(
           log_re,               //result of pcre_compile() 
           NULL,                 // The studied the pattern 
           check_string,         // the subject string 
           #if defined _GNU_SOURCE || _POSIX_C_SOURCE >= 200809L
           strnlen(check_string,LOG_RE_MATCH_STRING_SIZE/2), // the length of the subject string 
           #else
           strlen(check_string),
           #endif
           0,                    // start at offset 0 in the subject 
           0,                    // default options 
           ovector,              // vector of integers for substring information 
           PCRE_EXEC_OVECCOUNT); // number of elements (NOT size in bytes) 
    if(rc>=0){  ///we had a match!
       priority=LOG_ERR; //pretend is an err and this must be logged!
    }

  }
 
  rvalue=pthread_rwlock_unlock(&pcre_log_lock);
  assert(0==rvalue);

 
  if(syslog_ready!=0){
    //vsyslog(priority,string,arg);
    syslog(priority,"%s",check_string);
  }
  else{
     if(priority<=log_level ){
        //vfprintf(stderr,string,arg);
        fprintf(stderr,"%s",check_string);
     }
  }
}

/**
 get the current log level
*/
int  get_log_level(){
  int rvalue;
  int fvalue;
  rvalue=pthread_mutex_lock(&log_level_mutex);
  assert(0==rvalue);
  fvalue=log_level;
  rvalue=pthread_mutex_unlock(&log_level_mutex);
  assert(0==rvalue);
  return fvalue;
}

/**
  Set the current log level
*/

int  set_log_level(int new_level){
  int rvalue;
  if(new_level<0 || new_level>=8){
    return -1;
  } 
  rvalue=pthread_mutex_lock(&log_level_mutex);
  assert(0==rvalue);
  log_level=new_level;
  //if(0!=syslog_ready){
      setlogmask(LOG_UPTO(new_level));
  //}
  rvalue=pthread_mutex_unlock(&log_level_mutex);
  assert(0==rvalue);
  return new_level;

}



/**
*  do an fdatasync on a file name not a file descriptor.
*/
int  file_datasync(const char *filename){
   return file_datasync_drop_caches(filename,0);
}

int  file_drop_from_caches(const char *filename){
   return file_datasync_drop_caches(filename,1);
}


int  file_datasync_drop_caches(const char *filename,int drop_from_cache){
  int fd;
  int rvalue;  
  int fvalue=0; 

  fd=open(filename,O_RDWR);
  if (fd<0){
    print_log(LOG_ERR,"Could not open %s for flushing!\n",filename);
    return -1;
  }
  #if  _POSIX_C_SOURCE >= 199309L  
  rvalue=fdatasync(fd);
  #else
  rvalue=fsync(fd);
  #endif
  if(rvalue<0){
    print_log(LOG_ERR,"Could not flush %s\n",filename);
    fvalue=-1;
  }
  if(1==drop_from_cache){
     //The conditions should be better.
     #ifdef POSIX_FADV_DONTNEED  
     rvalue= posix_fadvise(fd, 0, 0,  POSIX_FADV_DONTNEED);
     #else
     rvalue=0;
     #endif
     if(rvalue<0){
        print_log(LOG_ERR,"Could not flush %s from system cache\n",filename);
        fvalue=-1;
     }
  }
  do{
     //try realy hard to close the fd.
     //even on signals keep trying!
     rvalue=close(fd);
  }while(rvalue<0 && EINTR==errno);
  if(rvalue<0){
    print_log(LOG_ERR,"Could not close %s\n",filename);
    fvalue=-1;
  }
  return fvalue;  

}

/**
  The same as an snmp_sess_init minus the net_snmp init.
  Somehow, netsnmp was trying to reinialize itself causing
  a memory leak.
*/
void simple_snmp_sess_init(netsnmp_session *session){
  memset(session, 0, sizeof(netsnmp_session));
  session->remote_port = SNMP_DEFAULT_REMPORT;
  session->timeout = SNMP_DEFAULT_TIMEOUT; //default is 5
  //session->timeout = 3;
  session->retries = SNMP_DEFAULT_RETRIES;
  session->version = SNMP_DEFAULT_VERSION;
  session->securityModel = SNMP_DEFAULT_SECMODEL;
  session->rcvMsgMaxSize = SNMP_MAX_MSG_SIZE;
  session->flags |= SNMP_FLAGS_DONT_PROBE;

}


