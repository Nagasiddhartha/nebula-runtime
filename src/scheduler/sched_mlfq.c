#include "sched_internal.h"

nebula_thread_t* sched_pick_mlfq(nebula_thread_t *head)
{
    for (int q = 0; q < MLFQ_QUEUES; q++) {
        for (nebula_thread_t *c = head; c; c = c->next) {
            if (c->state == NEBULA_THREAD_READY && c->queue_level == q)
                return c;
        }
    }
    return NULL;
}

void sched_mlfq_on_yield(nebula_thread_t *self)
{
    if (!self) return;
    self->time_used++;
    int slice = 1 << self->queue_level;
    if (self->time_used >= slice && self->queue_level < MLFQ_QUEUES - 1) {
        self->queue_level++;
        self->time_used = 0;
    }
}

void sched_mlfq_boost_all(nebula_thread_t *head)
{
    for (nebula_thread_t *c = head; c; c = c->next) {
        if (c->state != NEBULA_THREAD_TERMINATED) {
            c->queue_level = 0;
            c->time_used = 0;
        }
    }
}
