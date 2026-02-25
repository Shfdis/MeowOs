#pragma once
#include "types/types.h"
#include "types/cpu.h"
#include "page_orchestrator.h"

enum class ProcessState { Runnable, Blocked };

struct MemoryRegion {
    uint64_t base;
    uint64_t size;
    uint8_t permissions;
};

class MemoryMapping {
public:
    static constexpr size_t MAX_REGIONS = 64;
    MemoryRegion regions[MAX_REGIONS];
    size_t count = 0;

    void add_region(uint64_t base, uint64_t size, uint8_t permissions);
};

class Process {
public:
    Process();
    ~Process();

    void map_kernel_memory();
    uint64_t get_cr3() const;

    void switch_to();
    static void switch_to(Process& from, Process& to);

    uint64_t brk(uint64_t addr);
    uint64_t sbrk(int64_t increment);

    int64_t sbrk_pages(int64_t n_pages);
    bool load_binary(const void* data, uint32_t size, uint64_t entry_point);

    cpu_context_t context;
    ProcessState state;
    Process* wait_queue_next;
    uint64_t* pml4;
    uint64_t mapped_pages;
    uint64_t heap_start;
    uint64_t heap_end;
    uint64_t stack_top;
    uint64_t stack_size;
    uint64_t pid;
    MemoryMapping memory_mapping;

private:
    static uint64_t next_pid;
};
