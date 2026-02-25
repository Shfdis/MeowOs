#include "scheduler.h"
#include "process.h"
#include "syscall_handler.h"

Scheduler scheduler;

void Scheduler::init() {
    process_count = 0;
    current_index = 0;
    quantum = DEFAULT_QUANTUM;
    tick_count = 0;
    for (size_t i = 0; i < MAX_PROCESSES; ++i)
        processes[i] = nullptr;
}

void Scheduler::add_process(Process& proc) {
    if (process_count >= MAX_PROCESSES)
        return;
    processes[process_count++] = &proc;
    if (process_count == 1)
        current_process = &proc;
}

void Scheduler::remove_process(Process& proc) {
    for (size_t i = 0; i < process_count; ++i) {
        if (processes[i] == &proc) {
            for (size_t j = i; j + 1 < process_count; ++j)
                processes[j] = processes[j + 1];
            processes[--process_count] = nullptr;
            if (current_process == &proc)
                current_process = process_count > 0 ? processes[0] : nullptr;
            if (current_index == i)
                current_index = 0;
            else if (current_index > i)
                --current_index;
            if (current_index >= process_count)
                current_index = 0;
            return;
        }
    }
}

int Scheduler::find_next_runnable_index() {
    if (process_count == 0) return -1;
    for (size_t i = 0; i < process_count; ++i) {
        size_t idx = (current_index + 1 + i) % process_count;
        if (processes[idx]->state == ProcessState::Runnable)
            return static_cast<int>(idx);
    }
    return -1;
}

Process* Scheduler::tick() {
    ++tick_count;
    if (quantum > 0)
        --quantum;

    if (quantum > 0 || process_count == 0)
        return current_process;

    quantum = DEFAULT_QUANTUM;

    int next_idx = find_next_runnable_index();
    if (next_idx < 0)
        return nullptr;

    current_index = static_cast<size_t>(next_idx);
    current_process = processes[current_index];
    return current_process;
}

Process* Scheduler::pick_next_runnable() {
    int next_idx = find_next_runnable_index();
    if (next_idx < 0)
        return nullptr;
    current_index = static_cast<size_t>(next_idx);
    return processes[current_index];
}

Process* Scheduler::get_current() {
    return current_process;
}

uint64_t Scheduler::get_ticks() const {
    return tick_count;
}
