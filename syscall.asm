global syscall_entry
extern syscall_handler

syscall_entry:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rcx
    mov r13, r11
    mov r14, rax       
    mov r15, r10       
    mov r9, r8         
    mov r8, r15        
    mov rcx, rdx       
    mov rdx, rsi       
    mov rsi, rdi       
    mov rdi, r14       
    
    call syscall_handler
    
    
    mov r11, r13
    mov rcx, r12
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    
    o64 sysret