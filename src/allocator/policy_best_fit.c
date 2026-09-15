#include "allocator_internal.h"
#include <stdint.h>

block_header_t* policy_best_fit_find(size_t size) {
    block_header_t *cur = nebula_alloc_get_head();
    block_header_t *best = NULL;
    size_t best_sz = SIZE_MAX;
    while (cur) {
        if (cur->is_free && cur->size >= size && cur->size < best_sz) {
            best = cur;
            best_sz = cur->size;
        }
        cur = cur->next;
    }
    return best;
}
