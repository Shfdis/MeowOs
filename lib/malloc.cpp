#include "malloc.h"
#include "syscall.h"
#include <cstddef>

namespace {

constexpr uint64_t PAGE_SIZE = 4096;
constexpr uint64_t HEADER_SIZE = 8;
constexpr uint64_t MIN_BLOCK = 16;
constexpr uint64_t ALIGN = 8;

struct Block {
    uint64_t size;
    Block* next;
};

static Block* free_head = nullptr;

static uint64_t align_up(uint64_t n, uint64_t a) {
    return (n + a - 1) & ~(a - 1);
}

}

void* malloc(uint64_t size) {
    if (size == 0)
        return nullptr;
    uint64_t need = align_up(size, ALIGN) + HEADER_SIZE;
    if (need < MIN_BLOCK)
        need = MIN_BLOCK;

    for (;;) {
        Block* prev = nullptr;
        Block* curr = free_head;
        while (curr) {
            if (curr->size >= need) {
                if (curr->size - need >= MIN_BLOCK) {
                    Block* rest = reinterpret_cast<Block*>(reinterpret_cast<char*>(curr) + need);
                    rest->size = curr->size - need;
                    rest->next = curr->next;
                    curr->size = need;
                    if (prev)
                        prev->next = rest;
                    else
                        free_head = rest;
                } else {
                    if (prev)
                        prev->next = curr->next;
                    else
                        free_head = curr->next;
                }
                return reinterpret_cast<char*>(curr) + HEADER_SIZE;
            }
            prev = curr;
            curr = curr->next;
        }

        uint64_t n_pages = (need + PAGE_SIZE - 1) / PAGE_SIZE;
        int64_t new_end = sys_feed(static_cast<int64_t>(n_pages));
        if (new_end < 0)
            return nullptr;
        uint64_t region_start = static_cast<uint64_t>(new_end) - n_pages * PAGE_SIZE;
        Block* new_block = reinterpret_cast<Block*>(region_start);
        new_block->size = n_pages * PAGE_SIZE;
        new_block->next = free_head;
        free_head = new_block;
    }
}

void free(void* ptr) {
    if (!ptr)
        return;
    Block* block = reinterpret_cast<Block*>(static_cast<char*>(ptr) - HEADER_SIZE);
    block->next = free_head;
    free_head = block;
}

void* operator new(std::size_t size) {
    void* p = ::malloc(static_cast<uint64_t>(size));
    if (!p) {
        for (;;) {}
    }
    return p;
}

void* operator new[](std::size_t size) {
    return operator new(size);
}

void operator delete(void* ptr) noexcept {
    ::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    ::free(ptr);
}

void operator delete[](void* ptr) noexcept {
    ::free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    ::free(ptr);
}
