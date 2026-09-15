# Nebula Runtime

A user-space runtime in C implementing a **custom memory allocator** and a
**green-thread scheduler** on top of `mmap` and `ucontext`, without relying
on `malloc`/`pthread` for core mechanisms.

## Allocator

Block headers with 16-byte alignment, splitting, and coalescing.
Four policies behind one `nebula_malloc` / `nebula_free` interface:

| Policy | Strategy |
|--------|----------|
| First-Fit | Walk free list, take first block that fits |
| Best-Fit | Walk free list, take smallest block that fits |
| Buddy | Power-of-2 blocks, split on alloc, merge buddies on free |
| Slab | Pre-allocated fixed-size object caches (32–1024 B) |

### Benchmark vs glibc malloc

| Workload | Nebula | glibc | Notes |
|----------|--------|-------|-------|
| 50K × 64 B allocs | 3162 ms | 1.3 ms | O(n) list scan vs O(1) bins |
| 5K × mixed allocs | 80 ms | 1.7 ms | Same tradeoff |
| 100K alloc+free churn | **0.80 ms** | 0.41 ms | Competitive via coalescing |
| Long-lived + short-lived | 1.39 ms | 0.13 ms | glibc size-class caching |

> glibc is faster on bulk workloads due to binned free lists and
> per-thread arenas. Nebula demonstrates how allocation policy
> affects fragmentation and latency tradeoffs.

## Thread Scheduler

Cooperative green threads using `ucontext` (`makecontext`/`swapcontext`).

- **TCB** with four states: `READY`, `RUNNING`, `BLOCKED`, `TERMINATED`
- **API**: `create` / `yield` / `exit` / `join`
- **Policies**: Round Robin, Priority, MLFQ (multi-level feedback queue)
- **Metrics**: context-switch count, avg/min/max latency (nanoseconds)

## Build & Run

```bash
make            # build
./build/nebula  # run all 7 tests
make valgrind   # memory leak check
make asan       # AddressSanitizer check
make bench      # benchmark vs glibc
