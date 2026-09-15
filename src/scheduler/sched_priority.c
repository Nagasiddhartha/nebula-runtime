#include "sched_internal.h"

nebula_thread_t* sched_pick_priority(nebula_thread_t *head)
{
    nebula_thread_t *best = NULL;
    int best_prio = -1;
    for (nebula_thread_t *c = head; c; c = c->next) {
        if (c->state == NEBULA_THREAD_READY && c->priority > best_prio) {
            best = c;
            best_prio = c->priority;
        }
    }
    return best;
}
