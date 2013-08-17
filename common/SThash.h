#ifndef _ST_HASH_H
#define _ST_HASH_H

#include "STbase.h"

struct _ST_hash_node {
    void *data;
    ST_linker_t link;
};

typedef unsigned int (*ST_hash_index)(void *key);
typedef BOOL (*ST_hash_compare)(void *data, void *key, int *flag);
typedef void (*ST_hash_destroy) (void *data);

typedef struct _ST_hash {
    int size;
    struct _ST_hash_node *table;

    ST_hash_index   hash_index;
    ST_hash_compare hash_compare;
    ST_hash_destroy hash_destroy;
}ST_hash_t;


extern int ST_hash_init (ST_hash_t *hash, 
                         int size,
                         ST_hash_index index_routine,
                         ST_hash_compare compare_routine,
                         ST_hash_destroy destroy_routine);

extern int ST_hash_add_object (ST_hash_t *hash, void *data, void *key);
extern void ST_hash_remove_objects (ST_hash_t *hash, void *key);
extern void *ST_hash_get_objects (ST_hash_t *hash, void *key, int flag);
extern void ST_hash_remove_all_objects (ST_hash_t *hash);
extern void ST_hash_release (ST_hash_t *hash);
#endif
