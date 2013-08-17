#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>

#include "proto.h"

#include "mps.h"


typedef struct mps {
    int      sock_fd;
    unsigned int gid;
    unsigned int uid;
    pid_t    serv_pid;

    void     *priv_data;
}mps_t;
#define MPS(ptr)  ((struct mps*)ptr)



/*
 * a set of structures as below are used to create
 * MPS server.
 */

#define GID_BASE   40000
#define UID_BASE   100000

struct list {
    void *next;
    void *prev;
};

struct sock_fd {
    int sock_fd;
    struct pollfd *pollptr;
    void *data;
    struct list list;
};

typedef struct member {
    struct sock_fd *sock_fd;
    unsigned int uid;
    char         *name;

    struct list  list;
}member_t;

typedef struct group {
    unsigned int gid;
    char         *name;
    int          max_mbrs;
    int          num_mbrs;
    member_t     *mbr_master;
    member_t     mbrs_list;
    
    pthread_mutex_t  grp_mutex;

    struct list hash_list;
    struct list active_list;

//    struct group *next;
//    struct group *prev;
}group_t;

#define HASH_SIZE(num)   

struct mps_struct {
    int max_groups;
    int num_groups;
    group_t  *group_table;
    group_t   active_list;
    pthread_mutex_t mps_mutex;

    int num_sockets;
    struct sock_fd sock_list;
    int     *sock_queue;
    int     *sock_tail;
    int     *sock_head;

    volatile int exit_flag;
};


//////////////////////////////////
static struct mps_struct *mps_struct = NULL;


static unsigned int __gid ()
{
    static unsigned int gid = GID_BASE;
    return gid++;
}

static unsigned int __uid ()
{
    static unsigned int uid = UID_BASE;
    return uid++;
}

static group_t *__new_group (const char *name, int max_mbrs)
{
    group_t *grp = NULL;

    grp = calloc (1, sizeof (group_t));
    if (grp) {
        grp->name = strdup (name);
        grp->max_mbrs = max_mbrs;
        grp->hash_list.next = NULL;
        grp->hash_list.prev = NULL;

        grp->active_list.next = NULL;
        grp->active_list.prev = NULL;
        grp->gid            = __gid ();
        grp->num_mbrs       = 0;

        if (pthread_mutex_init (&grp->grp_mutex, NULL) < 0) {
            free (grp);
            return NULL;
        }
    }
    return grp;
}

static member_t *__new_member (const char *name)
{
    member_t *mbr = NULL;

    mbr = calloc (1, sizeof (member_t));
    if (mbr) {
        mbr->uid = __uid ();
        mbr->name = (name?strdup (name):NULL);

        mbr->list.next = NULL;
        mbr->list.prev = NULL;
    }
    return mbr;
}

static struct sock_fd *__new_sock_fd (int sock, void *data)
{
    struct sock_fd *sf = calloc (1, sizeof (struct sock_fd));

    if (sf) {
        sf->sock_fd = sock;
        sf->data    = data;
    }
    return sf;
}

static void __release_sock_fd (struct sock_fd *sf)
{
    close (sf->sock_fd);
    free (sf);
}
static void __add_new_sock_fd (struct mps_struct *ms, struct sock_fd *sf)
{
    struct sock_fd *prev = &ms->sock_list,
                   *next = ms->sock_list.list.next;

    if (prev) {
        sf->list.next = next;
        sf->list.prev = prev;
    }

    if (next) {
        next->list.prev = sf;
    }

    ms->num_sockets++;
}


static void __remove_sock_fd (struct mps_struct *ms, struct sock_fd *sf)
{
    struct sock_fd *prev = sf->list.prev,
                   *next = sf->list.next;

    if (prev) {
        prev->list.next = next;
    }

    if (next) {
        next->list.prev = prev;
    }
    ms->num_sockets--;
}

static void __release_group (group_t *grp)
{
    if (grp) {
        if (grp->name) free (grp->name);
        if (grp->mbr_master) free (grp->mbr_master);
        pthread_mutex_destroy (&grp->grp_mutex);
        free (grp);
    }
}

static void __release_member (member_t *mbr)
{
    if (mbr) {
        if (mbr->name) free (mbr->name);
        free (mbr);
    }
}

static void __remove_member (group_t *grp, member_t *mbr)
{
    member_t *prev = NULL,
             *next = NULL;

    prev = (member_t*)mbr->list.prev;
    next = (member_t*)mbr->list.next;

    if (prev) {
        prev->list.next = next;
    }

    if (next) {
        next->list.prev = prev;
    }

    grp->num_mbrs--;
}

static void __clean_group (group_t *grp)
{
    member_t  *mbr = NULL,
              *nmbr = NULL;
    mbr = grp->mbrs_list.list.next;
    while (mbr) {
        nmbr = mbr->list.next;
        __remove_member (grp, mbr);
        __release_member (mbr);
        mbr = nmbr;
    }
    __release_group (grp);
}

static void __add_member_to_group (group_t *grp, member_t *mbr)
{
    member_t *next = NULL;

    next = grp->mbrs_list.list.next;

    mbr->list.next = next;
    grp->mbrs_list.list.next = mbr;

    if (next) {
        next->list.prev = mbr;
    }
    grp->num_mbrs++;
}

static struct mps_struct *__init_mps_struct (int max_groups)
{
    struct mps_struct *ms = NULL;

    if (!ms) {
        ms = calloc (1, sizeof (struct mps_struct));
    }

    if (ms) {
        ms->max_groups  = max_groups;
        ms->group_table = calloc (HASH_SIZE, sizeof (group_t));
        if (ms->group_table == NULL) {
__err_quit:
            free (ms);
            return NULL;
        }

        ms->sock_queue = calloc (HASH_SIZE, sizeof (int));
        if (ms->sock_queue == NULL) {
            goto __err_quit;
        }

        ms->sock_head = ms->sock_queue;
        ms->sock_tail = ms->sock_queue;

        pthread_mutex_init (&ms->mps_mutex, NULL);
    }

    return ms;
}

static void __remove_group (struct mps_struct*, group_t*);
static void __free_mps (mps_t *mps)
{
    struct sock_fd *skptr = NULL;
    group_t        *grpptr = NULL;
    struct mps_struct *ms = mps->priv_data;
    void           *tmp;
    close (mps->sock_fd);

    grpptr = ms->active_list.active_list.next;
    while (grpptr) {
        tmp = grpptr->active_list.next;
        __clean_group (grpptr);
        __remove_group (ms, grpptr);
        __release_group (grpptr);
        grpptr = tmp;
    }

    skptr = ms->sock_list.list.next;
    while (skptr) {
        tmp = skptr->list.next;
        __remove_sock_fd (ms, skptr);
        __release_sock_fd (skptr);
        skptr = tmp;
    }

    if (ms->sock_queue) {
        free (ms->sock_queue);
    }
}

static group_t *__find_group_by_id (struct mps_struct *ms, 
                                    unsigned int gid)
{
    int index = (gid % GID_BASE);
    group_t *grp_ptr = NULL;

    grp_ptr = ms->group_table[index].hash_list.next;
    while (grp_ptr) {
        if (grp_ptr->gid == gid) {
            return grp_ptr;
        }
        grp_ptr = grp_ptr->hash_list.next;
    }
    return NULL;
}

static void __remove_group (struct mps_struct *ms, group_t *grp)
{
    group_t *next = NULL,
            *prev = NULL;

    //remove group from hash list.
    next = grp->hash_list.next;
    prev = grp->hash_list.prev;
    if (prev) {
        prev->hash_list.next = next;
    }

    if (next) {
       next->hash_list.prev = prev;
    }

    //remove group from active list.
    next = grp->active_list.next;
    prev = grp->active_list.prev;
    
    if (prev) {
        prev->active_list.next = next;
    }

    if (next) {
        next->active_list.prev = prev;
    }

    ms->num_groups--;
}

static void __add_group (struct mps_struct *ms, group_t *grp)
{
    int grp_index = (grp->gid % GID_BASE);
    group_t *prev = NULL,
            *next = NULL;

    //add group to hash list.
    prev = &ms->group_table[grp_index];
    next = ms->group_table[grp_index].hash_list.next;
    if (prev) {
        grp->hash_list.next = prev->hash_list.next;
        grp->hash_list.prev = prev;
    }

    if (next) {
        next->hash_list.prev = grp;
    }

    //add group to active list.
    prev = &ms->active_list;
    next = ms->active_list.active_list.next;
    if (prev) {
        grp->active_list.next = prev->active_list.next;
        grp->active_list.prev = prev;
    }

    if (next) {
        next->active_list.prev = grp;
    }

    ms->num_groups++;
}

static int __add_socket_into_queue (struct mps_struct *ms,int sock)
{
    int ret = 0;
    if (ms->sock_tail == ms->sock_head && *ms->sock_head != 0) {
        ret = -1;
    } else {
        *ms->sock_tail = sock;
        ms->sock_tail++;
        ms->sock_tail = ms->sock_queue + ((ms->sock_tail - ms->sock_queue)%HASH_SIZE);
    }
    return ret;
}


static int __get_socket_from_queue (struct mps_struct *ms)
{
    int sock = -1;
    if (ms->sock_tail == ms->sock_head && *ms->sock_head == 0) {
    } else {
        sock = *ms->sock_head;
        *ms->sock_head = 0;

        ms->sock_head++;
        ms->sock_head = ms->sock_queue + ((ms->sock_head - ms->sock_queue)%HASH_SIZE);
    }
    return sock;
}
static unsigned char __is_group_full (struct mps_struct *ms)
{
    if (ms->max_groups) {
        return (ms->num_groups>=ms->max_groups)?1:0;
    }
    return 0;
}

static unsigned char __is_member_full (group_t *grp)
{
    if (grp->max_mbrs) {
        return (grp->num_mbrs>=grp->max_mbrs)?1:0;
    }
    return 0;
}

static struct pollfd *__pollfds_from (struct mps_struct *ms,int *num)
{
    struct sock_fd *sockptr = ms->sock_list.list.next;
    struct pollfd *fds = NULL;
    int i = 0;

    fds = calloc (ms->num_sockets, sizeof (struct pollfd));
    if (fds) {
        while (sockptr) {
            fds[i].fd = sockptr->sock_fd;
            fds[i].events = POLLIN | POLLERR;
            fds[i].revents = 0;
            sockptr->pollptr = &fds[i];
            i++;

            sockptr = sockptr->list.next;
        }
    }
    *num = ms->num_sockets;
    return fds;
}

static void __sig_proc (int sig)
{
    switch (sig) {
        case SIGINT:
        case SIGTERM:
        case SIGKILL:

            mps_struct->exit_flag = 1;
            break;
    }
}


static int __request_core_proc (MPS_HEADER *mhdr, struct sock_fd *sf, struct mps_struct *ms)
{
    switch (mhdr->command) {
        case MPS_OP_REG:
        case MPS_OP_UNREG:
        case MPS_OP_ADD:
        case MPS_OP_QUIT:
        case MPS_OP_PUSH:
            break;
    }
    return 0;
}
static int __do_request (int nread, struct mps_struct *ms)
{
    struct sock_fd *sfptr = NULL;
    MPS_HEADER     mps_hdr;
    int       ret = 0;
    void *tmpptr = NULL;
    pthread_mutex_lock (&ms->mps_mutex);
    sfptr = ms->sock_list.list.next;
    while (sfptr) {
        tmpptr = sfptr->list.next;
        if (sfptr->pollptr && (sfptr->pollptr->revents & (POLLERR | POLLIN))) {
            nread--;
 
            ret = recv (sfptr->pollptr->fd, &mps_hdr, SZMPSHDR, 0);
            if (ret < SZMPSHDR) {
__rm_client:
                __remove_sock_fd (ms, sfptr);
                __release_sock_fd (sfptr);
                ret = 1;
            } else {
                if (memcmp (mps_hdr.ident, MPS_IDENTIFIER, 4) ||
                    __request_core_proc (&mps_hdr, sfptr, ms)) {
                    goto __rm_client;
                }
            }
        }
        if (nread <= 0) {
            break;
        }
        sfptr = tmpptr;
    }
    pthread_mutex_unlock (&ms->mps_mutex);
    return ret;
}

static void *__core_proc_thread (void *data)
{
    struct pollfd *fds = NULL;
    int ret, nfds = 0;
    int sock;
    mps_t  *mps = (mps_t*)data;
    struct sock_fd *sf = NULL;
    int refresh = 0;
    struct mps_struct *ms = mps->priv_data;

    do {
        ret = poll (fds, nfds, 500);
        if (ret < 0) {
            break;
        } else if (ret == 0) {
__refresh_poll:
            pthread_mutex_lock (&ms->mps_mutex);
            while ((sock = __get_socket_from_queue (ms))>0) {
                sf = __new_sock_fd (sock, NULL);
                __add_new_sock_fd (ms, sf);
                refresh = 1;
            }
            if (refresh) {
                refresh = 0;
                if (fds) free (fds);
                fds = __pollfds_from (ms, &nfds);
            }
            pthread_mutex_unlock (&ms->mps_mutex);
            continue;
        } else {
            if (__do_request (ret, ms)) {
                goto __refresh_poll;
            }
        }
    } while (!ms->exit_flag);
    return NULL;
}

static void __add_new_client (int client, mps_t *mps)
{
    struct mps_struct *ms = (struct mps_struct*)mps->priv_data;

    if (client < 0) return;

    if (pthread_mutex_lock (&ms->mps_mutex) == 0) {
        if (__add_socket_into_queue (ms, client) < 0) {
            close (client);
        }
        pthread_mutex_unlock (&ms->mps_mutex);
    } else {
        close (client);
    }
}

static int __mps_server (mps_t *mps, int max_groups)
{
    pthread_t proc_thrd;
    fd_set readfds;
    struct timeval timeout = {1, 0};
    int ret;

    daemon (0,0);
    mps->priv_data = mps_struct = __init_mps_struct (max_groups);
    if (mps_struct == NULL) {
        return -1;
    }

    signal (SIGALRM, SIG_IGN);
    signal (SIGTERM, __sig_proc);
    signal (SIGKILL, __sig_proc);
    signal (SIGINT,  __sig_proc);

    if (pthread_create (&proc_thrd, NULL, __core_proc_thread, mps) < 0) {
        free (mps_struct);
        return -1;
    }

    FD_ZERO (&readfds);
    FD_SET  (mps->sock_fd, &readfds);
    
    do {
        timeout.tv_sec = 1;
        ret = select (mps->sock_fd+1, &readfds, NULL, NULL, &timeout);
        if (ret < 0) {
            break;
        } else if (ret == 0) {
            continue;
        } else {
            if (FD_ISSET (mps->sock_fd, &readfds)) {
                __add_new_client (accept (mps->sock_fd, NULL, NULL), mps);
            }
        }
    } while (!mps_struct->exit_flag);

    pthread_join (proc_thrd, NULL);
    __free_mps (mps);
    return 0;
}


HMPS  mps_init (const char *ip,
                unsigned short port,
                int flag,
                int max_groups)
{
    mps_t   *mps = NULL;
    int     sock;
    int     sockflag = 1;
    struct sockaddr_in addrinfo;

    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return NULL;
    }

    
    if ((mps = calloc (1, sizeof (mps_t)))) {
        addrinfo.sin_family = AF_INET;
        addrinfo.sin_port   = htons (port);
        addrinfo.sin_addr.s_addr = ip?0:inet_addr (ip);
        mps->sock_fd        = sock;
        if (!flag) {
            if (connect (sock, (struct sockaddr*)&addrinfo, sizeof (addrinfo)) < 0) {
__err_quit:
                close (sock);
                free (mps);
                return NULL;
            }
        } else {
            setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &sockflag, sizeof (int));
            if (bind (sock, (struct sockaddr*)&addrinfo, sizeof (addrinfo)) < 0) {
                goto __err_quit;
            }
            if (listen (sock, 100) < 0) {
                goto __err_quit;
            }

            mps->serv_pid = fork ();
            if (mps->serv_pid < 0) {
                goto __err_quit;
            } else if (mps->serv_pid == 0) {
                exit (__mps_server (mps, max_groups));
            } else {
                close (sock);
            }
        }
    }
    return mps;
}

int main()
{
    HMPS *mps = mps_init ("127.0.0.1", 8787, 1, 10);
    if (mps==NULL) {
        printf ("mps_init failed\n");
        return -1;
    }

    return 0;
}
