#include <sys/mman.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include "myalloc.h"

node_t * _arena_start = NULL;
int size_arena;
int statusno = 0;

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

//size_t is never negative!
void* myalloc(size_t size) {
    if (_arena_start == NULL) {
        statusno = ERR_UNINITIALIZED;
        return NULL;
    }

    node_t *current = _arena_start;
    while (current && (!current->is_free || current->size < size)) {
        current = current->fwd;
    }

    if (!current) {
        statusno = ERR_OUT_OF_MEMORY;
        return NULL;
    }

    // Split only if space for a new node header remains
    if (current->size >= size + sizeof(node_t) + 1) {
        node_t *new_block = (node_t*)((char*)current + sizeof(node_t) + size);
        new_block->size = current->size - size - sizeof(node_t);
        new_block->is_free = 1;
        new_block->fwd = current->fwd;
        new_block->bwd = current;
        if (current->fwd)
            current->fwd->bwd = new_block;
        current->fwd = new_block;
        current->size = size;
    }

    current->is_free = 0;
    statusno = 0;

    return (char*)current + sizeof(node_t);
}


void myfree(void *ptr) {
    if (ptr == NULL) return;

    node_t* block = (node_t*)((char*)ptr - sizeof(node_t));
    block->is_free = 1;

    if (block->fwd && block->fwd->is_free) {
        node_t* next = block->fwd;
        block->size += sizeof(node_t) + next->size;
        block->fwd = next->fwd;
        if (next->fwd)
            next->fwd->bwd = block;
    }

    if (block->bwd && block->bwd->is_free) {
        node_t* prev = block->bwd;
        prev->size += sizeof(node_t) + block->size;
        prev->fwd = block->fwd;
        if (block->fwd)
            block->fwd->bwd = prev;
        block = prev;  
    }
}

int mydestroy() {
    if (NULL == _arena_start) return ERR_UNINITIALIZED;
    int success = munmap(_arena_start, _arena_start->size + sizeof(node_t));

    return success;
}
    