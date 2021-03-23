#ifndef common_h
#define common_h

#include <stdio.h>
#include <stdlib.h>

#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d %s\n",__FILE__,__LINE__,source),\
		     exit(EXIT_FAILURE))

#define MAXFD 20
#define MAX_PID_LENGTH 20
#define MAX_COMMAND_LENGTH 150
#define MAX_NUMBER_LENGTH 9
#define MAX_NUMBERS 200
#define DEFAULT_R 0
#define DEFAULT_MIN 10
#define DEFAULT_MAX 1000
#define DEFAULT_INTERVAL 600
#define MAX_PATH 200
#define PID_FILE_NAME ".numf_pid"
#define INDEX_FILE_NAME ".numf_index"
#define TEMP_FILE_NAME ".numf_temp"
#define SIZE_READ_PORTION 100
#define MAX_THREADS 200

#define STATUS "status"
#define INDEX "index"
#define QUERY "query "
#define EXIT "exit"

#endif