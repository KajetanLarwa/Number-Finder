#ifndef find_h
#define find_h

int find_numbers_recur(const char *path, const struct stat *s, int type, struct FTW *f);
void find_numbers(const char *path, int min, int max, int is_path_relative_to_idx);

#endif