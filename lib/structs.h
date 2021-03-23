#ifndef structs_h
#define structs_h

#include "list.h"
#include "common.h"

typedef struct queryArgs
{
    dirNode_t *head;
    char in[MAX_COMMAND_LENGTH+2];
    int *fd;
}queryArgs_t;

typedef struct searchArgs
{
    int min;
    int max;
    char *dir_with_index_file_path;
    char *numf_index_path;
    indexRecordNode_t *head;
}searchArgs_t;

typedef struct signalFlag
{
    int is_sent;
    int is_working;
    pthread_mutex_t is_sent_mutex;
    pthread_mutex_t is_working_mutex;
    
}signalFlag_t;

typedef struct workingThreadArgs
{
    char *dir;
    char *path_index_file;
    char* temp_file_path;
    int fd;
    int r;
    int min;
    int max;
    pthread_t main_tid;
    signalFlag_t *flag;
}workingThreadArgs_t;

searchArgs_t *srchArgs; //necessary because of nftw function

#endif