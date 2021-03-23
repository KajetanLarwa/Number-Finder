#ifndef io_h
#define io_h

#include "common.h"

ssize_t bulk_read(int fd, void *buf, size_t nbyte);
ssize_t bulk_write(int fd, void *buf, size_t nbyte);

#endif