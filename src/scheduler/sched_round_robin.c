#include "sched_internal.h"

nebula_thread_t* sched_pick_round_robin(nebula_thread_t *head,
                                        nebula_thread_t *current)
{
    if (!head) return NULL;
    nebula_thread_t *start = (current && current->next) ? current->next : head;
    nebula_thread_t *c = start;
    do {
        if (c->state == NEBULA_THREAD_READY) return c;
        c = c->next ? c->next : head;
    } while (c != start);
    return NULL;
}
