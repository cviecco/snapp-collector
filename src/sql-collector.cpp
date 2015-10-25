#include "config.h"

#include "sql-collector.hpp"
#include "collection_class.hpp"
#include "snapp_config.hpp"
#include "snapp_control.hpp"

#include "rrd_writer.h"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <assert.h>
#include <deque>
#include <list>

#include <signal.h>
       #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>
       #include <sys/time.h>
       #include <sys/resource.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <errno.h>
#include <pwd.h>

using namespace std;



#define GRANDPARENT_SLEEP_SECS 5


/*
  Stage 1 -> one net-snmp thread per host, per configuration


  One write queue
  X (<10) disk_threads //should be equal to the number of spindles as seen by the OS
  One thread per host

  Stage 2 -> (async) one thread for net-snmp.
    -> challenges -> creating the tables during resync
    
  

*/

extern int syslog_ready;

struct Global_vars globals;

//For signal handling
pthread_cond_t  reload_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t reload_lock = PTHREAD_MUTEX_INITIALIZER;
sigset_t signal_mask;
pthread_t signal_thread;
volatile int system_done=0;

int init_globals(){
    int rvalue;
    rvalue=sem_init(&globals.session_semaphore,0,800); //800 is a good limit
    assert(rvalue==0);
    return 0;
} 

/***************************
Signal handlers
****************************/

void parent_sig_handler(int sig)
    {
        fprintf(stderr, "snapp-collector: premature failure. Initialization aborted?\n");
        exit(EXIT_FAILURE);
    }
void parent_sig_handler_silent(int sig)
    {
        exit(EXIT_FAILURE);
    }



void *signal_handling_func(void *arg){
   int rvalue;
   int signo;
   sigset_t signal_set;

 
   print_log(LOG_INFO,"signal handler started\n");


   while(1==1){
/*
       rvalue=sigwait(&signal_mask,&signo);
       if(rvalue!=0){
           print_log(LOG_ERR,"Sigwait failed\n");
           exit(EXIT_FAILURE);
       } 
*/
       sigfillset( &signal_set );
       sigwait( &signal_set, &signo );

       switch (signo){
           case SIGHUP:
              print_log(LOG_NOTICE,"Got a SIGHUP\n");
              rvalue=pthread_mutex_lock(&reload_lock);
              assert(0==rvalue);
             // pthread_cond_signal(&reload_cond);

              rvalue=pthread_mutex_unlock(&reload_lock);
              assert(0==rvalue);
              pthread_cond_signal(&reload_cond);
              break;
           case SIGINT:
              print_log(LOG_NOTICE,"Got a sigint\n");
              exit(EXIT_FAILURE);
              break;
           case SIGTERM:
              print_log(LOG_NOTICE,"Got a SIGTERM\n");
              if(0==system_done){
                system_done=1;
                pthread_cond_signal(&reload_cond);
              }
              else{
                exit(EXIT_FAILURE);
              }
              break;
           case SIGUSR1:
              set_log_level(get_log_level()+1);
              print_log(LOG_NOTICE,"Got a SIGUSR1\n");
              break;
           case SIGUSR2:
              set_log_level(LOG_NOTICE);
              print_log(LOG_NOTICE,"Got a SIGUSR2\n");
              break;

           default:
              print_log(LOG_WARNING,"Unexpected signal caught. Singal=%d\n", signo);
       }
   }
   return (void*)0; //be clean
}



/************************** 

****************************/
/**
   Daemonization is done in two stages. This stage happens before the config
   is loaded and is in charge of forking and initializing syslog.
*/
int do_deamonize(){
    pid_t pid;
    //char *log_filename="log.txt";
    char *log_filename=NULL;
    int log_fd; 


   //
   if (signal(SIGCHLD, parent_sig_handler_silent) == SIG_ERR) {
         perror("signal");
         exit(EXIT_FAILURE);
   }


    if((pid = fork()) < 0){
          print_log(LOG_ERR,"Can not fork!!");
          exit(EXIT_FAILURE);
     }
     else{
          if(pid != 0){
              //this is the parent
              if (signal(SIGCHLD, parent_sig_handler_silent) == SIG_ERR) {
                perror("signal");
                exit(EXIT_FAILURE);
              }
              sleep(GRANDPARENT_SLEEP_SECS);
              exit(0);
          }
     }


     if(setsid()<0){
          print_log(LOG_ERR,"Cannot become session leader\n");
          exit(EXIT_FAILURE);
     }

     //second fork to prevent terminal errors
     if ((pid = fork()) < 0){
        print_log(LOG_ERR,"Can not fork!!");
        exit(1);
     }
       else{
         if(pid != 0){
             //this is the parent
/*
             if (signal(SIGCHLD, parent_sig_handler) == SIG_ERR) {
                perror("signal");
                exit(EXIT_FAILURE);
             }
*/
             //wait a few seconds, more than the last.
             sleep( GRANDPARENT_SLEEP_SECS+2);
             exit(EXIT_SUCCESS);
         }
     }

      if(NULL!=log_filename){
         log_fd=open(log_filename, O_WRONLY|O_CREAT |O_TRUNC, (S_IRUSR | S_IWUSR | S_IRGRP|S_IROTH ) );
         if(0>log_fd){perror("Cannot open deamon log file");exit(EXIT_FAILURE);}
         //redirect stderr and stout and redirect to log file
         dup2(log_fd,2);
         dup2(log_fd,1);
         dup2(log_fd,0); //aslo redirect STDIN?
         close(log_fd);
         //close(0);
      }
      else{
          //close all and send to syslog
          int fd0,fd1,fd2;
          unsigned int i;
          struct rlimit rl;
          //close all file descriptors, 
          if(getrlimit(RLIMIT_NOFILE,&rl)<0){
             print_log(LOG_ERR,"Cannot get file limit\n");
             exit(EXIT_FAILURE);
          }
          if(rl.rlim_max == RLIM_INFINITY){
            rl.rlim_max = 1024;
          }
          for(i = 0; i< rl.rlim_max; i++){
            close(i);
          }

          //reopen the base fd to 'known' locations   
          fd0 = open("/dev/null", O_RDWR);
          fd1= dup(0);
          fd2 = dup(0);

          //open syslog with generic SNAPP name
          //strcpy(collector_name,"SNAPP");
          openlog("SNAPP-SQL",LOG_PID,LOG_LOCAL0);
          syslog_ready=1;
      }
   return 0;
}

/**
  This makes the pid file, should only run after all other initializations have occured!
*/

int make_pid_file(const char *pid_dir){
      char pid_filename[512];
      FILE *fp1;
      snprintf(pid_filename,512-1,"%s/PID",pid_dir); //need to fix this later
      print_log(LOG_DEBUG,"PID file = %s\n",pid_filename);
      fp1 = fopen(pid_filename,"w");
      if(NULL==fp1){
         print_log(LOG_ERR,"Failed to create pid file, aborting\n");
         exit(EXIT_FAILURE);
      }
      fprintf(fp1,"%u", getpid());
      fclose(fp1);
      print_log(LOG_INFO,"the PID is %i\n", getpid());
      
      return 0;
}

/**
   This is the second stage of demonization (after config file is read)
   it sets up the pid file AND reopens syslog with its collection name
*/
int deamonize2(const char *in_collector_name,const char *pid_dir){
      static char collector_name[STD_STRING_SIZE];
      

      if(1==0){ 
        char pid_filename[512];
        FILE *fp1;
        snprintf(pid_filename,512-1,"%s/PID",pid_dir); //need to fix this later
        print_log(LOG_DEBUG,"PID file = %s\n",pid_filename);
        
        fp1 = fopen(pid_filename,"w");
        if(NULL==fp1){
           print_log(LOG_ERR,"Failed to create pid file, aborting\n");
           exit(EXIT_FAILURE);
        }
        fprintf(fp1,"%u", getpid());
        fclose(fp1); 
        print_log(LOG_INFO,"the PID is %i\n", getpid());

       
      }
      //now reset the syslog name logging to the 'correct' name 
      memset(collector_name,0x00,STD_STRING_SIZE);
      strncpy(collector_name,in_collector_name,STD_STRING_SIZE-1);
      //peculiar -> openlog & closelog are void funcs.
      closelog();
      openlog(collector_name,LOG_PID,LOG_LOCAL0);

      return 0;
}



int start_signaling_handler(){
    int rvalue;
/*
    sigset_t oldmask;


    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGHUP);  
    sigaddset(&signal_mask, SIGUSR1); 
    sigaddset(&signal_mask, SIGUSR2);   
    sigaddset(&signal_mask, SIGINT);

    rvalue=pthread_sigmask(SIG_BLOCK,&signal_mask,&oldmask);
    assert(0==rvalue);
*/
        sigset_t signal_set;
//        pthread_t sig_thread;

        /* block all signals */
        sigfillset( &signal_set );
        pthread_sigmask( SIG_BLOCK, &signal_set,
                NULL );



    rvalue=pthread_create(&signal_thread ,NULL,signal_handling_func ,0);    
    assert(0==rvalue);
   
    return 0;
}

int print_version(){
   FILE *out_stream=stdout;
   fprintf(out_stream,"snapp-collector version %s (svn %s)\n", VERSION,SVN_VERSION);
   return 0;

}

int print_help(){
   FILE *out_stream=stdout;
   fprintf(out_stream,"snapp-collector [-h|--help] [--foreground|-f] [-c CONFIG|--config=CONFIG]\n\n");
   return 0; 
}

int main(int argc, char **argv){
   
   Snapp_config *current_config,*new_config;
   int rvalue;
   int deamonize=1;
   int c;
   int digit_optind = 0;
   char default_config_name[] ="snapp_config.xml";
   char *config_name;
   Snapp_control controller;
   //Control_config control_config;
   pthread_mutex_t config_mutex=PTHREAD_MUTEX_INITIALIZER;
   char default_run_username[] ="apache";
   char *run_username;
   struct passwd *pw;

 
   ////init defaults
   config_name=default_config_name;
   run_username=default_run_username;
   //setlogmask(LOG_UPTO(LOG_NOTICE));
   set_log_level(LOG_NOTICE);

////////////////////
// Process options.
//////
   while (1) {
       int this_option_optind = optind ? optind : 1;
       int option_index = 0;
       static struct option long_options[] = {
                   {"help", 0, 0, 'h'},
                   {"verbose", 0, 0, 0},
                   {"debug", 0, 0, 0},
                   {"config", 1, 0, 'c'},
                   {"user",1,0, 'u'},
                   {"foreground", 0, 0, 'f'},
                   {0, 0, 0, 0}
       };

       c = getopt_long(argc, argv, "fhc:u:012",
                        long_options, &option_index);
       if (c == -1)
           break;

       switch (c) {
               case 0:
                   print_log(LOG_DEBUG,"option %s", long_options[option_index].name);
                   if (optarg)
                       print_log(LOG_DEBUG," with arg %s", optarg);
                   printf("\n");
                   if(0==strcmp(long_options[option_index].name,"verbose")){
                      //setlogmask(LOG_UPTO(LOG_INFO));
                      set_log_level(LOG_INFO);
                   }
                   if(0==strcmp(long_options[option_index].name,"debug")){
                      //setlogmask(LOG_UPTO(LOG_INFO));
                      set_log_level(LOG_DEBUG);
                   }
                   break;

               case '0':
               case '1':
               case '2':
                   if (digit_optind != 0 && digit_optind != this_option_optind)
                     print_log(LOG_DEBUG,"digits occur in two different argv-elements.\n");
                   digit_optind = this_option_optind;
                   print_log(LOG_DEBUG,"option %c\n", c);
                   break;

               case 'f':
                   print_log(LOG_DEBUG,"option f\n");
                   deamonize=0;
                   break;

               case 'c':
                   print_log(LOG_DEBUG,"option c with value '%s'\n", optarg);
                   config_name=optarg;
                   break;
               case 'u':
                   print_log(LOG_DEBUG,"option u with value '%s'\n", optarg);
                   run_username=optarg;
                   break;


               case 'h':
                   //printf("option h\n");
                   print_version();
                   print_help();
                   exit(EXIT_SUCCESS);
                   break;

               default:
                   printf("?? getopt returned character code 0%o ??\n", c);
       }
   }

/*
           if (optind < argc) {
               printf("non-option ARGV-elements: ");
               while (optind < argc)
                   printf("%s ", argv[optind++]);
               printf("\n");
           }

           exit(EXIT_SUCCESS);
*/
////////////////

   if(deamonize==1){
     do_deamonize();
   }
   print_log(LOG_NOTICE,"SNAPP-collector (version %s (svn %s)) started\n", VERSION,SVN_VERSION );


   //netsnmp initialization!
   start_signaling_handler();
   SOCK_STARTUP;
   init_snmp("SNAPP-Collector");
   init_globals();
   init_writers(NULL);
   init_snmp_logging ();

   

   //controller.initialize();
   //start_signaling_handler();

   //config.resize(1);


   //current_config=&config[0];
   current_config=new Snapp_config;
   assert(NULL!=current_config);
   current_config->globals=&globals; //dont like this.  
 
   rvalue=current_config->load_config(config_name);
   if(rvalue<0){
      print_log(LOG_ERR,"Failed to load the database config! \n");
      return -1;
   }
   print_log(LOG_NOTICE,"config loaded \n");   
 
   //daemonize part 2 -> rename the log,
   //                     create the pid file with the now known location.
   //                    
   if(deamonize==1){
     //do_deamonize();
     //int deamonize2(char *collector_name,char *pid_dir)
     //deamonize2(NULL,current_config->get_rrd_dir().c_str());
     deamonize2(current_config->get_collector_name().c_str(),current_config->get_rrd_dir().c_str());
   }

 
   //initialize the controller
   controller.listen_port =current_config->get_control_port();   
   controller.enable_password=current_config->get_control_enable_password();
   controller.signal_thread=signal_thread;
   controller.current_config=&current_config;
   controller.config_mutex=&config_mutex;
   controller.initialize_socket();


   //pid file is made before any data is processed or any other threads have started
   if(deamonize==1){
       //the core has been initialized now, it is time to write the PID file!
       make_pid_file(current_config->get_rrd_dir().c_str());
   }



//******************
//Drop privileges here!, before ANY data is processed AND any server threads have started
  //if(default_run_username!=run_username || 0==geteuid()){
  if(default_run_username!=run_username ){
     if (geteuid()) {
        print_log(LOG_ERR, "only root can use -u.\n");
        exit(EXIT_FAILURE);
     }
     pw=getpwnam(run_username);
     if(NULL==pw){
        fprintf(stderr, "User %s not found.aborting\n",run_username);
        exit(EXIT_FAILURE);
     }
     if(0!=setgid(pw->pw_gid) || 0!=setuid(pw->pw_uid)){
        perror("Could not change uid. aborting\n");
        exit(EXIT_FAILURE);
     }
  }



///////

   controller.initialize();


   rvalue=current_config->init_config();
   if(rvalue!=0){
     print_log(LOG_ERR,"Snapp initialization failure\n");
     exit(EXIT_FAILURE);
   }

   print_log(LOG_NOTICE,"Snapp initialization complete\n");   


   while(1==1){
      struct timeval now;
      struct timespec timeout;
      int retcode;

      pthread_mutex_lock(&reload_lock);
      gettimeofday(&now,NULL);
      timeout.tv_sec = now.tv_sec + 7200; //delay up to 7200 secs.
      timeout.tv_nsec = now.tv_usec * 1000;
      retcode = 0;
              //while (x <= y && retcode != ETIMEDOUT) {
      retcode = pthread_cond_timedwait(&reload_cond, &reload_lock, &timeout);
              //}
      if(retcode!= ETIMEDOUT && retcode!=0){
         print_log(LOG_ERR,"Failure on cond timedwait\n");
         exit(EXIT_FAILURE);
      }
      if (retcode == ETIMEDOUT) {
           /* timeout occurred */
         print_log(LOG_INFO,"Normal wait timedwait\n");
          
      } else {
           /* operate on x and y */
         if(0!=system_done){
            print_log(LOG_NOTICE,"Terminating !\n");
            goto done;
         }
         print_log(LOG_INFO,"Reloading config (SIGNAL) !\n");
      }
      print_log(LOG_NOTICE,"Starting config Reload!\n");
      pthread_mutex_unlock(&reload_lock);


      
      //now we create the new config!;
      new_config=new Snapp_config;
      assert(new_config!=NULL);
      print_log(LOG_DEBUG,"after new config on config loop\n");
      new_config->globals=&globals;
      rvalue=new_config->load_config(config_name);
      if(rvalue<0){
         delete new_config;
         print_log(LOG_ERR,"Failed to load new config\n");
         continue; 
      }
      print_log(LOG_INFO,"after load config on config loop\n");
      rvalue=new_config->init_config();
      if(rvalue<0){
         delete new_config;
         print_log(LOG_ERR,"Failed to init new config\n");
         continue; 
      }
      print_log(LOG_INFO,"New config initialized.\n");
      //assert(2==config.size());


      print_log(LOG_DEBUG,"removing old config\n");
      rvalue=pthread_mutex_lock(&config_mutex);
      assert(0==rvalue);
      //config.pop_front();
      //current_config->collection_class.clear();
     
      delete current_config;
      print_log(LOG_DEBUG,"Old config removed\n");
      current_config=new_config;
      rvalue=pthread_mutex_unlock(&config_mutex);
      assert(0==rvalue);

      print_log(LOG_NOTICE,"Config reload complete\n");
   }
   //sleep(120);

done:
   delete current_config;

/*
   //lprint_log(LOG_DEBUG,"removing old config\n");oad next
   old_config=current_config;
   current_config=&config[1];
   current_config->globals=&globals; //dont like this.  

   rvalue=current_config->load_config("snapp_config.xml");
   if(rvalue<0){
      return -1;
   }
   rvalue=current_config->init_config();
   //old_config->finalize(); 

   sleep(120);
*/
   print_log(LOG_NOTICE,"Clean termination of SNAPP\n");   
   return 0;
}
