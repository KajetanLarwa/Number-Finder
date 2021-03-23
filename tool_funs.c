#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "lib/common.h"
#include "lib/tool_funs.h"

int is_number(char *data)
{
    int len = strlen(data);
    for(int i=0;i<len;i++)
        if(data[i]>'9' || data[i]<'0')
            return 0;
    return 1;
}

void usage(char* pname)
{
    
	fprintf(stderr,"USAGE:%s [-r] [-m min=10] [-M max=1000] [-i interval=600] dir1 [dir2…]\n",pname);
    fprintf(stderr,"USAGE:%s [-r] 0 or 1\n",pname);
    fprintf(stderr,"USAGE:%s [-m min=10] min >= 1\n",pname);
    fprintf(stderr,"USAGE:%s [-M max=1000] max <= min\n",pname);
    fprintf(stderr,"USAGE:%s [-i interval=600] interval > 0\n",pname);
    fprintf(stderr,"USAGE:%s dir1 [dir2…] max path length - %d\n",pname,MAX_PATH);
	exit(EXIT_FAILURE);
}

void command_usage()
{
    fprintf(stdout,"COMMAND USAGE:\n status\n index\n query [n1] [n2...]\n exit\n");
}

int number_digits(int number)
{
    if(number == 0)
        return 1;
    int count = 0;
    while(number > 0)
    {
        number /= 10;
        count++;
    }
    return count;
}

char* make_rel_path(char* main_path, const char* path)
{
    int main_path_len = strlen(main_path);
    int path_len = strlen(path);
    if(path_len <= main_path_len)
    {
        char *rel_path = (char*)malloc(sizeof(char)*path_len);
        strncpy(rel_path,path,MAX_PATH);
        return rel_path;
    }
    char *rel_path = (char*)malloc(sizeof(char)*(path_len-main_path_len));    
    if(rel_path == NULL)
        ERR("malloc");
    int j=0;
    for(int i=main_path_len;i<=path_len;i++)
    {
        rel_path[j] = path[i];
        j++;
    }
    return rel_path;
}

int is_given_file(const char *path, const char *name)
{
    int len = strlen(path);
    int name_len = strlen(name);
    if(name_len > len)
        return 0;
    int j=0;
    for(int i=len-name_len;i<len;i++)
    {
        if(path[i] != name[j])
            return 0;
        j++;
    }
    return 1;
}

void set_mask(sigset_t *mask)
{
    if(sigemptyset(mask))
        ERR("sigemptyset");
	if(sigaddset(mask, SIGUSR1))
        ERR("sigaddset");
	if(sigaddset(mask, SIGUSR2))
        ERR("sigaddset");
    if(sigaddset(mask, SIGALRM))
        ERR("sigaddset");
    if(sigaddset(mask, SIGINT))
        ERR("sigaddset");
}