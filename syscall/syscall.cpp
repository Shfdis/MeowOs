#include "syscall/syscall.h"
#include "syscall/impl.h"
#include "types/cpu.h"
#include "process.h"
#include "scheduler.h"
#include "drivers/keyboard.h"

Process* current_process = nullptr;

extern "C" {
bool in_syscall = false;
}

extern "C" void syscall_entry();
extern "C" void process_restore_and_switch_to_ctx(cpu_context_t* to_ctx, uint64_t new_cr3);
extern "C" void save_context_and_switch_to(cpu_context_t* save_ctx, void* resume_rip,
                                           cpu_context_t* next_ctx, uint64_t next_cr3);

static Process* keyboard_wait_head = nullptr;
static Process* keyboard_wait_tail = nullptr;

static void add_to_keyboard_wait(Process* p) {
    p->wait_queue_next = nullptr;
    if (keyboard_wait_tail)
        keyboard_wait_tail->wait_queue_next = p;
    else
        keyboard_wait_head = p;
    keyboard_wait_tail = p;
}

static void wake_one_keyboard_waiter() {
    if (!keyboard_wait_head) return;
    Process* p = keyboard_wait_head;
    keyboard_wait_head = p->wait_queue_next;
    if (!keyboard_wait_head)
        keyboard_wait_tail = nullptr;
    p->wait_queue_next = nullptr;
    p->state = ProcessState::Runnable;
}

void wake_keyboard_waiters() {
    wake_one_keyboard_waiter();
}

void keyboard_wait_add(Process* p) {
    add_to_keyboard_wait(p);
}

void initialize_syscalls() {
    uint32_t efer_lo = 0;
    uint32_t efer_hi = 0;
    asm volatile("rdmsr" : "=a"(efer_lo), "=d"(efer_hi) : "c"(0xC0000080));
    uint64_t efer = (static_cast<uint64_t>(efer_hi) << 32) | efer_lo;
    efer |= 1ULL;
    wrmsr(0xC0000080, efer);
    uint64_t star = (static_cast<uint64_t>(0x10) << 48) | (static_cast<uint64_t>(0x08) << 32);
    wrmsr(0xC0000081, star);
    wrmsr(0xC0000082, reinterpret_cast<uint64_t>(&syscall_entry));
    wrmsr(0xC0000084, 0x200);
}

uint64_t syscall_handler(uint64_t num, uint64_t a0, uint64_t a1,
    uint64_t a2, uint64_t a3, uint64_t a4) {
    (void)a3;
    (void)a4;
    switch (num) {
        case static_cast<uint64_t>(SyscallCodes::ALIVE):
            return syscall_alive();
        case static_cast<uint64_t>(SyscallCodes::FEED):
            return syscall_feed(a0);
        case static_cast<uint64_t>(SyscallCodes::TIME):
            return syscall_time();
        case static_cast<uint64_t>(SyscallCodes::PLAY):
            return syscall_play(a0, a1, a2);
        case static_cast<uint64_t>(SyscallCodes::PET):
            return syscall_pet(a0, a1, a2);
        case static_cast<uint64_t>(SyscallCodes::MEOW):
            return syscall_meow(a0, a1, a2);
        case static_cast<uint64_t>(SyscallCodes::DROP):
            return syscall_drop();
        case 7:
            return 0;
    }
    return static_cast<uint64_t>(-1);
}
