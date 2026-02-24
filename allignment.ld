ENTRY(loader)

SECTIONS
{
    . = 0x100000;
    _kernel_start = .;
    .multiboot ALIGN(8) :
    {
        KEEP(*(.multiboot))
    }
    .text ALIGN(0x1000) :
    {
        *(.text)
    }
    .data ALIGN(0x1000) :
    {
        *(.data)
    }
    .rodata ALIGN(0x1000) :
    {
        *(.rodata)
    }
    .bss ALIGN(0x1000) :
    {
        *(.bss)
    }
    _kernel_end = .;
}