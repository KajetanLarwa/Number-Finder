#define _GNU_SOURCE
#include <fcntl.h>
#include <ftw.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "lib/structs.h"
#include "lib/common.h"
#include "lib/tool_funs.h"
#include "lib/io.h"
#include "lib/find.h"

int find_numbers_recur(const char *path, const struct stat *s, int type, struct FTW *f)
{
    if(type == FTW_F)
        find_numbers(path, srchArgs->min, srchArgs->max, 0);
    return 0;
}

void find_numbers(const char *path, int min, int max, int is_path_relative_to_idx)
{
    //skip if file is .numf_index or .numf_pid
    if(is_given_file(path,INDEX_FILE_NAME) || is_given_file(path,PID_FILE_NAME))
        return;
    
    int fd;
    if((fd = TEMP_FAILURE_RETRY(open(path,O_RDONLY))) < 0)
        ERR("open");

    //making path relative to dir with .numf_index
    char *rel_path;
    if(is_path_relative_to_idx == 0)
        rel_path = make_rel_path(srchArgs->dir_with_index_file_path, path);
    else
    {
        //path is already relative
        rel_path = (char*)malloc(sizeof(char) * (strlen(path)+1));
        if(rel_path == NULL)
            ERR("malloc");
        strcpy(rel_path,path); //rel_path has the same size
    }
        
    int max_number_digits = number_digits(max);
    char *number = (char*)malloc(sizeof(char)*(max_number_digits+1));
    if(number == NULL)
        ERR("malloc");
    int number_int;
    char c;
    ssize_t bytes_read;
    int current_number_of_digits = 0;
    int current_offset = 0;
    int offset;
    int skip = 0;
    char buff[SIZE_READ_PORTION];
    do
    {
        bytes_read = bulk_read(fd,buff,sizeof(char) * SIZE_READ_PORTION);
        
        if(bytes_read == 0) //if end of file
        {
            if(current_number_of_digits > 0) //if was a number
            {
                if(number_int <= max && number_int >= min) 
                {
                    //if number in range, add to list
                    insert_record_to_index(&(srchArgs->head),rel_path, strlen(rel_path), offset,number_int);
                }
            }
            break;
        }
        for(int i=0; i<(int)(bytes_read/sizeof(char));i++)
        {
            c = buff[i];
            current_offset++;
            if(skip == 1)
            {
                if(c <= '9' && c >= '0')
                    continue;
                else
                {
                    current_number_of_digits = 0;
                    skip = 0;
                    free(number);
                    number = (char*)malloc(sizeof(char)*(max_number_digits+1));
                }
            }
            if(c <= '9' && c >= '0')
            {
                if(current_number_of_digits == 0)
                {
                    if(c == '0') //number can't start with '0'
                        skip = 1;
                    else
                        offset = current_offset; //first digit offset is number offset
                }
                if(current_number_of_digits < max_number_digits && skip == 0)
                {
                    number[current_number_of_digits] = c;
                    current_number_of_digits++;
                    number_int = atoi(number);
                    if(number_int > max)
                        skip = 1;
                }
            }
            else
            {
                if(current_number_of_digits > 0) //if was a number
                {
                    if(number_int <= max && number_int >= min) 
                    {
                        //if number in range, add to list
                        insert_record_to_index(&(srchArgs->head),rel_path, strlen(rel_path), offset,number_int);
                    }         
                    current_number_of_digits=0;
                    free(number);
                    number = (char*)malloc(sizeof(char)*(max_number_digits+1));
                    if(number == NULL)
                        ERR("malloc");
                }
            }
        }
    }while(bytes_read != 0); //if not end of file

    if(TEMP_FAILURE_RETRY(close(fd)))
        ERR("close");
    free(number);
    free(rel_path);
}