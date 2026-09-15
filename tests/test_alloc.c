#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "nebula_alloc.h"

static void test_basic_alloc_free(void)
{
    nebula_alloc_init(1 << 20);
    void *p = nebula_malloc(64);
    assert(p != NULL);
    memset(p, 0xAB, 64);
    nebula_free(p);
    nebula_alloc_destroy();
    printf("test_basic_alloc_free: PASS\n");
}

static void test_split_and_coalesce(void)
{
    nebula_alloc_init(1 << 20);
    void *a = nebula_malloc(1000);
    void *b = nebula_malloc(1000);
    void *c = nebula_malloc(1000);
    assert(a && b && c);
    nebula_free(a);
    nebula_free(c);
    nebula_free(b);

    nebula_alloc_stats_t s;
    nebula_alloc_get_stats(&s);
    assert(s.num_free_blocks == 1);
    assert(s.total_allocated == 0);
    nebula_alloc_destroy();
    printf("test_split_and_coalesce: PASS\n");
}

static void test_best_fit_picks_tightest(void)
{
    nebula_alloc_init(1 << 20);
    nebula_alloc_set_policy(NEBULA_BEST_FIT);
    void *small = nebula_malloc(200);
    (void)nebula_malloc(16);
    void *large = nebula_malloc(800);
    (void)nebula_malloc(16);
    void *mid   = nebula_malloc(400);
    (void)nebula_malloc(16);

    nebula_free(small);
    nebula_free(large);
    nebula_free(mid);

    void *got = nebula_malloc(350);
    assert(got != NULL);
    assert(got == mid);
    nebula_free(got);
    nebula_alloc_destroy();
    printf("test_best_fit_picks_tightest: PASS\n");
}

int main(void)
{
    printf("=== test_alloc ===\n");
    test_basic_alloc_free();
    test_split_and_coalesce();
    test_best_fit_picks_tightest();
    printf("=== all alloc tests passed ===\n");
    return 0;
}
