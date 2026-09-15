#ifndef ALLOCATOR_INTERNAL_H
#define ALLOCATOR_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

typedef struct block_header {
    size_t size;
    int is_free;
    struct block_header *next;
    struct block_header *prev;
} block_header_t;

/* Linked-list policies (first-fit, best-fit) */
block_header_t* nebula_alloc_get_head(void);
block_header_t* policy_first_fit_find(size_t size);
block_header_t* policy_best_fit_find(size_t size);

/* Buddy allocator */
void  buddy_init(void *pool, size_t pool_size);
void *buddy_allocate(size_t size);
void  buddy_deallocate(void *ptr);

/* Slab allocator */
void  slab_init(void *pool, size_t pool_size);
void *slab_allocate(size_t size);
void  slab_deallocate(void *ptr);

#endif
