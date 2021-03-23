#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>
#include <ftw.h>

#include "lib/structs.h"
#include "lib/common.h"
#include "lib/list.h"
#include "lib/io.h"
#include "lib/tool_funs.h"
#include "lib/index.h"
#include "lib/find.h"

void read_arguments(int argc, char **argv, int *r, int *min, int *max, int *interval, int *count_dirs);
void* main_command_thread(void *voidArgs);
void* query_thread(void *voidArgs);
void create_indexing_process(char *dir, dirNode_t *head, int r, int min, int max, int interval);

int main(int argc, char **argv)
{
    int r, min, max, interval, count_dirs;
    dirNode_t *head = NULL;
    char path[MAX_PATH];

    read_arguments(argc, argv, &r, &min, &max, &interval, &count_dirs);

    //load dirs
    int len;
    for(int i=0;i<count_dirs;i++)
    {
        len = strlen(argv[optind+i]);
        if(len > MAX_PATH)
            usage(argv[0]);
        strcpy(path,argv[optind+i]);
        if(path[strlen(path)-1] != '/')
            strcat(path,"/");
        char *index_path = (char*)malloc(sizeof(char)*(strlen(path)+strlen(INDEX_FILE_NAME) + 1));
        if(index_path == NULL)
            ERR("malloc");
        strcpy(index_path,path);
        index_path = strcat(index_path,INDEX_FILE_NAME);
        
        insert_dir(&head, path, index_path);
        create_indexing_process(path, head, r, min, max, interval);
    }

    //main process loop
    pthread_t tid;
    if(pthread_create(&tid,NULL,main_command_thread,head) != 0)
        ERR("pthread_create");

    sigset_t mask;
    set_mask(&mask);
    if(pthread_sigmask(SIG_BLOCK, &mask, NULL))
        ERR("pthread_sigmask");

    //waiting for signal
    int signal;
    int quit = 0;
    while(!quit)
    {
        if(sigwait(&mask,&signal))
            ERR("sigwait");
        switch(signal)
        {
            case SIGUSR1:
                break;
            case SIGUSR2:
                break;
            case SIGALRM:
                break;
            case SIGINT:
                if(kill(0,SIGINT))
                    ERR("kill");
                quit = 1;
                break;
            default:
                fprintf(stderr, "Unexpected signal\n");
        }
    }   
    pthread_cancel(tid); 
    if(pthread_join(tid, NULL))
        ERR("pthread_join");
    while(wait(NULL)>0);
    remove_dir_list(&head);
    
    if(pthread_sigmask(SIG_UNBLOCK, &mask, NULL))
        ERR("pthread_sigmask");
    
    return EXIT_SUCCESS;
}

void read_arguments(int argc, char **argv, int *r, int *min, int *max, int *interval, int *count_dirs)
{
    *r = DEFAULT_R;
    *min = DEFAULT_MIN;
    *max = DEFAULT_MAX;
    *interval = DEFAULT_INTERVAL;
    
    int c;
    while((c = getopt(argc,argv,"rm:M:i:")) != -1)
    {
        switch(c)
        {
            case 'r':
                *r = 1;
                break;
            case 'm':
                if(is_number(optarg) == 0)
                    usage(argv[0]);
                *min = atoi(optarg);
                break;
            case 'M':
                if(is_number(optarg) == 0)
                    usage(argv[0]);
                *max = atoi(optarg);
                break;
            case 'i':
                if(is_number(optarg) == 0)
                    usage(argv[0]);
                *interval = atoi(optarg);
                break;
            default: 
                usage(argv[0]);
        }
    }
    *count_dirs = argc - optind;
    if(*interval <= 0)
        usage(argv[0]);
    if(*min < 1)
        usage(argv[0]);
    if(*min > *max)
        usage(argv[0]);    
    if(*count_dirs <= 0)
        usage(argv[0]);
}

void* main_command_thread(void *voidArgs)
{
    dirNode_t *head = voidArgs;
    int fd;
    pthread_t tid;
    char *eol;
    char in[MAX_COMMAND_LENGTH+2];
    int quit = 0;
    while(!quit)
    {
        
        fgets(in,MAX_COMMAND_LENGTH+2,stdin);
        eol = strchr(in,'\n');
        *eol = '\0';
        if(strlen(in) == strlen(STATUS) && strcmp(in, STATUS) == 0)
        {
            if(kill(0,SIGUSR1))
                ERR("kill");
        }
        else if(strlen(in) == strlen(INDEX) && strcmp(in, INDEX) == 0)
        {
            if(kill(0,SIGUSR2))
                ERR("kill");
        }
        else if(strlen(in) > strlen(QUERY))
        {
            if(strncmp(in, QUERY, strlen(QUERY)) == 0)
            {
                
                queryArgs_t args;
                args.head = head;
                args.fd = &fd;
                strcpy(args.in, in);
                
                pthread_attr_t threadAttr;
                if(pthread_attr_init(&threadAttr))
                    ERR("pthread_attr_init");
                if(pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED))
                    ERR("pthread_attr_setdetachstate");
                if(pthread_create(&tid,&threadAttr,query_thread,&args) != 0)
                    ERR("pthread_create");
                pthread_attr_destroy(&threadAttr);   
            }
            else
                command_usage();
        }
        else if((strlen(in) == strlen(EXIT) && strcmp(in, EXIT) == 0))
        {
            if(kill(0,SIGINT))
                ERR("kill");
            quit = 1;
        }
        else
            command_usage();
    }
    return NULL;
}

void* query_thread(void *voidArgs)
{
    queryArgs_t *args = voidArgs;
    int len = strlen(args->in);
    char c;
    char *number = (char*)malloc(sizeof(char) * MAX_NUMBER_LENGTH);
    int number_int[MAX_NUMBERS];
    int current_num=0;;
    int current_number_of_digits = 0;

    for(int i=strlen(QUERY);i<len;i++)
    {
        c = args->in[i];
        if(c == '0' && current_number_of_digits == 0)
        {
            while(args->in[++i] != ' ');
        }
        if((c <= '9' && c >= '0'))
        {
            if(current_number_of_digits < MAX_NUMBER_LENGTH)
            {
                number[current_number_of_digits] = c;
                current_number_of_digits++;
            }
            else
            {
                fprintf(stdout,"COMMAND USAGE: Number in query is too big!\n");
                return NULL;
            }
        }
        else if(c == ' ')
        {
            if(current_number_of_digits > 0)
            {
                number_int[current_num] = atoi(number);
                current_number_of_digits = 0;
                current_num++;
                free(number);
                number = (char*)malloc(sizeof(char) * MAX_NUMBER_LENGTH);
                continue;
            }
            continue;
        }
        else
        {
            command_usage();
            return NULL;         
        }
        if(i == len-1 && current_number_of_digits > 0)
        {
                number_int[current_num] = atoi(number);
                
                current_number_of_digits = 0;
                current_num++;
                free(number);
                number = (char*)malloc(sizeof(char) * MAX_NUMBER_LENGTH);
        }
    }
    search_index(args->head, number_int, current_num, args->fd);
    return NULL;
}

void create_indexing_process(char *dir, dirNode_t *head, int r, int min, int max, int interval)
{
    pid_t pid;
    if((pid = fork())<0)
        ERR("fork");
    if(!pid)
    {
        indexing_process_work(dir, r, min, max, interval);
        exit(EXIT_SUCCESS);
    }
}