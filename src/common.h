#ifndef COMMON_H
#define COMMON_H

#ifdef  LARGECOLLECTION 
  #define MONITORED_SIZE 80640
  #define MAX_OID_INDEX_TABLES 2048
#else
  #define MONITORED_SIZE 4164
  #define MAX_OID_INDEX_TABLES 512
#endif
#define CLASSES_SIZE 255
#define SNMP_ERR_STR_SIZE 128
#define CONFIG_SYS_STR_SIZE 1024
#define MAX_OIDS_PER_COLLECTION 12
#define STD_STRING_SIZE 256
#define MAX_CF_NAME_SIZE 16
#define MAX_RRA_PER_COLLECTION_CLASS 10
#define MAX_RRD_DATATYPE_SIZE 128


#endif
