#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include "nebula_alloc.h"
#include "allocator_internal.h"

#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define HEADER_SIZE ALIGN(sizeof(block_header_t))
#define MIN_SPLIT_SIZE 64

typedef struct {
    void *pool_start;
    size_t pool_size;
    block_header_t *head;
    nebula_alloc_policy_t policy;
} allocator_state_t;

static allocator_state_t state = {0};

typedef block_header_t* (*find_block_fn)(size_t aligned_size);
static find_block_fn current_find_fn = NULL;
static int policy_locked = 0;

typedef void* (*alloc_fn)(size_t size);
typedef void  (*dealloc_fn)(void *ptr);
static alloc_fn   current_alloc_fn   = NULL;
static dealloc_fn current_dealloc_fn = NULL;

block_header_t* nebula_alloc_get_head(void) {
    return state.head;
}

void nebula_alloc_init(size_t size) {
    if (state.pool_start != NULL) return;
    state.pool_start = mmap(NULL, size, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (state.pool_start == MAP_FAILED) {
        perror("[Nebula] mmap failed");
        state.pool_start = NULL;
        return;
    }
    state.pool_size = size;
    state.head = (block_header_t *)state.pool_start;
    state.head->size = size - HEADER_SIZE;
    state.head->is_free = 1;
    state.head->next = NULL;
    state.head->prev = NULL;
    policy_locked = 0;
    nebula_alloc_set_policy(NEBULA_FIRST_FIT);
    printf("[Nebula] Rented %zu bytes from OS\n", size);
}

void nebula_alloc_destroy(void) {
    if (state.pool_start != NULL) {
        if (state.policy == NEBULA_SLAB) slab_destroy();
        munmap(state.pool_start, state.pool_size);
        state.pool_start = NULL;
        state.head = NULL;
        policy_locked = 0;
        printf("[Nebula] Memory returned to OS\n");
    }
}
void nebula_alloc_set_policy(nebula_alloc_policy_t policy) {
    if (policy_locked && policy != state.policy) {
        fprintf(stderr, "[Nebula] cannot change policy after first allocation\n");
        return;
    }
    state.policy = policy;
    current_alloc_fn = NULL;
    current_dealloc_fn = NULL;
    current_find_fn = NULL;

    switch (policy) {
        case NEBULA_FIRST_FIT:
            current_find_fn = policy_first_fit_find;
            printf("[Nebula] Policy: FIRST FIT\n");
            break;
        case NEBULA_BEST_FIT:
            current_find_fn = policy_best_fit_find;
            printf("[Nebula] Policy: BEST FIT\n");
            break;
        case NEBULA_BUDDY:
            buddy_init(state.pool_start, state.pool_size);
            current_alloc_fn = buddy_allocate;
            current_dealloc_fn = buddy_deallocate;
            printf("[Nebula] Policy: BUDDY\n");
            break;
        case NEBULA_SLAB:
            slab_init(state.pool_start, state.pool_size);
            current_alloc_fn = slab_allocate;
            current_dealloc_fn = slab_deallocate;
            printf("[Nebula] Policy: SLAB\n");
            break;
    }
}

void *nebula_malloc(size_t size) {
    if (size == 0 || state.pool_start == NULL) return NULL;
    policy_locked = 1;

    if (current_alloc_fn) return current_alloc_fn(size);
    if (!current_find_fn) return NULL;

    size_t aligned_size = ALIGN(size);
    block_header_t *blk = current_find_fn(aligned_size);
    if (!blk) {
        printf("[Nebula] Out of Memory!\n");
        return NULL;
    }
    if (blk->size >= aligned_size + HEADER_SIZE + MIN_SPLIT_SIZE) {
        block_header_t *nb =
            (block_header_t *)((char *)blk + HEADER_SIZE + aligned_size);
        nb->size = blk->size - aligned_size - HEADER_SIZE;
        nb->is_free = 1;
        nb->next = blk->next;
        nb->prev = blk;
        if (blk->next) blk->next->prev = nb;
        blk->next = nb;
        blk->size = aligned_size;
    }
    blk->is_free = 0;
    return (void *)((char *)blk + HEADER_SIZE);
}

static void coalesce(block_header_t *blk) {
    if (blk->next && blk->next->is_free) {
        blk->size += HEADER_SIZE + blk->next->size;
        blk->next = blk->next->next;
        if (blk->next) blk->next->prev = blk;
    }
    if (blk->prev && blk->prev->is_free) {
        blk->prev->size += HEADER_SIZE + blk->size;
        blk->prev->next = blk->next;
        if (blk->next) blk->next->prev = blk->prev;
    }
}

void nebula_free(void *ptr) {
    if (!ptr || !state.pool_start) return;

    if (current_dealloc_fn) {
        current_dealloc_fn(ptr);
        return;
    }

    block_header_t *blk = (block_header_t *)((char *)ptr - HEADER_SIZE);
    if ((uintptr_t)blk < (uintptr_t)state.pool_start ||
        (uintptr_t)blk >= (uintptr_t)state.pool_start + state.pool_size) {
        fprintf(stderr, "[Nebula] invalid free at %p\n", ptr);
        return;
    }
    if (blk->is_free) {
        fprintf(stderr, "[Nebula] double free detected at %p\n", ptr);
        return;
    }
    blk->is_free = 1;
    coalesce(blk);
}

void nebula_alloc_get_stats(nebula_alloc_stats_t *s) {
    if (!s || !state.pool_start) return;
    s->total_pool_size = state.pool_size;
    s->total_allocated = 0;
    s->total_free = 0;
    s->largest_free_block = 0;
    s->num_free_blocks = 0;
    block_header_t *c = state.head;
    while (c) {
        if (c->is_free) {
            s->total_free += c->size;
            s->num_free_blocks++;
            if (c->size > s->largest_free_block)
                s->largest_free_block = c->size;
        } else {
            s->total_allocated += c->size;
        }
        c = c->next;
    }
}

void nebula_alloc_print_stats(void) {
    nebula_alloc_stats_t s;
    nebula_alloc_get_stats(&s);
    printf("  Pool:%zu  Alloc:%zu  Free:%zu  Largest:%zu  Blocks:%zu\n",
           s.total_pool_size, s.total_allocated, s.total_free,
           s.largest_free_block, s.num_free_blocks);
}
