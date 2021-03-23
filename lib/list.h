#ifndef list_h
#define list_h

#include "common.h"

typedef struct indexRecordNode 
{
    char path[MAX_PATH + 1];
    int offset;
    int number;
    int path_len;
    struct indexRecordNode *next;
} indexRecordNode_t;

typedef struct dirNode 
{
    char path[MAX_PATH + 1];
    char index_path[MAX_PATH + 1];
    struct dirNode *next;
} dirNode_t;

void insert_record_to_index(indexRecordNode_t **head, char *path, int path_len, int offset, int number);
void save_list_to_file(indexRecordNode_t *head, char *path, char *temp_file_path, int *fd);
void remove_index_list(indexRecordNode_t **head);
//void read_list(char *path);

void insert_dir(dirNode_t **head, char *path, char *index_path);
void search_index(dirNode_t *head, int *numbers, int n, int *fd);
void remove_dir_list(dirNode_t **head);

#endif