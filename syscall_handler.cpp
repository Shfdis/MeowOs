#include "syscall_handler.h"
#include "types/cpu.h"
void initialize_syscalls() {
    uint64_t star = ((uint64_t)0x10 << 48) | ((uint64_t)0x08 << 32);
    wrmsr(0xC0000081, star);
    extern void* syscall_entry;
    wrmsr(0xC0000082, (uint64_t)syscall_entry);
    wrmsr(0xC0000084, 0x200); 
}
enum class SyscallCodes : uint64_t {
    ALIVE = 0
};
static const char* MEOW = "MEOW";
uint64_t syscall_handler(uint64_t num, uint64_t a0, uint64_t a1, 
uint64_t a2, uint64_t a3, uint64_t a4) {
    switch (num) {
        case static_cast<uint64_t>(SyscallCodes::ALIVE):
            return reinterpret_cast<uint64_t>(MEOW);
    }
    return -1;
}