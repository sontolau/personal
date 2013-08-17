#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef OS_WINDOWS
#else
#include <errno.h>
#include <sys/select.h>
#endif


#include "STnotification.h"

static struct ST_notif_event *
__find_ST_notif_event (struct ST_notif_event *head,
                                                     const char *msg)
{
    struct ST_notif_event *ne = NULL;

    ne = head->link.next;
    while (ne) {
        if (!strcmp (ne->event, msg)) {
            return ne;
        }
        ne = ne->link.next;
    }
    return NULL;
}


static void __handle_notif (ST_notif_t *notif, const char *msg)
{
    struct ST_notif_event *ne = NULL;
    pthread_t        ST_notif_thrd;

    ne = __find_ST_notif_event (&notif->notif_queue, msg);
    if (ne && ne->handler) {
        pthread_create (&ST_notif_thrd, NULL, (void*)ne->handler, ne->data);
    }
}

static void *__notif_core (void *data)
{
    fd_set rdfds;
    ST_notif_t *notif = (ST_notif_t*)data;
    int ret;
    char msg[NOTIF_LEN_MAX + 1] = {0};
    unsigned char cmd = 0;

    FD_ZERO (&rdfds);
    FD_SET  (notif->pipes[0], &rdfds);

    while (1) {
        ret = select (notif->pipes[0] + 1, &rdfds, NULL, NULL, NULL);
        if (ret <= 0) {
            if (errno == EINTR) {
                continue;
            } else {
                break;
            }
        } else if (ret > 0) {
            if (FD_ISSET (notif->pipes[0], &rdfds)) {
                if (read (notif->pipes[0], &cmd, 1) < 0 || cmd == 0) {
                    break;
                } else {
                    ret = read (notif->pipes[0], msg, NOTIF_LEN_MAX);
                    if (ret < 0) {
                        break;
                    } else {
                        __handle_notif (notif, msg);
                    }
                }
            }
        }
    }

    return NULL;
}

ST_notif_t *ST_notif_init ()
{
    ST_notif_t *notif = NULL;

    notif = calloc (1, sizeof (ST_notif_t));
    if (notif == NULL) {
        return NULL;
    }

    if (pipe (notif->pipes) < 0) {
        free (notif);
        return NULL;
    }


    if (pthread_create (&notif->notif_thrd, NULL, __notif_core, notif) < 0) {
        close (notif->pipes[0]);
        close (notif->pipes[1]);
        free (notif);
        return NULL;
    }

    return notif;
}

int ST_notif_add (ST_notif_t *notif, 
               const char *msg, 
               void (*notif_routine)(void*),
               void*data)
{
    struct ST_notif_event *ne = NULL,
                       *head = NULL,
                       *next = NULL;

    ne = calloc (1, sizeof (struct ST_notif_event));
    if (ne == NULL) {
        return -1;
    }

    ne->event = msg;
    ne->handler = notif_routine;
    ne->data  = data;

    head = &notif->notif_queue;
    next = notif->notif_queue.link.next;

    ne->link.prev = head;
    if (head) {
        head->link.next = ne;
    }

    ne->link.next = next;
    if (next) {
        next->link.prev = ne;
    }

    return 0;
}

void ST_notif_remove (ST_notif_t *notif,
                   const char *msg)
{
    struct ST_notif_event *ne = NULL,
                       *prv = NULL,
                       *nxt = NULL;

    ne = __find_ST_notif_event (&notif->notif_queue, msg);
    if (ne) {
        prv = ne->link.prev;
        nxt = ne->link.next;

        if (prv) {
            prv->link.next = nxt;
        }

        if (nxt) {
            nxt->link.prev = prv;
        }

        free (ne);
    }
}

int ST_notif_send (ST_notif_t *notif,
                const char *msg)
{
    unsigned char bytes[NOTIF_LEN_MAX + 1] = {0};

    bytes[0] = 1;
    strncpy ((char*)&bytes[1], msg, NOTIF_LEN_MAX);

    return write (notif->pipes[1], bytes, sizeof (bytes));
}

void ST_notif_destroy (ST_notif_t *notif)
{
    unsigned char cmd = 0;

    if (write (notif->pipes[1], &cmd, 1) > 0) {
        pthread_join (notif->notif_thrd, NULL);
    }

    close (notif->pipes[0]);
    close (notif->pipes[1]);
    free (notif);
}
