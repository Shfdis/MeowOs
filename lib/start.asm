section .text.init
global _start
extern _start_impl

_start:
    jmp _start_impl
