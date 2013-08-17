#ifndef _ST_H
#define _ST_H

#if defined (WINDOWS) || defined (WINNT)
#include <windows.h>
#define OS_WINDOWS
#else
#include <unistd.h>
#endif

typedef struct _ST_linker {
    void *next;
    void *prev;
}ST_linker_t;


typedef unsigned char BOOL;

#ifndef FALSE
#define FALSE 0
#endif


#ifndef TRUE
#define TRUE (!FALSE)
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif
