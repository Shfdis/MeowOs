#include "syscall/impl.h"
#include "syscall/syscall.h"
#include "process.h"

uint64_t syscall_feed(uint64_t a0) {
    if (current_process)
        return static_cast<uint64_t>(current_process->sbrk_pages(static_cast<int64_t>(a0)));
    return static_cast<uint64_t>(-1);
}
