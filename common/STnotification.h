#ifndef _ST_NOTIF_H
#define _ST_NOTIF_H

#include "STbase.h"

#ifdef OS_WINDOWS
#else
#include <unistd.h>
#include <pthread.h>
#endif

#define NOTIF_LEN_MAX  50

struct ST_notif_event {
    const char   *event;
    void         *data;
    void (*handler)(void*);
    ST_linker_t     link;
};


typedef struct _ST_notif {
    struct ST_notif_event notif_queue;

#ifdef OS_WINDOWS
    
#else
    int pipes[2];
    pthread_t  notif_thrd;
#endif
}ST_notif_t;


extern ST_notif_t *ST_notif_init ();
extern int ST_notif_add (ST_notif_t *notif, 
                      const char *msg, 
                      void (*notif_routine)(void*),
                      void*data);
extern void ST_notif_remove (ST_notif_t *notif,
                          const char *msg);
extern int ST_notif_send (ST_notif_t *notif,
                       const char *msg);
extern void ST_notif_destroy (ST_notif_t *notif);
#endif
