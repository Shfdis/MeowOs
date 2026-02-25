#include "syscall.h"

extern int main();

extern "C" void _start_impl(void) {
    (void)main();
    sys_drop(0);
}
