#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "nebula_alloc.h"

#define POOL_16  (16 * 1024 * 1024)
#define POOL_64  (64 * 1024 * 1024)

static double ms_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static void bench_small(void) {
    int N = 50000;
    printf("\n[Workload 1] %d x 64-byte allocs\n", N);
    void **p = malloc(sizeof(void*) * N);

    nebula_alloc_init(POOL_16);
    double t0 = ms_now();
    for (int i = 0; i < N; i++) p[i] = nebula_malloc(64);
    double t1 = ms_now();
    for (int i = 0; i < N; i++) nebula_free(p[i]);
    double t2 = ms_now();
    printf("  Nebula : malloc %8.3f ms | free %8.3f ms | total %8.3f ms\n",
           t1-t0, t2-t1, t2-t0);
    nebula_alloc_destroy();

    t0 = ms_now();
    for (int i = 0; i < N; i++) p[i] = malloc(64);
    t1 = ms_now();
    for (int i = 0; i < N; i++) free(p[i]);
    t2 = ms_now();
    printf("  glibc  : malloc %8.3f ms | free %8.3f ms | total %8.3f ms\n",
           t1-t0, t2-t1, t2-t0);
    free(p);
}

static void bench_mixed(void) {
    int N = 5000;
    printf("\n[Workload 2] %d x mixed-size allocs (16-4096)\n", N);
    void **p = malloc(sizeof(void*) * N);
    srand(42);

    nebula_alloc_init(POOL_64);
    double t0 = ms_now();
    for (int i = 0; i < N; i++)
        p[i] = nebula_malloc(16 + (rand() % 4080));
    double t1 = ms_now();
    for (int i = 0; i < N; i++) nebula_free(p[i]);
    double t2 = ms_now();
    printf("  Nebula : malloc %8.3f ms | free %8.3f ms | total %8.3f ms\n",
           t1-t0, t2-t1, t2-t0);
    nebula_alloc_destroy();

    srand(42);
    t0 = ms_now();
    for (int i = 0; i < N; i++)
        p[i] = malloc(16 + (rand() % 4080));
    t1 = ms_now();
    for (int i = 0; i < N; i++) free(p[i]);
    t2 = ms_now();
    printf("  glibc  : malloc %8.3f ms | free %8.3f ms | total %8.3f ms\n",
           t1-t0, t2-t1, t2-t0);
    free(p);
}

static void bench_churn(void) {
    int N = 100000;
    printf("\n[Workload 3] %d x alloc+free churn (128 bytes)\n", N);

    nebula_alloc_init(POOL_16);
    double t0 = ms_now();
    for (int i = 0; i < N; i++) {
        void *p = nebula_malloc(128);
        nebula_free(p);
    }
    double t1 = ms_now();
    printf("  Nebula : total %8.3f ms\n", t1-t0);
    nebula_alloc_destroy();

    t0 = ms_now();
    for (int i = 0; i < N; i++) {
        void *p = malloc(128);
        free(p);
    }
    t1 = ms_now();
    printf("  glibc  : total %8.3f ms\n", t1-t0);
}

static void bench_longshort(void) {
    printf("\n[Workload 4] long-lived + short-lived mix\n");
    int long_count = 100;
    int short_iter = 10000;

    nebula_alloc_init(POOL_16);
    void **p = malloc(sizeof(void*) * long_count);
    double t0 = ms_now();
    for (int i = 0; i < long_count; i++)
        p[i] = nebula_malloc(256);
    for (int i = 0; i < short_iter; i++) {
        void *tmp = nebula_malloc(64);
        nebula_free(tmp);
    }
    for (int i = 0; i < long_count; i++)
        nebula_free(p[i]);
    double t1 = ms_now();
    printf("  Nebula : total %8.3f ms\n", t1-t0);
    nebula_alloc_destroy();
    free(p);

    p = malloc(sizeof(void*) * long_count);
    t0 = ms_now();
    for (int i = 0; i < long_count; i++)
        p[i] = malloc(256);
    for (int i = 0; i < short_iter; i++) {
        void *tmp = malloc(64);
        free(tmp);
    }
    for (int i = 0; i < long_count; i++)
        free(p[i]);
    t1 = ms_now();
    printf("  glibc  : total %8.3f ms\n", t1-t0);
    free(p);
}

int main(void) {
    printf("=== NEBULA vs GLIBC MALLOC BENCHMARK ===\n");
    printf("(Nebula = educational allocator, glibc = production allocator)\n");
    bench_small();
    bench_mixed();
    bench_churn();
    bench_longshort();
    printf("\n=== BENCHMARK COMPLETE ===\n");
    return 0;
}
