#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "allocator_internal.h"

#define NUM_SLAB_CLASSES 6
#define SLAB_CAPACITY 64

typedef struct slab_obj {
    struct slab_obj *next;
} slab_obj_t;

typedef struct slab_class {
    size_t obj_size;
    slab_obj_t *free_list;
    int total_objects;
    int free_count;
} slab_class_t;

typedef struct slab_region {
    char *start;
    size_t size;
    int class_idx;
    struct slab_region *next;
} slab_region_t;

static slab_class_t slabs[NUM_SLAB_CLASSES];
static slab_region_t *regions = NULL;
static void *slab_pool = NULL;
static size_t slab_pool_size = 0;
static size_t slab_pool_used = 0;

void slab_init(void *pool, size_t pool_size) {
    slab_pool = pool;
    slab_pool_size = pool_size;
    slab_pool_used = 0;

    slab_region_t *r = regions;
    while (r) { slab_region_t *n = r->next; free(r); r = n; }
    regions = NULL;

    size_t sizes[NUM_SLAB_CLASSES] = {32, 64, 128, 256, 512, 1024};
    for (int i = 0; i < NUM_SLAB_CLASSES; i++) {
        slabs[i].obj_size = sizes[i];
        slabs[i].free_list = NULL;
        slabs[i].total_objects = 0;
        slabs[i].free_count = 0;
    }
}

static void create_slab(slab_class_t *sc, int class_idx) {
    size_t slab_size = sc->obj_size * SLAB_CAPACITY;
    if (slab_pool_used + slab_size > slab_pool_size) return;

    char *region = (char *)slab_pool + slab_pool_used;
    slab_pool_used += slab_size;

    for (int i = 0; i < SLAB_CAPACITY; i++) {
        slab_obj_t *obj = (slab_obj_t *)(region + i * sc->obj_size);
        obj->next = sc->free_list;
        sc->free_list = obj;
    }
    sc->total_objects += SLAB_CAPACITY;
    sc->free_count += SLAB_CAPACITY;

    slab_region_t *r = (slab_region_t *)malloc(sizeof(slab_region_t));
    r->start = region;
    r->size = slab_size;
    r->class_idx = class_idx;
    r->next = regions;
    regions = r;
}

static int find_class(size_t size) {
    for (int i = 0; i < NUM_SLAB_CLASSES; i++) {
        if (size <= slabs[i].obj_size) return i;
    }
    return -1;
}

void *slab_allocate(size_t size) {
    if (!slab_pool || size == 0) return NULL;

    int cls = find_class(size);
    if (cls == -1) {
        size_t aligned = (size + 15) & ~(size_t)15;
        if (slab_pool_used + aligned > slab_pool_size) return NULL;
        void *ptr = (char *)slab_pool + slab_pool_used;
        slab_pool_used += aligned;
        return ptr;
    }

    slab_class_t *sc = &slabs[cls];
    if (!sc->free_list) {
        create_slab(sc, cls);
        if (!sc->free_list) return NULL;
    }

    slab_obj_t *obj = sc->free_list;
    sc->free_list = obj->next;
    sc->free_count--;
    return (void *)obj;
}

void slab_deallocate(void *ptr) {
    if (!ptr || !slab_pool) return;

    uintptr_t p = (uintptr_t)ptr;
    for (slab_region_t *r = regions; r; r = r->next) {
        uintptr_t start = (uintptr_t)r->start;
        uintptr_t end   = start + r->size;
        if (p >= start && p < end) {
            slab_class_t *sc = &slabs[r->class_idx];
            slab_obj_t *obj = (slab_obj_t *)ptr;
            obj->next = sc->free_list;
            sc->free_list = obj;
            sc->free_count++;
            return;
        }
    }
}
void slab_destroy(void) {
    slab_region_t *r = regions;
    while (r) { slab_region_t *n = r->next; free(r); r = n; }
    regions = NULL;
    slab_pool = NULL;
    slab_pool_size = 0;
    slab_pool_used = 0;
}
