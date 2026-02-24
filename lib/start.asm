section .text.init
global _start
extern _start_impl

_start:
    ; Set up stack (already set by kernel to 0x2000000)
    ; Jump to C++ entry point
    jmp _start_impl
