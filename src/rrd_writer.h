#ifndef RRD_WRITER_H
#define RRD_WRITER_H

#include "config.h"
#include <stdint.h>

//This is the declaration of the interface.
//Pretty simple

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup RRDwriter RRD_writer
 */
/*@{*/


#define MAX_RRD_WRITE_CHAR_LEN 1024
#define MAX_DS_PER_FILE 20


///describes the configuration of the writer module
struct Writer_config{
  uint32_t version;
  uint32_t num_writers;
  uint32_t flush_seconds;
};

///contains the curent exportable status of the writer module
struct Writer_status{
  uint32_t num_writers;
  uint64_t avg_queue_len_5min;
  uint64_t avg_queue_len_1min;
  uint64_t failed_updates_count;
  uint64_t failed_files_count_1min;
  uint64_t failed_files_count_5min;
  uint32_t num_files_counted;
};

int init_writers(struct Writer_config *config);

int push_data(double time,const char *filename,const char *rrd_template,const char *values);

int get_writer_status(struct Writer_status *writer_status);


/*@}*/

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif



#endif
