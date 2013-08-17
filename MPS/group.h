#ifndef _MPS_GROUP_H
#define _MPS_GROUP_H

#include "mps.h"
#include "SThash.h"

#define NUM_OBJS_EACH_LIST 20

typedef struct member {
    int          sock;
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

    ST_linker_t hash_list;
    ST_linker_t active_list;

}group_t;

extern group_t *new_group (const char *name, int num_mbrs);
extern void    destroy_group (group_t *grp);
extern member_t *new_member (const char *mbr);
extern void    destroy_member (member_t *mbr);
extern int     add_new_member (group_t *grp, member_t *mbr);
extern void    remove_member (group_t *grp, member_t *mbr);
#endif
