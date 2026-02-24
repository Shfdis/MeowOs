#pragma once
#include "types/cpu.h"

class Keyboard {
private:
    static bool shift_pressed;
    static bool ctrl_pressed;
    static bool caps_lock;
    static char buffer[256];
    static int head;
    static int tail;
    static const char scancode_to_ascii[];
    static const char scancode_to_ascii_shift[];

public:
    static void init();
    static void handle_interrupt(cpu_context_t* ctx);
    static char getchar();
    static bool has_char();
    static char peek();
};
