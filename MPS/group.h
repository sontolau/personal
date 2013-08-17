#ifndef _MPS_GROUP_H
#define _MPS_GROUP_H

#include "mps.h"
#include "SThash.h"

typedef struct member {
    struct sock_fd *sock_fd;
    unsigned int uid;
    char         *name;

    ST_linker_t  list;
}member_t;

typedef struct group {
    unsigned int gid;
    char         *name;
    int          max_mbrs;
    int          num_mbrs;
    member_t     *mbr_master;
    ST_hash_t    mbrs;

    pthread_mutex_t  grp_mutex;

    ST_linker_t hash_list;
    ST_linker_t active_list;

}group_t;


#endif
