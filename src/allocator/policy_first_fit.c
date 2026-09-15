#include "allocator_internal.h"

block_header_t* policy_first_fit_find(size_t size) {
    block_header_t *cur = nebula_alloc_get_head();
    while (cur) {
        if (cur->is_free && cur->size >= size)
            return cur;
        cur = cur->next;
    }
    return NULL;
}
