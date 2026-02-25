CXXFLAGS := -m64 -ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-stack-protector -I. -I./proc -I./drivers -O0

.PHONY: build iso emu disk build/shell.bin build/hello_world.bin

build:
	mkdir -p build

build/shell.bin:
	$(MAKE) -C lib shell.bin

build/hello_world.bin:
	$(MAKE) -C lib hello_world.bin

build/syscall.o: syscall.asm | build
	nasm -f elf64 -o build/syscall.o syscall.asm

build/start_kernel.o: start_kernel.asm | build
	nasm -f elf64 -o build/start_kernel.o start_kernel.asm

build/syscall_dispatch.o: syscall/syscall.cpp syscall/syscall.h syscall/impl.h | build
	g++ $(CXXFLAGS) -c -o build/syscall_dispatch.o syscall/syscall.cpp
build/syscall_alive.o: syscall/alive.cpp syscall/impl.h | build
	g++ $(CXXFLAGS) -c -o build/syscall_alive.o syscall/alive.cpp
build/syscall_feed.o: syscall/feed.cpp syscall/impl.h syscall/syscall.h proc/process.h | build
	g++ $(CXXFLAGS) -c -o build/syscall_feed.o syscall/feed.cpp
build/syscall_time.o: syscall/time.cpp syscall/impl.h proc/scheduler.h | build
	g++ $(CXXFLAGS) -c -o build/syscall_time.o syscall/time.cpp
build/syscall_play.o: syscall/play.cpp syscall/impl.h syscall/syscall.h proc/process.h proc/scheduler.h fs/filesystem.h fs/fs_error.h heap.h fs/fs_structs.h | build
	g++ $(CXXFLAGS) -c -o build/syscall_play.o syscall/play.cpp
build/syscall_pet.o: syscall/pet.cpp syscall/impl.h syscall/syscall.h drivers/keyboard.h fs/filesystem.h fs/fs_error.h fs/fs_structs.h | build
	g++ $(CXXFLAGS) -c -o build/syscall_pet.o syscall/pet.cpp
build/syscall_meow.o: syscall/meow.cpp syscall/impl.h drivers/framebuffer.h fs/filesystem.h fs/fs_error.h fs/fs_structs.h | build
	g++ $(CXXFLAGS) -c -o build/syscall_meow.o syscall/meow.cpp
build/syscall_drop.o: syscall/drop.cpp syscall/impl.h syscall/syscall.h proc/process.h proc/scheduler.h types/cpu.h | build
	g++ $(CXXFLAGS) -c -o build/syscall_drop.o syscall/drop.cpp

build/kernel_init.o: kernel_init.cpp kernel_init.h proc/process.h | build
	g++ $(CXXFLAGS) -c -o build/kernel_init.o kernel_init.cpp

build/page_orchestrator.o: page_orchestrator.cpp  page_orchestrator.h | build
	g++ $(CXXFLAGS) -c -o build/page_orchestrator.o page_orchestrator.cpp

build/heap.o: heap.cpp heap.h | build
	g++ $(CXXFLAGS) -c -o build/heap.o heap.cpp

build/keyboard.o: drivers/keyboard.cpp drivers/keyboard.h | build
	g++ $(CXXFLAGS) -c -o build/keyboard.o drivers/keyboard.cpp

build/framebuffer.o: drivers/framebuffer.cpp drivers/framebuffer.h | build
	g++ $(CXXFLAGS) -c -o build/framebuffer.o drivers/framebuffer.cpp

build/interrupt_handler.o: interrupt_handler.cpp interrupt_handler.h proc/process.h proc/scheduler.h syscall_handler.h drivers/keyboard.h | build
	g++ $(CXXFLAGS) -c -o build/interrupt_handler.o interrupt_handler.cpp

build/idt.o: idt.cpp  | build
	g++ $(CXXFLAGS) -c -o build/idt.o idt.cpp

build/interrupts.o: interrupts.asm | build
	nasm -f elf64 -o build/interrupts.o interrupts.asm

build/disk_io.o: fs/disk_io.cpp fs/disk_io.h | build
	g++ $(CXXFLAGS) -c -o build/disk_io.o fs/disk_io.cpp

build/block_allocator.o: fs/block_allocator.cpp fs/block_allocator.h fs/fs_structs.h fs/fs_error.h fs/disk_io.h | build
	g++ $(CXXFLAGS) -c -o build/block_allocator.o fs/block_allocator.cpp

build/filesystem.o: fs/filesystem.cpp fs/filesystem.h fs/fs_structs.h fs/fs_error.h fs/disk_io.h fs/block_allocator.h | build
	g++ $(CXXFLAGS) -c -o build/filesystem.o fs/filesystem.cpp

build/process.o: proc/process.cpp proc/process.h types/kernel_info.h types/cpu.h page_orchestrator.h | build
	g++ $(CXXFLAGS) -c -o build/process.o proc/process.cpp

build/process_switch.o: proc/process_switch.asm | build
	nasm -f elf64 -o build/process_switch.o proc/process_switch.asm

build/timer.o: proc/timer.cpp proc/timer.h | build
	g++ $(CXXFLAGS) -c -o build/timer.o proc/timer.cpp

build/scheduler.o: proc/scheduler.cpp proc/scheduler.h proc/process.h syscall_handler.h | build
	g++ $(CXXFLAGS) -c -o build/scheduler.o proc/scheduler.cpp

kernel.elf: build/start_kernel.o build/interrupts.o build/kernel_init.o build/page_orchestrator.o build/heap.o build/keyboard.o build/framebuffer.o build/interrupt_handler.o build/idt.o build/syscall.o build/syscall_dispatch.o build/syscall_alive.o build/syscall_feed.o build/syscall_time.o build/syscall_play.o build/syscall_pet.o build/syscall_meow.o build/syscall_drop.o build/disk_io.o build/block_allocator.o build/filesystem.o build/process.o build/process_switch.o build/timer.o build/scheduler.o
	ld -T allignment.ld -melf_x86_64 \
	   build/start_kernel.o \
	   build/interrupts.o \
	   build/kernel_init.o \
	   build/page_orchestrator.o \
	   build/heap.o \
	   build/keyboard.o \
	   build/framebuffer.o \
	   build/interrupt_handler.o \
	   build/idt.o \
	   build/syscall.o \
	   build/syscall_dispatch.o \
	   build/syscall_alive.o \
	   build/syscall_feed.o \
	   build/syscall_time.o \
	   build/syscall_play.o \
	   build/syscall_pet.o \
	   build/syscall_meow.o \
	   build/syscall_drop.o \
	   build/disk_io.o \
	   build/block_allocator.o \
	   build/filesystem.o \
	   build/process.o \
	   build/process_switch.o \
	   build/timer.o \
	   build/scheduler.o \
	   -o kernel.elf


iso: kernel.elf
	mkdir -p iso/boot/grub
	cp kernel.elf iso/boot/kernel.elf
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o os.iso iso

disk: build/shell.bin build/hello_world.bin
	if [ ! -f disk.img ]; then dd if=/dev/zero of=disk.img bs=1M count=64; fi
	python disk_util.py format disk.img 1048576
	python disk_util.py add disk.img build/shell.bin init
	python disk_util.py add disk.img build/hello_world.bin hello_world

emu: iso disk
	qemu-system-x86_64 -boot d -cdrom os.iso -drive file=disk.img,format=raw,if=ide -m 512 -serial file:.cursor/debug-ccf50d.log
