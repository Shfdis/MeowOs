#include "heap.h"

namespace {

constexpr uint64_t MIN_BLOCK_SIZE = 32;
constexpr uint64_t ALIGN = 16;

BlockHeader* free_list_head = nullptr;
char* heap_start = nullptr;
char* heap_end = nullptr;

inline uint64_t align_up(uint64_t size, uint64_t align_val) {
    return (size + align_val - 1) & ~(align_val - 1);
}

inline BlockHeader* payload_to_header(void* ptr) {
    return reinterpret_cast<BlockHeader*>(reinterpret_cast<char*>(ptr) - sizeof(BlockHeader));
}

inline void* header_to_payload(BlockHeader* h) {
    return reinterpret_cast<void*>(reinterpret_cast<char*>(h) + sizeof(BlockHeader));
}

void remove_from_free_list(BlockHeader* block) {
    if (free_list_head == block) {
        free_list_head = block->next;
        return;
    }
    for (BlockHeader* p = free_list_head; p && p->next; p = p->next) {
        if (p->next == block) {
            p->next = block->next;
            return;
        }
    }
}

void insert_into_free_list(BlockHeader* block) {
    block->next = free_list_head;
    free_list_head = block;
}

BlockHeader* get_next_block(BlockHeader* block) {
    char* next_addr = reinterpret_cast<char*>(block) + block->size;
    if (next_addr >= heap_end)
        return nullptr;
    return reinterpret_cast<BlockHeader*>(next_addr);
}

BlockHeader* get_prev_block(BlockHeader* block) {
    if (reinterpret_cast<char*>(block) <= heap_start)
        return nullptr;
    BlockHeader* p = reinterpret_cast<BlockHeader*>(heap_start);
    while (reinterpret_cast<char*>(p) < reinterpret_cast<char*>(block)) {
        BlockHeader* next = get_next_block(p);
        if (!next)
            return nullptr;
        if (next == block)
            return p;
        p = next;
    }
    return nullptr;
}

} // namespace

void heap_init(void* start, void* end) {
    heap_start = reinterpret_cast<char*>(start);
    heap_end = reinterpret_cast<char*>(end);
    uint64_t region_size = static_cast<uint64_t>(heap_end - heap_start);
    if (region_size < sizeof(BlockHeader) + 16) {
        free_list_head = nullptr;
        return;
    }
    BlockHeader* initial = reinterpret_cast<BlockHeader*>(heap_start);
    initial->size = region_size;
    initial->next = nullptr;
    initial->is_free = true;
    free_list_head = initial;
}

void* kmalloc(uint64_t size) {
    if (size == 0 || !free_list_head)
        return nullptr;
    uint64_t needed = align_up(size, ALIGN) + sizeof(BlockHeader);
    if (needed < MIN_BLOCK_SIZE)
        needed = MIN_BLOCK_SIZE;

    BlockHeader* prev = nullptr;
    BlockHeader* curr = free_list_head;
    while (curr) {
        if (curr->is_free && curr->size >= needed) {
            if (curr->size >= needed + MIN_BLOCK_SIZE) {
                uint64_t rest_size = curr->size - needed;
                curr->size = needed;
                BlockHeader* rest = reinterpret_cast<BlockHeader*>(reinterpret_cast<char*>(curr) + needed);
                rest->size = rest_size;
                rest->next = curr->next;
                rest->is_free = true;
                if (prev)
                    prev->next = rest;
                else
                    free_list_head = rest;
            } else {
                remove_from_free_list(curr);
            }
            curr->is_free = false;
            curr->next = nullptr;
            return header_to_payload(curr);
        }
        prev = curr;
        curr = curr->next;
    }
    return nullptr;
}

void kfree(void* ptr) {
    if (!ptr || ptr < heap_start || ptr >= heap_end)
        return;
    BlockHeader* block = payload_to_header(ptr);
    if (block->is_free)
        return;
    block->is_free = true;

    BlockHeader* next = get_next_block(block);
    while (next && next->is_free) {
        block->size += next->size;
        remove_from_free_list(next);
        next = get_next_block(block);
    }

    BlockHeader* prev = get_prev_block(block);
    while (prev && prev->is_free) {
        remove_from_free_list(prev);
        prev->size += block->size;
        block = prev;
        prev = get_prev_block(block);
    }

    insert_into_free_list(block);
}

void* operator new(unsigned long size) {
    void* p = kmalloc(size);
    if (!p)
        return nullptr;
    return p;
}

void* operator new[](unsigned long size) {
    return operator new(size);
}

void operator delete(void* ptr) {
    kfree(ptr);
}

void operator delete(void* ptr, unsigned long) {
    kfree(ptr);
}

void operator delete[](void* ptr) {
    kfree(ptr);
}

void* operator new(unsigned long, void* p) {
    return p;
}
