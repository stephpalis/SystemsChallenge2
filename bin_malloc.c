#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>

#include "bin_malloc.h"
#include "hmalloc.h"

typedef struct bin_free_cell {
    struct bin_free_cell* next;
} bin_free_cell;

typedef struct bin_node {
    int size;
    bin_free_cell* bins;
} bin_node;

static const size_t bucket_base = 32;
#define BUCKET_NUM 8

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static bin_node buckets[BUCKET_NUM];
static const int64_t CHUNK_SIZE = 65536;

int
get_bucket_num(size_t bytes)
{
    size_t size = bucket_base;

    for (int ii = 0; ii < BUCKET_NUM; ++ii) {
        if (bytes <= size) {
            // return the first bin that has the proper number of bytes
            return ii;
        }
        size = size * 2;
    }

    // Can't find a bucket because > 4096
    return -1;
}    


void*
allocate_bucket(size_t size) {

    char* cur = mmap(0, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    *(size_t *)cur = size + sizeof(size_t);
    
    bin_free_cell* list = (bin_free_cell*)(cur + sizeof(size_t));
    list->next = NULL;

    char* end = cur + CHUNK_SIZE;

    while (1) {
        cur += size + sizeof(size_t);

        if ((cur + size + sizeof(size_t)) >= end) {
            break;
        }

        *(size_t*) cur = size + sizeof(size_t);

        bin_free_cell* cell = (bin_free_cell*) (cur + sizeof(size_t));

        cell->next = list;
        list = cell;
    }

    return list;
}


void
init_buckets() {
    size_t size = bucket_base;
    
    for (int ii = 0; ii < BUCKET_NUM; ++ii) {
        buckets[ii].size = size;
        buckets[ii].bins = allocate_bucket(size);

        size = size * 2;
    }
}


    void*
bin_malloc(size_t size)
{
    int offset = get_bucket_num(size);

    if (offset < 0) {
        return hmalloc(size);
    }
    
    pthread_mutex_lock(&mutex);

    if (buckets[0].size == 0) {
        // first time calling --> need to reallocate 
        init_buckets();
    }

    bin_node* node = &buckets[offset];

    if (node->bins == NULL) {
        node->bins = allocate_bucket(node->size);
    }

    void* value = node->bins;
    node->bins = node->bins->next;

    pthread_mutex_unlock(&mutex);

    return value;

}

void
bin_free(void* item) {
    size_t size = *(size_t*) ((char*) item - sizeof(size_t));
    size -= sizeof(size_t);

    int offset = get_bucket_num(size);

    if (offset < 0) {
        // Just unmap
        hfree(item);
        return;
    }   

    pthread_mutex_lock(&mutex);

    bin_node* node = &buckets[offset];

    bin_free_cell* cell = (bin_free_cell*) item;

    cell->next = node->bins;
    node->bins = cell;

    pthread_mutex_unlock(&mutex);
}

void*
bin_realloc(void* prev, size_t bytes)
{
    if (prev == NULL) {
        void* value = bin_malloc(bytes);
        return value;
    }
    char* block = ((char*) prev) - sizeof(size_t);
    size_t block_size = *(size_t *) block;
    size_t new_size = bytes + block_size;

    if (new_size <= block_size) {
        return prev;
    }

    void* dest = bin_malloc(new_size);

    memcpy(dest, prev, (block_size - sizeof(size_t)));
    hfree(prev);

    return dest;
}
