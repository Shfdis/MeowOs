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
    Process* tick();
    Process* pick_next_runnable();
    Process* get_current();
    uint64_t get_ticks() const;

    Process* processes[MAX_PROCESSES];
    size_t process_count;
    size_t current_index;
    uint64_t quantum;
    uint64_t tick_count;

private:
    int find_next_runnable_index();
};

extern Scheduler scheduler;
