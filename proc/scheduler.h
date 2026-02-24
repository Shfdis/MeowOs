#pragma once
#include "types/types.h"

class Process;

class Scheduler {
public:
    static constexpr size_t MAX_PROCESSES = 64;
    static constexpr uint64_t DEFAULT_QUANTUM = 1;

    void init();
    void add_process(Process& proc);
    void remove_process(Process& proc);
    /** Called from timer IRQ. Returns process to run next (may switch). */
    Process* tick();
    Process* get_current();
    uint64_t get_ticks() const;

    Process* processes[MAX_PROCESSES];
    size_t process_count;
    size_t current_index;
    uint64_t quantum;
    uint64_t tick_count;
};

extern Scheduler scheduler;
