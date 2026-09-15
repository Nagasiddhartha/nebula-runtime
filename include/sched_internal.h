#ifndef SCHED_INTERNAL_H
#define SCHED_INTERNAL_H

#include "nebula_thread.h"

nebula_thread_t* sched_pick_round_robin(nebula_thread_t *head,
                                        nebula_thread_t *current);
nebula_thread_t* sched_pick_priority(nebula_thread_t *head);
nebula_thread_t* sched_pick_mlfq(nebula_thread_t *head);

#define MLFQ_QUEUES 4
#define MLFQ_BOOST_INTERVAL 20

void sched_mlfq_on_yield(nebula_thread_t *self);
void sched_mlfq_boost_all(nebula_thread_t *head);

#endif
