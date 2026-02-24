#pragma once
#include "types/cpu.h"
#include "types/types.h"

namespace debug_serial {
constexpr uint16_t COM1 = 0x3F8;

inline void init() {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x01);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

inline void putc(char c) {
    while ((inb(COM1 + 5) & 0x20) == 0) {}
    outb(COM1, static_cast<uint8_t>(c));
}

inline void puts(const char* s) {
    while (*s) { putc(*s++); }
}

inline void put_hex(uint64_t x) {
    const char hex[] = "0123456789ABCDEF";
    puts("0x");
    if (x == 0) { putc('0'); return; }
    char buf[20];
    int i = 0;
    for (; x && i < 16; i++) { buf[i] = hex[x & 15]; x >>= 4; }
    while (i--) putc(buf[i]);
}
}
