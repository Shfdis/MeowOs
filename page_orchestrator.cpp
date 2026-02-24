#include "page_orchestrator.h"
#include "kernel_init.h"

PageOrchestrator::PageOrchestrator(void* bitmap_begin, uint64_t page_count) :
    bitmap_begin(reinterpret_cast<uint64_t*>(bitmap_begin)),
    bitmap_end(reinterpret_cast<uint64_t*>(bitmap_begin) + (page_count + 63) / 64),
    page_count(page_count) {}

bool PageOrchestrator::operator[](uint64_t page_number) {
    if (page_number >= page_count) {
        return false;
    }
    return bitmap_begin[page_number / 64] & (1ULL << (page_number % 64));
}

void* PageOrchestrator::get_page() {
    for (uint64_t i = 0; i < (page_count + 63) / 64; ++i) {
        if (bitmap_begin[i] != 0xFFFFFFFFFFFFFFFFULL) {
            for (uint64_t j = 0; j < 64; ++j) {
                if (!(*this)[i * 64 + j] && j + i * 64 < page_count) {
                    return reinterpret_cast<char*>(kernel_basic_info.mem_begin) + (i * 64 + j) * 4096;
                }
            }
        }
    }
    return 0;
}

void PageOrchestrator::set_page(void* page) {
    uint64_t page_number = (reinterpret_cast<uint64_t>(page) - reinterpret_cast<uint64_t>(kernel_basic_info.mem_begin)) / 4096;
    bitmap_begin[page_number / 64] |= 1ULL << (page_number % 64);
}

void PageOrchestrator::release_page(void* page) {
    uint64_t page_number = (reinterpret_cast<uint64_t>(page) - reinterpret_cast<uint64_t>(kernel_basic_info.mem_begin)) / 4096;
    if (page_number < page_count)
        bitmap_begin[page_number / 64] &= ~(1ULL << (page_number % 64));
}
