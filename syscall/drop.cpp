#include "syscall/impl.h"
#include "syscall/syscall.h"
#include "types/cpu.h"
#include "process.h"
#include "scheduler.h"

extern "C" void process_restore_and_switch_to_ctx(cpu_context_t* to_ctx, uint64_t new_cr3);

uint64_t syscall_drop() {
    if (!current_process)
        return static_cast<uint64_t>(-1);
    Process* exiting = current_process;
    scheduler.remove_process(*exiting);
    delete exiting;
    if (current_process != nullptr && current_process->state == ProcessState::Blocked) {
        current_process = scheduler.pick_next_runnable();
    }
    if (current_process != nullptr) {
        process_restore_and_switch_to_ctx(&current_process->context, current_process->get_cr3());
    }
    enable_interrupts();
    for (;;) {
        halt();
        Process* woken = scheduler.pick_next_runnable();
        if (woken) {
            disable_interrupts();
            current_process = woken;
            process_restore_and_switch_to_ctx(&current_process->context, current_process->get_cr3());
        }
    }
}
