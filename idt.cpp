#include "types/idt.h"
#include "types/cpu.h"

idt_entry_t IDT::table[256];
idtr_t IDT::descriptor;

extern "C" {
    void isr0();   void isr1();   void isr2();   void isr3();
    void isr4();   void isr5();   void isr6();   void isr7();
    void isr8();   void isr9();   void isr10();  void isr11();
    void isr12();  void isr13();  void isr14();  void isr15();
    void isr16();  void isr17();  void isr18();  void isr19();
    void isr20();  void isr21();  void isr22();  void isr23();
    void isr24();  void isr25();  void isr26();  void isr27();
    void isr28();  void isr29();  void isr30();  void isr31();
    void isr32();  void isr33();  void isr34();  void isr35();
    void isr36();  void isr37();  void isr38();  void isr39();
    void isr40();  void isr41();  void isr42();  void isr43();
    void isr44();  void isr45();  void isr46();  void isr47();
}

static void* isr_table[48] = {
    reinterpret_cast<void*>(isr0),  reinterpret_cast<void*>(isr1),  reinterpret_cast<void*>(isr2),  reinterpret_cast<void*>(isr3),
    reinterpret_cast<void*>(isr4),  reinterpret_cast<void*>(isr5),  reinterpret_cast<void*>(isr6),  reinterpret_cast<void*>(isr7),
    reinterpret_cast<void*>(isr8),  reinterpret_cast<void*>(isr9),  reinterpret_cast<void*>(isr10), reinterpret_cast<void*>(isr11),
    reinterpret_cast<void*>(isr12), reinterpret_cast<void*>(isr13), reinterpret_cast<void*>(isr14), reinterpret_cast<void*>(isr15),
    reinterpret_cast<void*>(isr16), reinterpret_cast<void*>(isr17), reinterpret_cast<void*>(isr18), reinterpret_cast<void*>(isr19),
    reinterpret_cast<void*>(isr20), reinterpret_cast<void*>(isr21), reinterpret_cast<void*>(isr22), reinterpret_cast<void*>(isr23),
    reinterpret_cast<void*>(isr24), reinterpret_cast<void*>(isr25), reinterpret_cast<void*>(isr26), reinterpret_cast<void*>(isr27),
    reinterpret_cast<void*>(isr28), reinterpret_cast<void*>(isr29), reinterpret_cast<void*>(isr30), reinterpret_cast<void*>(isr31),
    reinterpret_cast<void*>(isr32), reinterpret_cast<void*>(isr33), reinterpret_cast<void*>(isr34), reinterpret_cast<void*>(isr35),
    reinterpret_cast<void*>(isr36), reinterpret_cast<void*>(isr37), reinterpret_cast<void*>(isr38), reinterpret_cast<void*>(isr39),
    reinterpret_cast<void*>(isr40), reinterpret_cast<void*>(isr41), reinterpret_cast<void*>(isr42), reinterpret_cast<void*>(isr43),
    reinterpret_cast<void*>(isr44), reinterpret_cast<void*>(isr45), reinterpret_cast<void*>(isr46), reinterpret_cast<void*>(isr47)
};

void IDT::set_entry(uint8_t n, void* handler, uint8_t type) {
    uint64_t addr = reinterpret_cast<uint64_t>(handler);
    table[n].offset_low = addr & 0xFFFF;
    table[n].offset_mid = (addr >> 16) & 0xFFFF;
    table[n].offset_high = (addr >> 32) & 0xFFFFFFFF;
    table[n].selector = 0x08;
    table[n].ist = 0;
    table[n].type_attr = type;
    table[n].reserved = 0;
}

void IDT::remap_pic() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

void IDT::init() {
    remap_pic();

    for (int i = 0; i < 48; i++) {
        set_entry(i, isr_table[i], 0x8E);
    }

    descriptor.limit = sizeof(table) - 1;
    descriptor.base = reinterpret_cast<uint64_t>(&table);
}

void IDT::load() {
    asm volatile("lidt %0" : : "m"(descriptor));
}
