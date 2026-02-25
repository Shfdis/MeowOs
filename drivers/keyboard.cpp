#include "keyboard.h"
#include "types/cpu.h"

bool Keyboard::shift_pressed = false;
bool Keyboard::ctrl_pressed = false;
bool Keyboard::caps_lock = false;
char Keyboard::buffer[256];
int Keyboard::head = 0;
int Keyboard::tail = 0;

const char Keyboard::scancode_to_ascii[] = {
    0,    0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  '\b',   '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,   'a', 's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b',  'n', 'm', ',', '.', '/', 0,   '*', 0,   ' '
};

const char Keyboard::scancode_to_ascii_shift[] = {
    0,    0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   '\t',
    'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,   'A', 'S',
    'D',  'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B',  'N', 'M', '<', '>', '?', 0,   '*', 0,   ' '
};

void Keyboard::init() {
    shift_pressed = false;
    ctrl_pressed = false;
    caps_lock = false;
    head = 0;
    tail = 0;
    uint8_t mask = inb(0x21);
    outb(0x21, mask & ~0x02);
}

void Keyboard::handle_interrupt(cpu_context_t* ctx) {
    uint8_t scancode = inb(0x60);
    outb(0x20, 0x20);

    bool released = scancode & 0x80;
    uint8_t code = scancode & 0x7F;

    switch (code) {
        case 0x1D:
            ctrl_pressed = !released;
            return;
        case 0x2A:
        case 0x36:
            shift_pressed = !released;
            return;
        case 0x3A:
            if (!released) caps_lock = !caps_lock;
            return;
    }

    if (released) return;

    char c = 0;
    if (code < sizeof(scancode_to_ascii)) {
        if (shift_pressed) {
            c = scancode_to_ascii_shift[code];
        } else {
            c = scancode_to_ascii[code];
        }
    }

    if (caps_lock && c >= 'a' && c <= 'z') {
        c -= 32;
    } else if (caps_lock && c >= 'A' && c <= 'Z' && !shift_pressed) {
        c += 32;
    }

    if (c != 0) {
        int next_head = (head + 1) % 256;
        if (next_head != tail) {
            buffer[head] = c;
            head = next_head;
            wake_keyboard_waiters();
        }
    }
}

bool Keyboard::has_char() {
    return head != tail;
}

char Keyboard::peek() {
    if (head == tail) return 0;
    return buffer[tail];
}

int Keyboard::debug_head() {
    return head;
}

int Keyboard::debug_tail() {
    return tail;
}

char Keyboard::getchar() {
    while (head == tail) {
        halt();
    }
    char c = buffer[tail];
    tail = (tail + 1) % 256;
    return c;
}
