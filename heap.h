#pragma once
#include "types/types.h"

struct BlockHeader {
    uint64_t size;
    BlockHeader* next;
    bool is_free;
};

void heap_init(void* start, void* end);
void* kmalloc(uint64_t size);
void kfree(void* ptr);
