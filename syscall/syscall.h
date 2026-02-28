#pragma once
#include "types/types.h"

class Process;

enum class SyscallCodes : uint64_t {
    ALIVE = 0,
    FEED = 1,
    TIME = 2,
    PLAY = 3,
    PET = 4,
    MEOW = 5,
    DROP = 6,
    LIST = 7,
    WAIT = 8
};

void initialize_syscalls();
extern "C" uint64_t syscall_handler(uint64_t number, uint64_t a0,
    uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);

extern Process* current_process;
void wake_keyboard_waiters();
void keyboard_wait_add(Process* p);
