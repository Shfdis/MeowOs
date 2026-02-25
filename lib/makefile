CXXFLAGS := -m64 -ffreestanding  -fno-exceptions -fno-rtti -fno-stack-protector -fno-pie -I. -O0
LDFLAGS := -T shell.ld -static

BUILD := ../build

.PHONY: all clean shell.bin hello_world.bin

all: $(BUILD)/shell.bin $(BUILD)/hello_world.bin

shell.bin: $(BUILD)/shell.bin
hello_world.bin: $(BUILD)/hello_world.bin

$(BUILD)/start.o: start.asm | $(BUILD)
	nasm -f elf64 -o $(BUILD)/start.o start.asm

$(BUILD)/crt.o: crt.cpp | $(BUILD)
	g++ $(CXXFLAGS) -c -o $(BUILD)/crt.o crt.cpp

$(BUILD)/shell.o: shell.cpp syscall.h types.h | $(BUILD)
	g++ $(CXXFLAGS) -c -o $(BUILD)/shell.o shell.cpp

$(BUILD)/malloc.o: malloc.cpp malloc.h syscall.h types.h | $(BUILD)
	g++ $(CXXFLAGS) -c -o $(BUILD)/malloc.o malloc.cpp

$(BUILD)/shell.elf: $(BUILD)/start.o $(BUILD)/crt.o $(BUILD)/shell.o $(BUILD)/malloc.o shell.ld
	ld $(LDFLAGS) -o $(BUILD)/shell.elf $(BUILD)/start.o $(BUILD)/crt.o $(BUILD)/shell.o $(BUILD)/malloc.o

$(BUILD)/shell.bin: $(BUILD)/shell.elf
	objcopy -O binary $(BUILD)/shell.elf $(BUILD)/shell.bin

$(BUILD)/hello_world.o: hello_world.cpp out.h syscall.h | $(BUILD)
	g++ $(CXXFLAGS) -c -o $(BUILD)/hello_world.o hello_world.cpp

$(BUILD)/hello_world.elf: $(BUILD)/start.o $(BUILD)/crt.o $(BUILD)/hello_world.o $(BUILD)/malloc.o shell.ld
	ld $(LDFLAGS) -o $(BUILD)/hello_world.elf $(BUILD)/start.o $(BUILD)/crt.o $(BUILD)/hello_world.o $(BUILD)/malloc.o

$(BUILD)/hello_world.bin: $(BUILD)/hello_world.elf
	objcopy -O binary $(BUILD)/hello_world.elf $(BUILD)/hello_world.bin

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -f $(BUILD)/shell.elf $(BUILD)/shell.bin $(BUILD)/hello_world.elf $(BUILD)/hello_world.bin $(BUILD)/start.o $(BUILD)/crt.o $(BUILD)/shell.o $(BUILD)/hello_world.o $(BUILD)/malloc.o
