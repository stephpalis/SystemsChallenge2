#include <stdlib.h>
#include <unistd.h>

#include "bin_malloc.h"
#include "xmalloc.h"


void*
xmalloc(size_t bytes)
{
    return bin_malloc(bytes);
}

void
xfree(void* ptr)
{
    bin_free(ptr);
}

void*
xrealloc(void* prev, size_t bytes)
{
    return bin_realloc(prev, bytes);
}

