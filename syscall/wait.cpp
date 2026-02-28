#include "syscall/impl.h"
#include "syscall/syscall.h"
#include "types/cpu.h"
#include "process.h"
#include "scheduler.h"

extern "C" bool in_syscall;
extern "C" void save_context_and_switch_to(cpu_context_t* save_ctx, void* resume_rip,
                                           cpu_context_t* next_ctx, uint64_t next_cr3);

uint64_t syscall_wait(uint64_t a0) {
    if (!current_process)
        return static_cast<uint64_t>(-1);
    uint64_t pid = a0;
    if (pid == current_process->pid)
        return static_cast<uint64_t>(-1);
    Process* target = scheduler.find_process_by_pid(pid);
    if (!target)
        return static_cast<uint64_t>(-1);

    Process* blocked = current_process;
    blocked->exit_wait_next = target->exit_wait_head;
    target->exit_wait_head = blocked;
    blocked->state = ProcessState::Blocked;

    Process* next = scheduler.pick_next_runnable();
    if (next == nullptr) {
        enable_interrupts();
        while (scheduler.pick_next_runnable() == nullptr)
            halt();
        disable_interrupts();
        next = scheduler.pick_next_runnable();
        if (next != nullptr) {
            current_process = next;
            in_syscall = false;
            save_context_and_switch_to(&blocked->context, &&wait_resume,
                &next->context, next->get_cr3());
        }
    }
    if (next != nullptr) {
        current_process = next;
        in_syscall = false;
        save_context_and_switch_to(&blocked->context, &&wait_resume,
            &next->context, next->get_cr3());
    }
wait_resume:
    return 0;
}
