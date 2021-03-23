#ifndef tool_funs_h
#define tool_funs_h

int is_number(char *data);
void usage(char* pname);
void command_usage();
int number_digits(int num);
char* make_rel_path(char* main_path, const char* path);
int is_given_file(const char *path, const char *name);
void set_mask(sigset_t *mask);

#endif