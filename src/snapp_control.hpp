#include "config.h"
#include "snapp_config.hpp"

#include <pthread.h>
#include <semaphore.h>

#include <string>
#include <vector>
#include <queue>

using namespace std;

/**
 * \defgroup SNAPPcontroler SNAPP controler
 */
/*@{*/


#define CONTROL_CONNECTION_STATE_CONNECTED          1   
#define CONTROL_CONNECTION_STATE_USERNAME_RECEIVED  2
#define CONTROL_CONNECTION_STATE_AUTHENTICATED      3

 
/************
Command codes come from ftp like protocol
*/

#define CONTROL_RESPONSE_CODE_COMMAND_OK                  200
#define CONTROL_RESPONSE_CODE_READY                       220
#define CONTROL_RESPONSE_CODE_LOGIN_SUCESSFUL             230
#define CONTROL_RESPONSE_CODE_NEED_PASSWORD               331 
#define CONTROL_RESPONSE_CODE_COMMAND_NOT_IMPLEMENTED     502
#define CONTROL_RESPONSE_CODE_USER_NOT_LOGGED_IN          530

#define CONTROL_COMMAND_QUIT                 0
#define CONTROL_COMMAND_USER                 1
#define CONTROL_COMMAND_PASS                 2
#define CONTROL_COMMAND_RELOAD               3
#define CONTROL_COMMAND_STATUS               4
#define CONTROL_COMMAND_HELP                 5
#define CONTROL_COMMAND_LOGREGEXP            6

class Snapp_control;

/*! \brief This class handles a single connection.

  This class handles each indvidual control connection
  after is accepted.
*/
class Snapp_control_worker{
  public:
    int fd;
    pthread_t thread;
    int connection_state;
    int failure_count;
    Snapp_control *parent;
  public:
    int thread_start();
    int handle_connection();
  private:
    int send_help();
    int send_status();
    int do_reload();
    int do_regexp(char *arg); 
};

/*! \brief main class for socket control

  The main class for control.
  This creates workers(and their threads), it should be only
  one of this per snapp process.
*/
class Snapp_control{
  public:
    static int listen_port;
    string enable_password;
    pthread_mutex_t *config_mutex;
    Snapp_config **current_config;
    pthread_t signal_thread; 
    friend class Snapp_control_worker; //the control worker class
                                       //have access to the snapp control private vars
  private:
    int bind_fd;
    vector<Snapp_control_worker> worker;
    queue<int> pending_fd;
    pthread_mutex_t queue_lock;
    sem_t pending_semaphore; //how signal workers for data 
    int initialized;
    pthread_t listen_thread;
  
  public:
    int thread_start();
    int initialize_socket();
    int initialize(); 
    

};

/*\@}*/

