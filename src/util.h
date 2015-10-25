
#ifndef UTIL_H
#define UTIL_H

#include <syslog.h>
#include <net-snmp/net-snmp-config.h> 
#include <net-snmp/net-snmp-includes.h>


//Now the extrenal functions


#ifdef __cplusplus
extern "C" {
#endif

/**
* \note this is a note
* Will this survive into the file documentation?
*/

void print_log(int priority, const char *string, ... );
int  file_datasync(const char *filename);
int  file_drop_from_caches(const char *filename);

int  get_log_level();
int  set_log_level(int new_level);

///wrappers to make some funtions thread safe
void simple_snmp_sess_init(netsnmp_session *session);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
