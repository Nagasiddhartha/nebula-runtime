#ifndef NEBULA_ALLOC_H
#define NEBULA_ALLOC_H

#include <stddef.h>

typedef enum {
    NEBULA_FIRST_FIT,
    NEBULA_BEST_FIT,
    NEBULA_BUDDY,
    NEBULA_SLAB
} nebula_alloc_policy_t;

typedef struct {
    size_t total_pool_size;
    size_t total_allocated;
    size_t total_free;
    size_t largest_free_block;
    size_t num_free_blocks;
} nebula_alloc_stats_t;

void  nebula_alloc_init(size_t size);
void  nebula_alloc_destroy(void);
void  nebula_alloc_set_policy(nebula_alloc_policy_t policy);
void *nebula_malloc(size_t size);
void  nebula_free(void *ptr);
void  nebula_alloc_get_stats(nebula_alloc_stats_t *stats);
void  nebula_alloc_print_stats(void);

#endif
