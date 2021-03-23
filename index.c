#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <ftw.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib/structs.h"
#include "lib/index.h"
#include "lib/common.h"
#include "lib/tool_funs.h"
#include "lib/io.h"
#include "lib/find.h"


void indexing_process_work(char *dir, int r, int min, int max, int interval)
{ 
    sigset_t mask;
    set_mask(&mask);
    if(pthread_sigmask(SIG_BLOCK, &mask, NULL))
        ERR("pthread_sigmask");

    //making .numf_pid file path 
    if(strlen(dir)+strlen(PID_FILE_NAME) > MAX_PATH)
        ERR("path length");
    char *path_pid_file = (char*)malloc(sizeof(char)*(strlen(dir)+strlen(PID_FILE_NAME)+1));
    if(path_pid_file == NULL)
        ERR("malloc");
    strcpy(path_pid_file,dir);
    path_pid_file = strcat(path_pid_file,PID_FILE_NAME);

    //.numf_pid
    int fd;
    pid_t pid;
    ssize_t bytes_read_written;    
    fd = TEMP_FAILURE_RETRY(open(path_pid_file, O_TRUNC | O_CREAT | O_EXCL | O_WRONLY, 0777));
    if(fd < 0 && errno == EEXIST) //if .numf_pid file exists
    {
        if((fd = TEMP_FAILURE_RETRY(open(path_pid_file, O_RDONLY))) < 0)
            ERR("open");
        bytes_read_written = bulk_read(fd,&pid,sizeof(pid_t));
        if(bytes_read_written != sizeof(pid_t))
            ERR("read");
        if(TEMP_FAILURE_RETRY(close(fd)))
            ERR("close");
        free(path_pid_file);
        fprintf(stderr,"%d\n",pid);
        exit(EXIT_FAILURE);
    }
    else if(fd < 0)
        ERR("open");
    pid = getpid();
    bytes_read_written = bulk_write(fd, &pid, sizeof(pid_t));
    if(bytes_read_written != sizeof(pid_t))
        ERR("write");
    if(TEMP_FAILURE_RETRY(close(fd)))
        ERR("close");

    //making .numf_index file path
    if(strlen(dir)+strlen(INDEX_FILE_NAME) > MAX_PATH)
        ERR("path length");
    char *path_index_file = (char*)malloc(sizeof(char)*(strlen(dir)+strlen(INDEX_FILE_NAME)));
    if(path_index_file == NULL)
        ERR("malloc");
    strcpy(path_index_file,dir);
    path_index_file = strcat(path_index_file,INDEX_FILE_NAME);

    int is_active = 0;
    clock_t start = 0;
    clock_t end = 0;
    signalFlag_t flag;
    flag.is_sent_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    flag.is_sent = 0;
    flag.is_working_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    flag.is_working = 0;
    pthread_t tid = pthread_self();
    
    //start indexing procedure if .numf_index doesn't exist
    if(access(path_index_file, F_OK) != 0)
    {
        is_active = 1;
        indexing_procedure(dir, path_index_file, r, min, max, &start, &flag, &tid);
    }
    
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
                if(is_active == 1)
                {
                    end = clock();
                    fprintf(stdout, "[%d] Indexing procedure is active for %ld\n", getpid(), (end-start)/CLOCKS_PER_SEC);
                }
                else
                    fprintf(stdout, "[%d] Indexing procedure is inactive\n", getpid());
                break;
            case SIGUSR2:
                pthread_mutex_lock(&(flag.is_sent_mutex));
                if(flag.is_sent) //signal from indexing thread
                {
                    alarm(interval);
                    flag.is_sent = 0;
                    is_active = 0;
                    pthread_mutex_unlock(&(flag.is_sent_mutex));
                }
                else //signal from main process
                {
                    pthread_mutex_unlock(&(flag.is_sent_mutex));
                    if(is_active == 1)
                        break;
                    is_active = 1;
                    tid = pthread_self();
                    indexing_procedure(dir, path_index_file, r, min, max, &start, &flag, &tid);
                }
                break;
            case SIGALRM:
                if(is_active == 0)
                {
                    is_active = 1;
                    tid = pthread_self();
                    indexing_procedure(dir, path_index_file, r, min, max, &start, &flag, &tid);              
                }
                break;
            case SIGINT:
                if(is_active == 1)
                {
                    pthread_cancel(tid);
                }
                quit = 1;
                break;
            default:
                fprintf(stderr, "Unexpected signal\n");
        }
    }

    if(pthread_sigmask(SIG_UNBLOCK, &mask, NULL))
        ERR("pthread_sigmask");

    //wait for cleanup handlers execution
    int still_working = 1;
    struct timespec t = {0, 10000000L};
    while(still_working)
    {
        pthread_mutex_lock(&(flag.is_working_mutex));
        if(flag.is_working == 0)
            still_working = 0;
        pthread_mutex_unlock(&(flag.is_working_mutex));
        nanosleep(NULL,&t);
    }

    //remove .numf_pid file
    if(unlink(path_pid_file))
        ERR("unlink");

    free(path_index_file);
    free(path_pid_file);

    exit(EXIT_SUCCESS);
}

void indexing_procedure(char *dir, char *path_index_file, int r, int min, int max, clock_t *start, signalFlag_t *flag, pthread_t *tid)
{
    workingThreadArgs_t *args = (workingThreadArgs_t*)malloc(sizeof(workingThreadArgs_t));
    if(args == NULL)
        ERR("malloc");
    
    char *temp_file_path = (char*)malloc(sizeof(char) * (strlen(dir) + strlen(TEMP_FILE_NAME) + 1));
    if(temp_file_path == NULL)
        ERR("malloc");
    strcpy(temp_file_path, dir);
    strcat(temp_file_path, TEMP_FILE_NAME);
    
    args->dir = dir;
    args->path_index_file = path_index_file;
    args->temp_file_path = temp_file_path;
    args->r = r;
    args->min = min;
    args->max = max;
    args->flag = flag;
    args->main_tid = *tid;
    
    pthread_attr_t threadAttr;
    if(pthread_attr_init(&threadAttr))
        ERR("pthread_attr_init");
    if(pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED))
        ERR("pthread_attr_setdetachstate");
    if(pthread_create(tid,&threadAttr,indexing_thread,args) != 0)
        ERR("pthread_create");
    *start = clock(); 
    pthread_attr_destroy(&threadAttr);
}

//indexing thread function
void* indexing_thread(void *voidArgs)
{
    
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    workingThreadArgs_t *args = voidArgs;
    pthread_mutex_lock(&(args->flag->is_working_mutex));
    args->flag->is_working = 1;
    pthread_mutex_unlock(&(args->flag->is_working_mutex));
    pthread_cleanup_push(indexing_thread_clean, args);

    srchArgs = (searchArgs_t*)malloc(sizeof(searchArgs_t));
    if(srchArgs == NULL)
        ERR("srchArgs");
    srchArgs->min = args->min;
    srchArgs->max = args->max;
    srchArgs->head = NULL;
    if(strlen(args->dir+1) > MAX_PATH)
        ERR("path length");
    srchArgs->numf_index_path = args->path_index_file;
    srchArgs->dir_with_index_file_path = (char*)malloc(sizeof(char)*strlen(args->dir)+1);
    if(srchArgs->dir_with_index_file_path == NULL)
        ERR("malloc");
    strcpy(srchArgs->dir_with_index_file_path,args->dir);
    
    if(args->r==0)
    {
        //searching only in given dir, not recursively
        //saving current and changing dir
        char cwd[MAX_PATH];
        if(getcwd(cwd, MAX_PATH) == NULL)
            ERR("getcwd");
        if(chdir(args->dir) == -1)
            ERR("chdir");
        
        DIR *dirp;
        struct dirent *dp;
        struct stat filestat;
        if((dirp = opendir(".")) == NULL)
            ERR("opendir");  
        do
        {
            errno = 0;
            if((dp = readdir(dirp)) != NULL)
            {
                if (lstat(dp->d_name, &filestat)) 
                    ERR("lstat");
                    if(S_ISREG(filestat.st_mode))
                        find_numbers(dp->d_name, args->min, args->max, 1);
            }
        }while(dp != NULL);
        
        //changing to dir where began
        if(chdir(cwd) == -1)
            ERR("chdir");
    }
    else
    {
        //searching recursively
        if(nftw(args->dir,find_numbers_recur, MAXFD,FTW_PHYS) != 0)
            ERR("nftw"); 
    }
    
    //writing to .numf_index file whole structure using .numf_temp file
    save_list_to_file(srchArgs->head, args->path_index_file, args->temp_file_path, &(args->fd)); 
    
    remove_index_list(&(srchArgs->head));
    free(args->temp_file_path);
    pthread_mutex_lock(&(args->flag->is_sent_mutex));
    args->flag->is_sent = 1;
    pthread_mutex_unlock(&(args->flag->is_sent_mutex));
    if(pthread_kill(args->main_tid,SIGUSR2))
        ERR("kill");

    free(srchArgs->dir_with_index_file_path);
    free(srchArgs);
    pthread_cleanup_pop(1);
    return NULL;
}

void indexing_thread_clean(void *voidArgs)
{
    workingThreadArgs_t *args = voidArgs;
    if(access(args->temp_file_path, F_OK) == 0)
    {
        if(TEMP_FAILURE_RETRY(close(args->fd)) != 0 && errno != EBADF)
            ERR("close");
        if(unlink(args->temp_file_path))
            ERR("unlink");     
    }
    pthread_mutex_unlock(&(args->flag->is_sent_mutex));
    pthread_mutex_lock(&(args->flag->is_working_mutex));
    args->flag->is_working = 0;
    pthread_mutex_unlock(&(args->flag->is_working_mutex));
    free(args);
}