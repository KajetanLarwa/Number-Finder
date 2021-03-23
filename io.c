#define _GNU_SOURCE
#include <unistd.h>
#include <errno.h>

#include "lib/common.h"
#include "lib/io.h"

ssize_t bulk_read(int fd, void *buf, size_t nbyte)
{
    ssize_t bytes_read = 0;
    ssize_t temp_bytes_read;
    do
    {
        temp_bytes_read = TEMP_FAILURE_RETRY(read(fd, buf, nbyte));
        if(temp_bytes_read == 0)
            return bytes_read;
        if(temp_bytes_read < 0)
            return temp_bytes_read;
        nbyte -= temp_bytes_read;
        buf += temp_bytes_read;
        bytes_read += temp_bytes_read;
    }while(nbyte > 0);
    return bytes_read;
}

ssize_t bulk_write(int fd, void *buf, size_t nbyte)
{
    ssize_t bytes_read = 0;
    ssize_t temp_bytes_read;
    do
    {
        temp_bytes_read = TEMP_FAILURE_RETRY(write(fd, buf, nbyte));
        if(temp_bytes_read < 0)
            return temp_bytes_read;
        nbyte -= temp_bytes_read;
        buf += temp_bytes_read;
        bytes_read += temp_bytes_read;
    }while(nbyte > 0);
    return bytes_read;
}