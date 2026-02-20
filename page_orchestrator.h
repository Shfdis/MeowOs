#pragma once
#include "types/types.h"

class PageOrchestrator {
private:
    uint64_t* bitmap_begin;
    uint64_t* bitmap_end;
    uint64_t page_count;
public:
    PageOrchestrator(void* bitmap_begin, uint64_t page_count);
    bool operator[](uint64_t page_number);
    void set_page(void* page);
    void* get_page();
};
