global syscall_entry
extern syscall_handler
extern in_syscall

syscall_entry:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    push rcx
    push r11
    mov r14, rax
    mov r15, r10
    mov r9, r8         
    mov r8, r15        
    mov rcx, rdx       
    mov rdx, rsi       
    mov rsi, rdi       
    mov rdi, r14

    mov byte [rel in_syscall], 1
    call syscall_handler
    mov byte [rel in_syscall], 0

    ; Same-privilege return path: restore flags and jump to saved post-syscall RIP.
    pop r11
    pop rcx

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    push r11
    popfq
    jmp rcx