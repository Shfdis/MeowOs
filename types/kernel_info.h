#pragma once
#include "types.h"

class PageOrchestrator;

#define PAGE_BITMAP_SIZE (2 * 1024 * 1024)

struct kernel_basic_info_t {
    char* mem_begin;
    char* mem_end;
    uint64_t total_pages;
    void* pages_bitmap;
    uint64_t pages_bitmap_size;
    uint64_t* pml4_table;
    uint16_t (*frame_buffer)[80];
    PageOrchestrator* page_orchestrator;
};

extern "C" {
    extern char _kernel_start[];
    extern char _kernel_end[];
    extern uint64_t pml4_table[512];
}

extern kernel_basic_info_t kernel_basic_info;
