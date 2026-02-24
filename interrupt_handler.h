#pragma once
#include "types/cpu.h"

/** Top-level interrupt handler called from isr_handler (interrupts.asm). Dispatches by vector. */
extern "C" void interrupt_handler(cpu_context_t* ctx);
