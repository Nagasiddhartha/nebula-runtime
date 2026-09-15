/*
 * BUDDY ALLOCATOR
 * Memory is managed in power-of-2 sized blocks.
 * Allocation splits larger blocks; freeing merges "buddies" back together.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "allocator_internal.h"

#define MIN_BLOCK_SIZE 64
#define MAX_ORDER 20  /* 2^20 * 64 = 64MB max */

typedef struct free_node {
    struct free_node *next;
} free_node_t;

static free_node_t *free_lists[MAX_ORDER + 1];
static void *buddy_pool = NULL;
static size_t buddy_pool_size = 0;
static int max_order = 0;

/* Smallest order whose block size >= requested size */
static int size_to_order(size_t size) {
    size_t block = MIN_BLOCK_SIZE;
    int order = 0;
    while (block < size && order < max_order) {
        block <<= 1;
        order++;
    }
    return order;
}

static size_t order_to_size(int order) {
    return (size_t)MIN_BLOCK_SIZE << order;
}

/* Get the buddy address for a block at given order */
static void* get_buddy(void *block, int order) {
    uintptr_t offset = (uintptr_t)block - (uintptr_t)buddy_pool;
    uintptr_t buddy_offset = offset ^ order_to_size(order);
    return (void *)((uintptr_t)buddy_pool + buddy_offset);
}

void buddy_init(void *pool, size_t pool_size) {
    buddy_pool = pool;
    buddy_pool_size = pool_size;

    for (int i = 0; i <= MAX_ORDER; i++)
        free_lists[i] = NULL;

    /* Find the largest order that fits in the pool */
    max_order = 0;
    while (order_to_size(max_order + 1) <= pool_size && max_order < MAX_ORDER)
        max_order++;

    /* Add the entire pool as one big block at max_order */
    free_node_t *initial = (free_node_t *)pool;
    initial->next = NULL;
    free_lists[max_order] = initial;
}

void *buddy_allocate(size_t size) {
    if (!buddy_pool || size == 0) return NULL;

    /* Add space for a size tag so we know the order on free */
    size_t total = size + sizeof(size_t);
    int needed_order = size_to_order(total);

    if (needed_order > max_order) return NULL;

    /* Find the smallest order >= needed_order with a free block */
    int found_order = -1;
    for (int o = needed_order; o <= max_order; o++) {
        if (free_lists[o] != NULL) {
            found_order = o;
            break;
        }
    }
    if (found_order == -1) return NULL;

    /* Pop block from found_order */
    free_node_t *block = free_lists[found_order];
    free_lists[found_order] = block->next;

    /* Split down to needed_order */
    while (found_order > needed_order) {
        found_order--;
        void *buddy = (char *)block + order_to_size(found_order);
        free_node_t *buddy_node = (free_node_t *)buddy;
        buddy_node->next = free_lists[found_order];
        free_lists[found_order] = buddy_node;
    }

    /* Store the order at the beginning of the block for later free */
    size_t *tag = (size_t *)block;
    *tag = (size_t)needed_order;

    return (void *)((char *)block + sizeof(size_t));
}

void buddy_deallocate(void *ptr) {
    if (!ptr || !buddy_pool) return;

    /* Read the order tag */
    size_t *tag = (size_t *)((char *)ptr - sizeof(size_t));
    int order = (int)*tag;
    void *block = (void *)tag;

    /* Try to merge with buddy */
    while (order < max_order) {
        void *buddy = get_buddy(block, order);

        /* Check if buddy is in the free list at this order */
        free_node_t **list = &free_lists[order];
        int found = 0;
        while (*list) {
            if ((void *)*list == buddy) {
                /* Remove buddy from free list */
                *list = (*list)->next;
                found = 1;
                break;
            }
            list = &(*list)->next;
        }
        if (!found) break;

        /* Merge: use the lower address as the merged block */
        if (buddy < block) block = buddy;
        order++;
    }

    /* Add merged block to free list */
    free_node_t *node = (free_node_t *)block;
    node->next = free_lists[order];
    free_lists[order] = node;
}
