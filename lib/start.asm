section .text.init
global _start
extern _start_impl

_start:
    mov rdi, [rsp]
    mov rsi, [rsp+8]
    jmp _start_impl
