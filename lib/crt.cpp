#include "syscall.h"

extern int main(int argc, char** argv);

extern "C" void _start_impl(int argc, char** argv) {
    (void)main(argc, argv);
    sys_drop(0);
}
