// CS3650 CH02 starter code
// Spring 2019
//
// Author: Nat Tuck
//
// Once you've read this, you're done with the simple allocator homework.

#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "hmalloc.h"

typedef struct nu_free_cell {
    int64_t              size;
    struct nu_free_cell* next;
} nu_free_cell;

static const int64_t CHUNK_SIZE = 65536;
static const int64_t CELL_SIZE  = (int64_t)sizeof(nu_free_cell);
static nu_free_cell* nu_free_list = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int64_t
nu_free_list_length()
{
    int len = 0;

    pthread_mutex_lock(&mutex);
    for (nu_free_cell* pp = nu_free_list; pp != 0; pp = pp->next) {
        len++;
    }
    pthread_mutex_unlock(&mutex);

    return len;
}

static
void
nu_free_list_coalesce()
{
    
    pthread_mutex_lock(&mutex);
    nu_free_cell* pp = nu_free_list;
    int free_chunk = 0;

    while (pp != 0 && pp->next != 0) {
        if (((int64_t)pp) + pp->size == ((int64_t) pp->next)) {
            pp->size += pp->next->size;
            pp->next  = pp->next->next;
        }

        pp = pp->next;
    }
    pthread_mutex_unlock(&mutex);
}

static
void
nu_free_list_insert(nu_free_cell* cell)
{
    pthread_mutex_lock(&mutex);
    if (nu_free_list == 0 || ((uint64_t) nu_free_list) > ((uint64_t) cell)) {
        cell->next = nu_free_list;
        nu_free_list = cell;
        pthread_mutex_unlock(&mutex);
        return;
    }

    nu_free_cell* pp = nu_free_list;

    while (pp->next != 0 && ((uint64_t)pp->next) < ((uint64_t) cell)) {
        pp = pp->next;
    }
    
    cell->next = pp->next;
    pp->next = cell;
    
    pthread_mutex_unlock(&mutex);
    nu_free_list_coalesce();
}

static
nu_free_cell*
free_list_get_cell(int64_t size)
{
    pthread_mutex_lock(&mutex);
    nu_free_cell** prev = &nu_free_list;

    for (nu_free_cell* pp = nu_free_list; pp != 0; pp = pp->next) {
        if (pp->size >= size) {
            *prev = pp->next;
            pthread_mutex_unlock(&mutex);
            return pp;
        }
        prev = &(pp->next);
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}

static
nu_free_cell*
make_cell()
{
    void* addr = mmap(0, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    nu_free_cell* cell = (nu_free_cell*) addr; 
    cell->size = CHUNK_SIZE;
    return cell;
}

void*
hmalloc(size_t usize)
{
    int64_t size = (int64_t) usize;

    // space for size
    int64_t alloc_size = size + sizeof(int64_t);

    // space for free cell when returned to list
    if (alloc_size < CELL_SIZE) {
        alloc_size = CELL_SIZE;
    }

    if (alloc_size > CHUNK_SIZE) {
        void* addr = mmap(0, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        *((int64_t*)addr) = alloc_size;
        return addr + sizeof(int64_t);
    }

    nu_free_cell* cell = free_list_get_cell(alloc_size);
    if (!cell) {
        cell = make_cell();
    }

    // Return unused portion to free list.
    int64_t rest_size = cell->size - alloc_size;
    if (rest_size >= CELL_SIZE) {
        void* addr = (void*) cell;
        nu_free_cell* rest = (nu_free_cell*) (addr + alloc_size);
        rest->size = rest_size;
        nu_free_list_insert(rest);
    }

    *((int64_t*)cell) = alloc_size;
    return ((void*)cell) + sizeof(int64_t);
}

void
hfree(void* addr) 
{
    nu_free_cell* cell = (nu_free_cell*)(addr - sizeof(int64_t));
    // cast to nufreecrll
    int64_t size = *((int64_t*) cell);

    if (size > CHUNK_SIZE) {
        munmap((void*) cell, size);
    }
    else {
        cell->size = size;
        nu_free_list_insert(cell);
    }
}

void*
hrealloc(void* prev, size_t bytes)
{
    if (prev == NULL) {
        void* value = hmalloc(bytes);
        return value;
    }
    char* block = ((char*) prev) - sizeof(size_t);
    size_t block_size = *(size_t *) block;
    size_t new_size = bytes + block_size;

    void* dest = hmalloc(new_size);

    memcpy(dest, prev, (block_size - sizeof(size_t)));
    hfree(prev);

    return dest;
}
