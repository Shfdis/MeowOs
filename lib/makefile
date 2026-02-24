CXXFLAGS := -m64 -ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-stack-protector -fno-pie -I. -O0
LDFLAGS := -T shell.ld -nostdlib -static

BUILD := ../build

.PHONY: all clean

all: $(BUILD)/shell.bin

shell.bin: $(BUILD)/shell.bin

$(BUILD)/start.o: start.asm | $(BUILD)
	nasm -f elf64 -o $(BUILD)/start.o start.asm

$(BUILD)/shell.o: shell.cpp syscall.h types.h | $(BUILD)
	g++ $(CXXFLAGS) -c -o $(BUILD)/shell.o shell.cpp

$(BUILD)/shell.elf: $(BUILD)/start.o $(BUILD)/shell.o shell.ld
	ld $(LDFLAGS) -o $(BUILD)/shell.elf $(BUILD)/start.o $(BUILD)/shell.o

$(BUILD)/shell.bin: $(BUILD)/shell.elf
	objcopy -O binary $(BUILD)/shell.elf $(BUILD)/shell.bin

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -f $(BUILD)/shell.elf $(BUILD)/shell.bin $(BUILD)/start.o $(BUILD)/shell.o
