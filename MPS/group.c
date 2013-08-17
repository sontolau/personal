#include "group.h"


static unsigned int __member_hash_index (void *key)
{
    unsigned int gid = (unsigned int)key;

    return ((gid>>16)&0x0000FFFF)-1;
}

static unsigned int __str2id (int hash_sz, const char *name)
{
    static unsigned int short hi2 = 0;
    unsigned short low2 = 0;
    int i = 0;
    unsigned int gid = 0;

    for (i=0; i<strlen (name); i++) {
        low2 += (unsigned short)name[i];
    }

    hi2 %= hash_sz;
    
    gid = ((hi2 + 1)<<16 | low2);
    return gid;
}

static BOOL __member_hash_compare (void *data, void *key, int *flag)
{
    member_t *mbr = (member_t*)data;
    unsigned char type = ((unsigned char*)key)[0];
    void          *rkey = (char*)key + 1;

    *flag = 0;
    if (type == 0) {//validate GID
        return (mbr->gid == *(unsigned int*)rkey)?TRUE:FALSE;
    } else {
        return strcmp (mbr->name, (char*)rkey)?FALSE:TRUE;
    }
    return FALSE;
}

static void __member_hash_destroy (void *data)
{
    member_t *mbr = (member_t*)data;

    if (mbr->name) {
        free (mbr->name);
    }

    close (mbr->sock);
}

group_t *new_group (const char *name, int num_mbrs)
{
    float hash_sz = num_mbrs / NUM_OBJS_EACH_LIST;

    hash_sz = ((int)hash_sz < hash_sz)?(hash_sz + 1):hash_sz;
    group_t *grp = calloc (1, sizeof (group_t));
    if (grp == NULL) {
        return NULL;
    }

    if (ST_hash_init (&grp->mbrs, 
                      (int)hash_sz,
                      __member_hash_index,
                      __member_hash_compare,
                      __member_hash_destroy) < 0) {
        free (grp);
        return NULL;
    }

    grp->max_mbrs = num_mbrs;
    grp->num_mbrs = 0;
    
    return grp;
}

extern void    destroy_group (group_t *grp);
extern member_t *new_member (const char *mbr);
extern void    destroy_member (member_t *mbr);
extern int     add_new_member (group_t *grp, member_t *mbr);
extern void    remove_member (group_t *grp, member_t *mbr);

