
#ifndef __ADX_TYPE_H__
#define __ADX_TYPE_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/socket.h>

// EPOLL
#define HAVE_EPOLL

// JEMALLOC
#define __JEMALLOC__

#ifdef __JEMALLOC__
#include <jemalloc/jemalloc.h>
#else
#define je_malloc malloc
#define je_realloc realloc
#define je_calloc calloc
#define je_free free
#endif

#define zmalloc je_malloc
#define zrealloc je_realloc
#define zcalloc je_calloc
#define zfree je_free

#endif


