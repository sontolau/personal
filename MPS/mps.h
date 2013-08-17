#ifndef _MPS_H
#define _MPS_H

#include "STbase.h"

struct sock_fd {
    int sock_fd;
    struct pollfd *pollptr;
    void *data;
    ST_linker_t list;
};

typedef struct _MPS_MSG {
    unsigned int MID;      //message ID
    unsigned int GID;      //group ID
    unsigned int srcID;    //the source ID
    unsigned int dstID;      //the destination ID, for all if zero.
    unsigned int length; //the length of message
    void         *message;
}MPS_MSG;

typedef void* HMPS;

/*
 * mps_init will return a object referred to HMPS, the 
 * 'max_groups' argument will be ignored if flag is zero
 */
extern HMPS  mps_init (const char *ip, 
                       unsigned short port, 
                       int flag, 
                       int max_groups);

/*
 * A group will be registered to the server, the 'max_mbrs'
 * indicates the maximum number of members owned by the group,
 * if 'flag' is non-zero ,the anyone of the group can push 
 * message to others, otherwise only the owner of the group
 * can push message.
 */
extern int   mps_register (HMPS mps, 
                           const char *group, 
                           unsigned int max_mbrs,
                           int flag);

/*
 * to unregister a group, just the owner of the group
 * can do this.
 */
extern int   mps_unregister (HMPS mps);

/*
 * to broadcast a message to all members of the group.
 */
extern int   mps_push (HMPS, void *msg, unsigned int len);

/*
 * add a member to a group.
 */
extern int   mps_add_to (HMPS mps, const char *group);
extern int   mps_quit_from (HMPS);
extern void  mps_destroy (HMPS);
#endif
