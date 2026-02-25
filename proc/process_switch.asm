section .text
global process_switch_to_asm

%assign O_R15    0
%assign O_R14    8
%assign O_R13    16
%assign O_R12    24
%assign O_R11    32
%assign O_R10    40
%assign O_R9     48
%assign O_R8     56
%assign O_RBP    64
%assign O_RDI    72
%assign O_RSI    80
%assign O_RDX    88
%assign O_RCX    96
%assign O_RBX    104
%assign O_RAX    112
%assign O_INT_NO 120
%assign O_ERR    128
%assign O_RIP    136
%assign O_CS     144
%assign O_RFLAGS 152
%assign O_RSP    160
%assign O_SS     168

process_switch_to_asm:
    push rax
    mov rax, rdi

    mov [rax + O_R15], r15
    mov [rax + O_R14], r14
    mov [rax + O_R13], r13
    mov [rax + O_R12], r12
    mov [rax + O_R11], r11
    mov [rax + O_R10], r10
    mov [rax + O_R9], r9
    mov [rax + O_R8], r8
    mov [rax + O_RBP], rbp
    mov [rax + O_RDI], rdi
    mov [rax + O_RSI], rsi
    mov [rax + O_RDX], rdx
    mov [rax + O_RCX], rcx
    mov [rax + O_RBX], rbx
    pop rbx
    mov [rax + O_RAX], rbx

    mov qword [rax + O_INT_NO], 0
    mov qword [rax + O_ERR], 0
    mov rbx, [rsp]
    mov [rax + O_RIP], rbx
    mov word [rax + O_CS], 0x08
    pushfq
    pop rbx
    mov [rax + O_RFLAGS], rbx
    lea rbx, [rsp + 8]
    mov [rax + O_RSP], rbx
    mov word [rax + O_SS], 0x10

    mov cr3, rdx
    mov rax, rsi
    mov r15, [rax + O_R15]
    mov r14, [rax + O_R14]
    mov r13, [rax + O_R13]
    mov r12, [rax + O_R12]
    mov r11, [rax + O_R11]
    mov r10, [rax + O_R10]
    mov r9, [rax + O_R9]
    mov r8, [rax + O_R8]
    mov rbp, [rax + O_RBP]
    mov rdi, [rax + O_RDI]
    mov rsi, [rax + O_RSI]
    mov rdx, [rax + O_RDX]
    mov rcx, [rax + O_RCX]
    mov rbx, [rax + O_RBX]
    mov rsp, [rax + O_RSP]
    push qword [rax + O_RIP]
    mov rax, [rax + O_RAX]
    ret

global save_context_and_switch_to
save_context_and_switch_to:
    push rax
    mov rax, rdi

    mov [rax + O_R15], r15
    mov [rax + O_R14], r14
    mov [rax + O_R13], r13
    mov [rax + O_R12], r12
    mov [rax + O_R11], r11
    mov [rax + O_R10], r10
    mov [rax + O_R9], r9
    mov [rax + O_R8], r8
    mov [rax + O_RBP], rbp
    mov [rax + O_RDI], rdi
    mov [rax + O_RSI], rsi
    mov [rax + O_RDX], rdx
    mov [rax + O_RCX], rcx
    mov [rax + O_RBX], rbx
    pop rbx
    mov [rax + O_RAX], rbx

    mov qword [rax + O_INT_NO], 0
    mov qword [rax + O_ERR], 0
    mov [rax + O_RIP], rsi
    mov word [rax + O_CS], 0x08
    pushfq
    pop rbx
    mov [rax + O_RFLAGS], rbx
    lea rbx, [rsp + 8]
    mov [rax + O_RSP], rbx
    mov word [rax + O_SS], 0x10

    mov cr3, rcx
    mov rax, rdx
    mov r15, [rax + O_R15]
    mov r14, [rax + O_R14]
    mov r13, [rax + O_R13]
    mov r12, [rax + O_R12]
    mov r11, [rax + O_R11]
    mov r10, [rax + O_R10]
    mov r9, [rax + O_R9]
    mov r8, [rax + O_R8]
    mov rbp, [rax + O_RBP]
    mov rdi, [rax + O_RDI]
    mov rsi, [rax + O_RSI]
    mov rdx, [rax + O_RDX]
    mov rcx, [rax + O_RCX]
    mov rbx, [rax + O_RBX]
    mov rsp, [rax + O_RSP]
    push qword [rax + O_RIP]
    push qword [rax + O_RFLAGS]
    popfq
    mov rax, [rax + O_RAX]
    ret

global process_restore_and_switch_to_ctx
process_restore_and_switch_to_ctx:
    mov rax, rdi
    mov cr3, rsi
    mov r15, [rax + O_R15]
    mov r14, [rax + O_R14]
    mov r13, [rax + O_R13]
    mov r12, [rax + O_R12]
    mov r11, [rax + O_R11]
    mov r10, [rax + O_R10]
    mov r9, [rax + O_R9]
    mov r8, [rax + O_R8]
    mov rbp, [rax + O_RBP]
    mov rdi, [rax + O_RDI]
    mov rsi, [rax + O_RSI]
    mov rdx, [rax + O_RDX]
    mov rcx, [rax + O_RCX]
    mov rbx, [rax + O_RBX]
    mov rsp, [rax + O_RSP]
    push qword [rax + O_RIP]
    push qword [rax + O_RFLAGS]
    popfq
    mov rax, [rax + O_RAX]
    ret
