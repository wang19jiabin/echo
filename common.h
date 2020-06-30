#ifndef COMMON_H
#define COMMON_H

#include <sys/syscall.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define log(fmt, ...) do {\
        char str[128];\
        snprintf(str, sizeof(str), "%ld:%s:%d:%s:"fmt"\n", syscall(SYS_gettid),\
                __FILE__, __LINE__, __func__, ##__VA_ARGS__);\
        write(STDOUT_FILENO, str, strlen(str));\
} while (0)

#endif
