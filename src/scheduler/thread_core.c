#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ucontext.h>
#include "nebula_thread.h"

#define DEFAULT_STACK_SIZE (64 * 1024)
#define MAX_THREADS 128

static nebula_thread_t *thread_list = NULL;
static nebula_thread_t *current_thread = NULL;
static nebula_thread_t main_thread;
static int next_id = 1;
static int thread_count = 0;
static nebula_sched_policy_t current_policy = NEBULA_SCHED_ROUND_ROBIN;
static nebula_sched_stats_t sched_stats = {0};

static void scheduler(void);
static nebula_thread_t* pick_next_rr(void);
static nebula_thread_t* pick_next_priority(void);
static void thread_entry(int lo, int hi);

void nebula_thread_init(void) {
    memset(&main_thread, 0, sizeof(nebula_thread_t));
    main_thread.id = 0;
    main_thread.state = NEBULA_THREAD_RUNNING;
    current_thread = &main_thread;
    thread_list = &main_thread;
    thread_count = 1;
    next_id = 1;
    memset(&sched_stats, 0, sizeof(sched_stats));
    printf("[Nebula Thread] Runtime initialized\n");
}

void nebula_thread_destroy(void) {
    nebula_thread_t *cur = thread_list->next;
    while (cur) {
        nebula_thread_t *nxt = cur->next;
        if (cur->stack) free(cur->stack);
        free(cur);
        cur = nxt;
    }
    thread_list->next = NULL;
    thread_count = 1;
    printf("[Nebula Thread] Runtime destroyed\n");
}

static void add_thread(nebula_thread_t *t) {
    nebula_thread_t *tmp = thread_list;
    while (tmp->next) tmp = tmp->next;
    tmp->next = t;
}

nebula_thread_t* nebula_thread_create(void (*func)(void*), void *arg) {
    if (thread_count >= MAX_THREADS) return NULL;
    nebula_thread_t *t = (nebula_thread_t *)malloc(sizeof(nebula_thread_t));
    if (!t) return NULL;
    memset(t, 0, sizeof(nebula_thread_t));
    t->id = next_id++;
    t->state = NEBULA_THREAD_READY;
    t->stack_size = DEFAULT_STACK_SIZE;
    t->stack = malloc(t->stack_size);
    t->priority = 0;
    t->next = NULL;
    if (!t->stack) { free(t); return NULL; }

    void **sp = (void **)t->stack;
    sp[0] = (void *)func;
    sp[1] = arg;

    getcontext(&t->context);
    t->context.uc_stack.ss_sp = (char *)t->stack + 2 * sizeof(void*);
    t->context.uc_stack.ss_size = t->stack_size - 2 * sizeof(void*);
    t->context.uc_link = NULL;

    uintptr_t ptr = (uintptr_t)t;
    int lo = (int)(ptr & 0xFFFFFFFF);
    int hi = (int)(ptr >> 32);
    makecontext(&t->context, (void (*)(void))thread_entry, 2, lo, hi);

    add_thread(t);
    thread_count++;
    sched_stats.threads_created++;
    printf("[Nebula Thread] Created thread ID %d\n", t->id);
    return t;
}

static void thread_entry(int lo, int hi) {
    uintptr_t ptr = ((uintptr_t)(unsigned int)hi << 32) | (unsigned int)lo;
    nebula_thread_t *self = (nebula_thread_t *)ptr;
    void **sp = (void **)self->stack;
    void (*func)(void*) = (void (*)(void*))sp[0];
    void *arg = sp[1];
    func(arg);
    nebula_thread_exit();
}

static nebula_thread_t* pick_next_rr(void) {
    nebula_thread_t *start = current_thread->next;
    if (!start) start = thread_list;
    nebula_thread_t *c = start;
    do {
        if (c->state == NEBULA_THREAD_READY) return c;
        c = c->next;
        if (!c) c = thread_list;
    } while (c != start);
    return NULL;
}

static nebula_thread_t* pick_next_priority(void) {
    nebula_thread_t *best = NULL;
    int best_prio = -1;
    nebula_thread_t *c = thread_list;
    while (c) {
        if (c->state == NEBULA_THREAD_READY && c->priority > best_prio) {
            best = c;
            best_prio = c->priority;
        }
        c = c->next;
    }
    return best;
}

static void scheduler(void) {
    nebula_thread_t *next = NULL;
    if (current_policy == NEBULA_SCHED_ROUND_ROBIN)
        next = pick_next_rr();
    else if (current_policy == NEBULA_SCHED_PRIORITY)
        next = pick_next_priority();
    else
        next = pick_next_rr();

    if (!next) {
        if (current_thread != &main_thread &&
            main_thread.state == NEBULA_THREAD_READY)
            next = &main_thread;
        else return;
    }
    if (next == current_thread) {
        current_thread->state = NEBULA_THREAD_RUNNING;
        return;
    }
    nebula_thread_t *prev = current_thread;
    if (prev->state == NEBULA_THREAD_RUNNING)
        prev->state = NEBULA_THREAD_READY;
    next->state = NEBULA_THREAD_RUNNING;
    current_thread = next;
    sched_stats.total_context_switches++;
    swapcontext(&prev->context, &next->context);
}

void nebula_thread_yield(void) {
    if (!current_thread) return;
    current_thread->state = NEBULA_THREAD_READY;
    scheduler();
}

void nebula_thread_exit(void) {
    if (!current_thread) return;
    current_thread->state = NEBULA_THREAD_TERMINATED;
    sched_stats.threads_terminated++;
    printf("[Nebula Thread] Thread ID %d terminated\n", current_thread->id);
    nebula_thread_t *c = thread_list;
    while (c) {
        if (c->state == NEBULA_THREAD_BLOCKED &&
            c->join_target == current_thread) {
            c->state = NEBULA_THREAD_READY;
            c->join_target = NULL;
        }
        c = c->next;
    }
    scheduler();
}

void nebula_thread_join(nebula_thread_t *target) {
    if (!target || target->state == NEBULA_THREAD_TERMINATED) return;
    current_thread->state = NEBULA_THREAD_BLOCKED;
    current_thread->join_target = target;
    scheduler();
}

void nebula_sched_set_policy(nebula_sched_policy_t p) {
    current_policy = p;
    const char *n[] = {"Round Robin", "Priority", "MLFQ"};
    printf("[Nebula Thread] Scheduler: %s\n", n[p]);
}

void nebula_sched_get_stats(nebula_sched_stats_t *s) {
    if (s) *s = sched_stats;
}

void nebula_sched_print_stats(void) {
    printf("  Switches:%d  Created:%d  Terminated:%d\n",
           sched_stats.total_context_switches,
           sched_stats.threads_created,
           sched_stats.threads_terminated);
}
