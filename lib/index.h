#ifndef index_h
#define index_h

void indexing_process_work(char *dir, int r, int min, int max, int interval);
void indexing_procedure(char *dir, char *path_index_file, int r, int min, int max, clock_t *start, signalFlag_t *flag, pthread_t *tid);
void* indexing_thread(void *voidArgs);
void indexing_thread_clean(void *voidArgs);

#endif