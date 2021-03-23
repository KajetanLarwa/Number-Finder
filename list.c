#define _GNU_SOURCE
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "lib/list.h"
#include "lib/common.h"
#include "lib/io.h"

void insert_record_to_index(indexRecordNode_t **head, char *path, int path_len, int offset, int number)
{
    indexRecordNode_t *new_node = (indexRecordNode_t*)malloc(sizeof(indexRecordNode_t));
    if(new_node == NULL)
        ERR("malloc");
    new_node->path_len = path_len+1;
    new_node->number = number;
    new_node->offset = offset;
    strncpy(new_node->path,path,path_len+1);
    new_node->next = *head;
    *head = new_node;
}

void save_list_to_file(indexRecordNode_t *head, char *path, char *temp_file_path, int *fd)
{
    //file format: length of path, path, number, offset (without spaces and separators)
    ssize_t bytes_written;
    
    if((*fd = TEMP_FAILURE_RETRY(open(temp_file_path, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777))) < 0)
            ERR("open");
    while(head != NULL)
    {
        //----- write to stdout----------------
        printf("PATH LENGTH + 1 [%d], PATH [%s], OFFSET [ %d ], NUMBER [%d]\n",head->path_len, head->path, head->offset, head->number);
        
        //writing length of path
        bytes_written = bulk_write(*fd, &(head->path_len), sizeof(int));
        if(bytes_written != sizeof(int))
            ERR("write");
        //writing path
        bytes_written = bulk_write(*fd, head->path, sizeof(char)*(head->path_len));
        if(bytes_written != (sizeof(char)*(head->path_len)))
            ERR("write");
        //writing number
        bytes_written = bulk_write(*fd, &(head->number), sizeof(int));
        if(bytes_written != sizeof(int))
            ERR("write");
        //writing offset
        bytes_written = bulk_write(*fd, &(head->offset), sizeof(int));
        if(bytes_written != sizeof(int))
            ERR("write");
        head = head->next;
    }
    if(TEMP_FAILURE_RETRY(close(*fd)))
        ERR("close");
    if(rename(temp_file_path, path) < 0)
        ERR("rename");
}

void remove_index_list(indexRecordNode_t **head)
{
    indexRecordNode_t *node;
    while(*head)
    {
        node = (*head)->next;
        free(*head);
        *head = node;
    }
}

//reading from .numf_index
// void read_list(char *path)
// {
//     //file format: length of path, path, number, offset (without spaces and separators)
//     int fd;
//     int len;
//     int offset;
//     int number;
//     char *p;
//     ssize_t bytes_written;
//     if((fd = TEMP_FAILURE_RETRY(open(path, O_RDONLY))) < 0)
//             ERR("open");
//     while(bulk_read(fd,&len,sizeof(int)) != 0)
//     {
//         p = (char*)malloc(sizeof(char)*len);
//         if(p == NULL)
//             ERR("malloc");
//         //read path
//         bytes_written = bulk_read(fd, p, sizeof(char)*len);
//         if(bytes_written != sizeof(char)*len)
//             ERR("read");
//         // //read number
//         bytes_written = bulk_read(fd, &number, sizeof(int));
//         if(bytes_written != sizeof(int))
//             ERR("read");
//         // //read offset
//         bytes_written = bulk_read(fd, &offset, sizeof(int));
//         if(bytes_written != sizeof(int))
//             ERR("read");
//         printf("%d%s%d%d\n",len, p, offset, number);
//         free(p);
//     }
//     if(TEMP_FAILURE_RETRY(close(fd)))
//         ERR("close");       
// }

void insert_dir(dirNode_t **head, char *path, char *index_path)
{
    dirNode_t *new_node = (dirNode_t*)malloc(sizeof(dirNode_t));
    if(new_node == NULL)
        ERR("malloc");
    if(strlen(path) > MAX_PATH)
        ERR("insert_dir");
    strcpy(new_node->path,path);
    if(strlen(index_path) > MAX_PATH)
        ERR("insert_dir");
    strcpy(new_node->index_path,index_path);
    new_node->next = *head;
    *head = new_node;
}

void remove_dir_list(dirNode_t **head)
{
    dirNode_t *node;
    while(*head)
    {
        node = (*head)->next;
        free(*head);
        *head = node;
    }
}

void search_index(dirNode_t *head, int *numbers, int n, int *fd)
{
    int len;
    int offset;
    int number;
    char *p;
    ssize_t bytes_written;
    dirNode_t *temp = head;
    for(int i=0;i<n;i++)
    {
        temp = head;
        printf("Number %d occurrences:\n",numbers[i]);
        while(temp != NULL)
        {
            if(access(temp->index_path, F_OK) != 0)
            {
                //there is no .numf_index in that directory
                printf("There is no .numf_index file in %s yet.\n",temp->path);
                temp = temp->next; 
                continue;
            }
            if((*fd = TEMP_FAILURE_RETRY(open(temp->index_path, O_RDONLY))) < 0)
                    ERR("open");
            while(bulk_read(*fd,&len,sizeof(int)) != 0)
            {
                p = (char*)malloc(sizeof(char)*len);
                //read path
                bytes_written = bulk_read(*fd, p, sizeof(char)*len);
                if(bytes_written != sizeof(char)*len)
                    ERR("read");
                // //read number
                bytes_written = bulk_read(*fd, &number, sizeof(int));
                if(bytes_written != sizeof(int))
                    ERR("read");
                // //read offset
                bytes_written = bulk_read(*fd, &offset, sizeof(int));
                if(bytes_written != sizeof(int))
                    ERR("read");
                if(number == numbers[i])
                {
                    printf("%s%s:%d\n",temp->path,p,offset);
                }
                free(p);
            }
            if(TEMP_FAILURE_RETRY(close(*fd)))
                ERR("close");     
            temp = temp->next;    
        }
    }
}