CXXFLAGS := -m64 -ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-stack-protector -O0

.PHONY: build iso emu

build:
	mkdir -p build

build/syscall.o: syscall.asm | build
	nasm -f elf64 -o build/syscall.o syscall.asm

build/start_kernel.o: start_kernel.asm | build
	nasm -f elf64 -o build/start_kernel.o start_kernel.asm

build/syscall_handler.o: syscall_handler.h syscall_handler.cpp | build
	g++ $(CXXFLAGS) -c -o build/syscall_handler.o syscall_handler.cpp

build/kernel_init.o: kernel_init.cpp kernel_init.h | build
	g++ $(CXXFLAGS) -c -o build/kernel_init.o kernel_init.cpp

build/page_orchestrator.o: page_orchestrator.cpp  page_orchestrator.h | build
	g++ $(CXXFLAGS) -c -o build/page_orchestrator.o page_orchestrator.cpp

build/keyboard.o: keyboard.cpp keyboard.h | build
	g++ $(CXXFLAGS) -c -o build/keyboard.o keyboard.cpp

build/idt.o: idt.cpp  | build
	g++ $(CXXFLAGS) -c -o build/idt.o idt.cpp

build/interrupts.o: interrupts.asm | build
	nasm -f elf64 -o build/interrupts.o interrupts.asm

kernel.elf: build/start_kernel.o build/interrupts.o build/kernel_init.o build/page_orchestrator.o build/keyboard.o build/idt.o build/syscall_handler.o build/syscall.o
	ld -T allignment.ld -melf_x86_64 \
	   build/start_kernel.o \
	   build/interrupts.o \
	   build/kernel_init.o \
	   build/page_orchestrator.o \
	   build/keyboard.o \
	   build/idt.o \
	   build/syscall.o \
	   build/syscall_handler.o \
	   -o kernel.elf


iso: kernel.elf
	mkdir -p iso/boot/grub
	cp kernel.elf iso/boot/kernel.elf
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o os.iso iso


emu: iso
	qemu-system-x86_64 -boot d -cdrom os.iso -m 512