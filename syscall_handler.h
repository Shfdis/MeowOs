#pragma once
#include "types/types.h"

class Process;

void initialize_syscalls();
extern "C" uint64_t syscall_handler(uint64_t number, uint64_t a0,
    uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);

/** Global current process (set by scheduler). */
extern Process* current_process;