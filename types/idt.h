#pragma once
#include "types.h"

struct idt_entry_t {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idtr_t {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

class IDT {
private:
    static idt_entry_t table[256];
    static idtr_t descriptor;

public:
    static void init();
    static void set_entry(uint8_t n, void* handler, uint8_t type);
    static void load();
    static void remap_pic();
};
