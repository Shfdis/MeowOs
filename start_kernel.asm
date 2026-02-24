global loader
global pml4_table

;  Multiboot 2 header constants 
MB2_MAGIC    equ 0xE85250D6
MB2_ARCH     equ 0                               ; 0 = i386 protected-mode entry
MB2_LENGTH   equ (mb2_header_end - mb2_header)
MB2_CHECKSUM equ -(MB2_MAGIC + MB2_ARCH + MB2_LENGTH)


;  BSS 
section .bss

align 16
    resb 8192              ; 8 KiB stack buffer
global stack_top
stack_top:                 ; ESP/RSP points here (stack grows downward)

global tss
align 4
tss: resb 108              ; 64-bit TSS (RSP0 at offset 4)

; Writable GDT copy so we can set TSS base without touching .rodata (multiboot header must stay early)
gdt_copy:    resb 40       ; 5 descriptors
gdt_ptr_copy: resb 10      ; limit (2) + base (4) for lgdt; 64-bit lgdt uses 2+8 but we only need 4-byte base

align 4096                 ; Page tables must be 4 KiB-aligned (bits 11:0 == 0)
pml4_table: resb 4096      ; Level 4 — root, loaded into CR3
pdp_table:  resb 4096      ; Level 3
pd_table:   resb 4096      ; Level 2
; 16 page tables for 32 MiB identity mapping (each PT covers 2 MiB)
pt_table:   resb 4096      ; Level 1 — covers virtual 0x000000–0x1FFFFF
pt_table2:  resb 4096      ; Level 1 — covers virtual 0x200000–0x3FFFFF
pt_table3:  resb 4096      ; Level 1 — covers virtual 0x400000–0x5FFFFF
pt_table4:  resb 4096      ; Level 1 — covers virtual 0x600000–0x7FFFFF
pt_table5:  resb 4096      ; Level 1 — covers virtual 0x800000–0x9FFFFF
pt_table6:  resb 4096      ; Level 1 — covers virtual 0xA00000–0xBFFFFF
pt_table7:  resb 4096      ; Level 1 — covers virtual 0xC00000–0xDFFFFF
pt_table8:  resb 4096      ; Level 1 — covers virtual 0xE00000–0xFFFFFF
pt_table9:  resb 4096      ; Level 1 — covers virtual 0x1000000–0x11FFFFF
pt_table10: resb 4096      ; Level 1 — covers virtual 0x1200000–0x13FFFFF
pt_table11: resb 4096      ; Level 1 — covers virtual 0x1400000–0x15FFFFF
pt_table12: resb 4096      ; Level 1 — covers virtual 0x1600000–0x17FFFFF
pt_table13: resb 4096      ; Level 1 — covers virtual 0x1800000–0x19FFFFF
pt_table14: resb 4096      ; Level 1 — covers virtual 0x1A00000–0x1BFFFFF
pt_table15: resb 4096      ; Level 1 — covers virtual 0x1C00000–0x1DFFFFF
pt_table16: resb 4096      ; Level 1 — covers virtual 0x1E00000–0x1FFFFFF 
section .multiboot
align 8
mb2_header:
    dd MB2_MAGIC            ; Magic number GRUB2 scans for
    dd MB2_ARCH             ; Architecture (0 = i386)
    dd MB2_LENGTH           ; Header byte length
    dd MB2_CHECKSUM         ; Makes the four fields sum to 0 mod 2^32
    ; End tag (required): type=0, flags=0, size=8
    dw 0, 0
    dd 8
mb2_header_end:

section .rodata
;  GDT
align 8
gdt_ptr:
    dw gdt_end - gdt - 1   ; Limit
    dd gdt                  ; Base
gdt:
    dq 0                    ; Descriptor 0: null (required)

    ; Descriptor 1 — 64-bit code segment (selector 0x08)
    ;   Access 0x9A = Present | DPL=0 | S=1 | Type=1010 (execute/read code)
    ;   Flags  0xAF = G=1 | D=0 | L=1 (64-bit!) | limit_hi=0xF
    dw 0xFFFF, 0x0000
    db 0x00, 0x9A, 0xAF, 0x00

    ; Descriptor 2 — 64-bit data segment (selector 0x10)
    ;   Access 0x92 = Present | DPL=0 | S=1 | Type=0010 (read/write data)
    dw 0xFFFF, 0x0000
    db 0x00, 0x92, 0x00, 0x00
    ; Descriptor 3 — 64-bit TSS (selector 0x18)
    dw 0x006B            ; Limit 15:0 (108-1)
    dw 0                 ; Base 15:0
    db 0                 ; Base 23:16
    db 0x89              ; Type = 64-bit TSS (available)
    db 0x00              ; Limit 19:16, flags
    db 0                 ; Base 31:24
    dd 0                 ; Base 63:32
gdt_end:
align 4
multiboot_magic:   resd 1
multiboot_info:    resd 1  
section .text
[bits 32]                   ; GRUB2 always hands control in 32-bit protected mode
loader:
    mov  [multiboot_magic], eax
    mov  [multiboot_info], ebx
    ;  1. Stack 
    mov esp, stack_top
    
    xor  eax, eax            ; Value to store: 0
    mov  ecx, 19 * 1024      ; 19 tables × 1024 dwords each (1 PML4 + 1 PDP + 1 PD + 16 PT)
    mov  edi, pml4_table     ; Start of the first table
    rep  stosd               ; Zero all tables in one pass
    mov  edi, pt_table        ; Point EDI at the first PT entry
    mov  eax, 0x00000003      ; Physical address 0 | Present | R/W
    mov  ecx, 512 * 16        ; 512 entries × 16 tables = 8192 entries

.fill_all_pt:
    mov  [edi], eax           ; Write lower 32 bits of entry (upper 32 bits already 0)
    add  eax, 0x1000          ; Advance to next 4 KiB page frame
    add  edi, 8               ; Advance to next 8-byte entry slot
    loop .fill_all_pt         ; Decrement ECX, jump if not zero
    mov  eax, pdp_table
    or   eax, 0x03
    mov  [pml4_table], eax

    ; PDP[0] → pd_table
    mov  eax, pd_table
    or   eax, 0x03
    mov  [pdp_table], eax

    ; PD[0-15] → pt_table[1-16] (maps virtual 0x000000–0x1FFFFFF = 32 MiB)
    mov  eax, pt_table
    mov  ecx, 16
    mov  edi, pd_table

.fill_pd:
    or   eax, 0x03            ; Present | R/W
    mov  [edi], eax
    mov  ebx, eax
    and  ebx, ~0x03           ; Clear flags to get clean address
    add  ebx, 0x1000          ; Next page table
    mov  eax, ebx
    add  edi, 8               ; Next PD entry
    loop .fill_pd
    mov  eax, cr4
    bts  eax, 5               ; Set PAE bit
    mov  cr4, eax
    mov  eax, pml4_table
    mov  cr3, eax
    mov  ecx, 0xC0000080
    rdmsr
    bts  eax, 8               ; Set Long Mode Enable
    wrmsr
    mov  eax, cr0
    bts  eax, 31   
    mov  cr0, eax             ; CPU enters 64-bit compatibility mode here


    ; Load 64-bit GDT and far-jump into full 64-bit mode 
    lgdt [gdt_ptr]
    jmp  0x08:long_mode_entry


;  long_mode_entry 
[bits 64]
long_mode_entry:

    ; The far jump updated CS; update the rest to use our data descriptor.
    mov  ax, 0x10
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    mov  ss, ax

    ; Build writable GDT copy in .bss (cannot write .rodata; multiboot header must stay in first 32KB)
    lea  rsi, [rel gdt]
    lea  rdi, [rel gdt_copy]
    mov  ecx, 5
    rep  movsq                       ; copy 40 bytes
    lea  rax, [rel tss]
    lea  rdi, [rel gdt_copy]
    mov  [rdi + 26], ax              ; TSS descriptor base 15:0
    shr  rax, 16
    mov  [rdi + 28], al              ; base 23:16
    mov  [rdi + 31], ah              ; base 31:24
    shr  rax, 32
    mov  [rdi + 32], eax             ; base 63:32
    lea  rax, [rel gdt_copy]
    lea  rdi, [rel gdt_ptr_copy]
    mov  word [rdi], 39              ; limit
    mov  [rdi + 2], eax              ; base low 32 bits
    xor  eax, eax
    mov  [rdi + 6], eax              ; base high 32 bits (identity-mapped, so 0)
    lgdt [rdi]                       ; load GDT from writable copy
    lea  rax, [rel tss]
    lea  rbx, [rel stack_top]
    mov  [rax + 4], rbx              ; TSS.RSP0 = kernel stack top
    mov  ax, 0x18
    ltr  ax                          ; Load TSS selector

    extern kernel_init
    mov  eax, [multiboot_magic]
    mov  edi, eax
    mov  eax, [multiboot_info]
    mov  esi, eax
    call kernel_init