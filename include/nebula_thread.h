#ifndef NEBULA_THREAD_H
#define NEBULA_THREAD_H

#include <stddef.h>
#include <stdint.h>
#include <ucontext.h>

typedef enum {
    NEBULA_THREAD_READY,
    NEBULA_THREAD_RUNNING,
    NEBULA_THREAD_BLOCKED,
    NEBULA_THREAD_TERMINATED
} nebula_thread_state_t;

typedef enum {
    NEBULA_SCHED_ROUND_ROBIN,
    NEBULA_SCHED_PRIORITY,
    NEBULA_SCHED_MLFQ
} nebula_sched_policy_t;

typedef struct nebula_thread {
    int id;
    nebula_thread_state_t state;
    ucontext_t context;
    void *stack;
    size_t stack_size;
    int priority;
    int time_used;
    struct nebula_thread *join_target;
    struct nebula_thread *next;
} nebula_thread_t;

typedef struct {
    int total_context_switches;
    int threads_created;
    int threads_terminated;
} nebula_sched_stats_t;

void nebula_thread_init(void);
void nebula_thread_destroy(void);
nebula_thread_t* nebula_thread_create(void (*func)(void*), void *arg);
void nebula_thread_yield(void);
void nebula_thread_exit(void);
void nebula_thread_join(nebula_thread_t *thread);
void nebula_sched_set_policy(nebula_sched_policy_t policy);
void nebula_sched_get_stats(nebula_sched_stats_t *stats);
void nebula_sched_print_stats(void);

#endif
