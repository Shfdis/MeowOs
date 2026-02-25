#include "interrupt_handler.h"
#include "process.h"
#include "scheduler.h"
#include "syscall_handler.h"
#include "drivers/keyboard.h"
#include "types/cpu.h"

extern "C" void process_restore_and_switch_to_ctx(cpu_context_t* to_ctx, uint64_t new_cr3);
extern "C" bool in_syscall;

extern "C" void interrupt_handler(cpu_context_t* ctx) {
    if (ctx->int_no == 32) {
        Process* saved_current = current_process;
        if (saved_current && !in_syscall && ctx->rip >= saved_current->heap_start) {
            uint64_t* s = reinterpret_cast<uint64_t*>(&saved_current->context);
            uint64_t* f = reinterpret_cast<uint64_t*>(ctx);
            for (size_t i = 0; i < 22; ++i)
                s[i] = f[i];
            Process* next = scheduler.tick();
            if (next && next != saved_current) {
                current_process = next;
                outb(0x20, 0x20);
                process_restore_and_switch_to_ctx(&next->context, next->get_cr3());
            }
        }
        outb(0x20, 0x20);
        return;
    }
    if (ctx->int_no == 33) {
        Keyboard::handle_interrupt(ctx);
        return;
    }
}
