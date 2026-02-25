#include "syscall/impl.h"
#include "scheduler.h"

uint64_t syscall_time() {
    return scheduler.get_ticks();
}
