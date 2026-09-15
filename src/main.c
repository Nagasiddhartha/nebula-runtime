#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "nebula_alloc.h"
#include "nebula_thread.h"
#include "sched_internal.h"
void worker_a(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 4; i++) {
        printf("  [Thread %d] Working... step %d\n", id, i);
        nebula_thread_yield();
    }
    printf("  [Thread %d] Done!\n", id);
}

void worker_b(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 3; i++) {
        printf("  [Thread %d] Computing... step %d\n", id, i);
        nebula_thread_yield();
    }
    printf("  [Thread %d] Done!\n", id);
}

int main(void) {
    printf("=== NEBULA RUNTIME ===\n\n");

    /* PART 1: FIRST-FIT */
    printf("--- PART 1: FIRST-FIT ---\n");
    nebula_alloc_init(4 * 1024 * 1024);
    void *a1 = nebula_malloc(1000);
    void *a2 = nebula_malloc(500);
    void *a3 = nebula_malloc(2000);
    printf("Allocated a1=%p a2=%p a3=%p\n", a1, a2, a3);
    nebula_alloc_print_stats();
    nebula_free(a2);
    nebula_free(a1);
    printf("After coalescing:\n");
    nebula_alloc_print_stats();
    nebula_free(a3);
    nebula_alloc_destroy();

    /* PART 2: BEST-FIT */
    printf("\n--- PART 2: BEST-FIT ---\n");
    nebula_alloc_init(4 * 1024 * 1024);
    nebula_alloc_set_policy(NEBULA_BEST_FIT);
    void *b1 = nebula_malloc(200);
    void *s1 = nebula_malloc(16);
    void *b2 = nebula_malloc(800);
    void *s2 = nebula_malloc(16);
    void *b3 = nebula_malloc(400);
    void *s3 = nebula_malloc(16);
    nebula_free(b1);
    nebula_free(b2);
    nebula_free(b3);
    printf("3 separate holes (~200, ~800, ~400):\n");
    nebula_alloc_print_stats();
    printf("BEST-FIT malloc(350):\n");
    void *b5 = nebula_malloc(350);
    printf("  got %p\n", b5);
    nebula_free(b5);
    nebula_free(s1);
    nebula_free(s2);
    nebula_free(s3);
    nebula_alloc_destroy();

    /* PART 3: BUDDY */
    printf("\n--- PART 3: BUDDY ---\n");
    nebula_alloc_init(4 * 1024 * 1024);
    nebula_alloc_set_policy(NEBULA_BUDDY);
    void *c1 = nebula_malloc(100);
    void *c2 = nebula_malloc(300);
    void *c3 = nebula_malloc(1000);
    printf("Buddy allocated: c1=%p c2=%p c3=%p\n", c1, c2, c3);
    printf("Freeing c2, then c1 (buddies should merge)\n");
    nebula_free(c2);
    nebula_free(c1);
    printf("Buddy allocator test complete\n");
    nebula_free(c3);
    nebula_alloc_destroy();

    /* PART 4: SLAB */
    printf("\n--- PART 4: SLAB ---\n");
    nebula_alloc_init(4 * 1024 * 1024);
    nebula_alloc_set_policy(NEBULA_SLAB);
    void *d1 = nebula_malloc(32);
    void *d2 = nebula_malloc(64);
    void *d3 = nebula_malloc(128);
    void *d4 = nebula_malloc(256);
    printf("Slab allocated: d1=%p d2=%p d3=%p d4=%p\n", d1, d2, d3, d4);
    printf("Freeing d2 (returns to 64-byte slab cache)\n");
    nebula_free(d2);
    void *d5 = nebula_malloc(64);
    printf("Re-alloc 64 bytes: d5=%p (should reuse d2's slot)\n", d5);
    nebula_free(d1);
    nebula_free(d3);
    nebula_free(d4);
    nebula_free(d5);
    nebula_alloc_destroy();

    /* PART 5: ROUND ROBIN */
    printf("\n--- PART 5: ROUND ROBIN ---\n");
    nebula_thread_init();
    nebula_sched_set_policy(NEBULA_SCHED_ROUND_ROBIN);
    int id1 = 1, id2 = 2;
    nebula_thread_t *t1 = nebula_thread_create(worker_a, &id1);
    nebula_thread_t *t2 = nebula_thread_create(worker_b, &id2);
    nebula_thread_yield();
    nebula_thread_join(t1);
    nebula_thread_join(t2);
    nebula_sched_print_stats();
    nebula_thread_destroy();

    /* PART 6: PRIORITY */
    printf("\n--- PART 6: PRIORITY ---\n");
    nebula_thread_init();
    nebula_sched_set_policy(NEBULA_SCHED_PRIORITY);
    int id3 = 3, id4 = 4;
    nebula_thread_t *t3 = nebula_thread_create(worker_a, &id3);
    nebula_thread_t *t4 = nebula_thread_create(worker_b, &id4);
    t3->priority = 10;
    t4->priority = 1;
    printf("Thread 3 prio=10, Thread 4 prio=1\n");
    nebula_thread_yield();
    nebula_thread_join(t3);
    nebula_thread_join(t4);
    nebula_sched_print_stats();
    nebula_thread_destroy();

    /* PART 7: MLFQ */
    printf("\n--- PART 7: MLFQ ---\n");
    nebula_thread_init();
    nebula_sched_set_policy(NEBULA_SCHED_MLFQ);
    int id5 = 5, id6 = 6;
    nebula_thread_t *t5 = nebula_thread_create(worker_a, &id5);
    nebula_thread_t *t6 = nebula_thread_create(worker_b, &id6);
    printf("Both threads start at MLFQ queue 0 (highest)\n");
    printf("Threads demote after exhausting slice; boost every %d switches\n",
           MLFQ_BOOST_INTERVAL);
    nebula_thread_yield();
    nebula_thread_join(t5);
    nebula_thread_join(t6);
    nebula_sched_print_stats();
    nebula_thread_destroy();

    printf("\n=== NEBULA RUNTIME COMPLETE ===\n");
    return 0;
}
