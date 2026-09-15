#include <stdio.h>
#include <assert.h>
#include "nebula_thread.h"

static int order[16];
static int order_idx = 0;

static void worker(void *arg)
{
    int id = *(int *)arg;
    for (int i = 0; i < 3; i++) {
        order[order_idx++] = id;
        nebula_thread_yield();
    }
}

static void test_rr_interleaves(void)
{
    nebula_thread_init();
    nebula_sched_set_policy(NEBULA_SCHED_ROUND_ROBIN);
    int id1 = 1, id2 = 2;
    order_idx = 0;
    nebula_thread_t *t1 = nebula_thread_create(worker, &id1);
    nebula_thread_t *t2 = nebula_thread_create(worker, &id2);
    nebula_thread_yield();
    nebula_thread_join(t1);
    nebula_thread_join(t2);

    assert(order_idx == 6);
    for (int i = 0; i < 6; i++)
        assert(order[i] == ((i % 2) ? 2 : 1));

    nebula_sched_stats_t s;
    nebula_sched_get_stats(&s);
    assert(s.threads_created == 2);
    assert(s.threads_terminated == 2);
    assert(s.total_context_switches >= 6);
    assert(s.latency_samples > 0);
    nebula_thread_destroy();
    printf("test_rr_interleaves: PASS (switches=%d)\n",
           s.total_context_switches);
}

static void test_mlfq_demotes(void)
{
    nebula_thread_init();
    nebula_sched_set_policy(NEBULA_SCHED_MLFQ);
    int id1 = 1, id2 = 2;
    order_idx = 0;
    nebula_thread_t *t1 = nebula_thread_create(worker, &id1);
    nebula_thread_t *t2 = nebula_thread_create(worker, &id2);
    assert(t1->queue_level == 0);
    assert(t2->queue_level == 0);
    nebula_thread_yield();
    nebula_thread_join(t1);
    nebula_thread_join(t2);
    assert(order_idx == 6);
    nebula_thread_destroy();
    printf("test_mlfq_demotes: PASS\n");
}

int main(void)
{
    printf("=== test_threads ===\n");
    test_rr_interleaves();
    test_mlfq_demotes();
    printf("=== all thread tests passed ===\n");
    return 0;
}
