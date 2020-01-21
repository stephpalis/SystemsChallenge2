#ifndef BIN_MALLOC_H
#define BIN_MALLOC_H

void* bin_malloc(size_t size);
void bin_free(void* item);
void* bin_realloc(void* prev, size_t bytes);

#endif
