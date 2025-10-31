#include <sys/mman.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include "myalloc.h"

node_t * _arena_start = NULL;
int size_arena;

int myinit(size_t size)
{
    // node_t * _arena_start;
    size_t page_size = getpagesize();
    
    size_t pages = (size + page_size - 1) / page_size;
    size_t arena_bytes = pages * (size_t)page_size;

    _arena_start = (node_t *) mmap(NULL, arena_bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

    if (_arena_start == MAP_FAILED) {
        _arena_start = NULL;
        return ERR_BAD_ARGUMENTS;
    }
    _arena_start->size = arena_bytes - sizeof(node_t);
    _arena_start->fwd = NULL;
    _arena_start->bwd = NULL;
    _arena_start->is_free = 1;

    return arena_bytes;
}

int mydestroy() {
    if (NULL == _arena_start) return ERR_UNINITIALIZED;
    int success = munmap(_arena_start, _arena_start->size + sizeof(node_t));

    return success;
}
    