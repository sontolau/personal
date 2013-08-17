#include "SThash.h"

int ST_hash_init (ST_hash_t *hash, 
                  int size,
                  ST_hash_index index_routine,
                  ST_hash_compare compare_routine,
                  ST_hash_destroy destroy_routine)
{
    hash->size = size;
    hash->table = calloc (size, sizeof (struct _ST_hash_node));
    if (!hash->table) {
        return -1;
    }

    hash->hash_index = index_routine;
    hash->hash_compare = compare_routine;
    hash->hash_destroy = destroy_routine;
    return 0;
}

int ST_hash_add_object (ST_hash_t *hash, void *data, void *key)
{
    unsigned int idx;
    struct _ST_hash_node *node_head = NULL;
    struct _ST_hash_node *node      = NULL,
                        *next      = NULL;

    node = calloc (1, sizeof (struct _ST_hash_node));
    if (node == NULL) {
        return -1;
    }

    node->data = data;

    if (hash->hash_index) {
        idx = hash->hash_index (key);
    }

    node_head = &hash->table[idx % hash->size];
    next = node_head->link.next;
    node->link.prev = node_head;
    node->link.next = next;

    if (node_head) {
        node_head->link.next = node;
    }
    if (next) {
        next->link.prev = node;
    }

    return 0;
}

void ST_hash_remove_objects (ST_hash_t *hash, void *key)
{
    unsigned int idx = 0;
    int          flag = 0;
    struct _ST_hash_node *node = NULL,
                        *prev = NULL,
                        *next = NULL,
                        *tmpptr = NULL;

    idx = (hash->hash_index?hash->hash_index (key):idx);
    node = hash->table[idx % hash->size].link.next;

    while (node && hash->hash_compare) {
        tmpptr = node->link.next;
        if (hash->hash_compare (node->data, key, &flag)) {
            prev = node->link.prev;
            next = node->link.next;

            if (prev) {
                prev->link.next = next;
            }

            if (next) {
                next->link.prev = prev;
            }

            if (hash->hash_destroy) {
                hash->hash_destroy (node->data);
            }

            free (node);
            
            if (!flag) { //if returned a zero,stop searching.
                break;
            }
        }
        node = tmpptr;
    }
}

void *ST_hash_get_object (ST_hash_t *hash, void *key, int flag)
{
    unsigned int idx = 0;
    static struct _ST_hash_node *node = NULL;

    idx = (hash->hash_index?hash->hash_index (key):idx);
    if (flag || node == NULL) {
        node = hash->table[idx % hash->size].link.next;
    }

    while (node && hash->hash_compare) {
        if (hash->hash_compare (node->data, key,&flag )) {
            return node->data;
        }
        node = node->link.next;
    }

    return NULL;
}

void ST_hash_remove_all_objects (ST_hash_t *hash)
{
    int i = 0;
    struct _ST_hash_node *node = NULL,
                        *tmpptr = NULL;

    for (i=0; i<hash->size; i++) {
        node = hash->table[i].link.next;
        hash->table[i].link.next = NULL;

        while (node) {
            tmpptr = node->link.next;
            if (hash->hash_destroy) {
                hash->hash_destroy (node->data);
            }

            free (node);
            node = tmpptr;
        }
    }
}

void ST_hash_release (ST_hash_t *hash)
{
    ST_hash_remove_all_objects (hash);
    free (hash->table);

    memset (hash, '\0', sizeof (ST_hash_t));
}
