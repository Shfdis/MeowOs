#include "syscall.h"

static const char prompt[] = "> ";

static void putstr(const char* s) {
    const char* p = s;
    while (*p) ++p;
    sys_meow(nullptr, s, static_cast<uint32_t>(p - s));
}

static int readline(char* buf, uint32_t max_size) {
    uint32_t n = 0;
    while (n + 1 < max_size) {
        char c = 0;
        int64_t got = sys_pet(nullptr, &c, 1);
        if (got <= 0) return -1;
        if (c == '\b') {
            if (n > 0) {
                n--;
                sys_meow(nullptr, &c, 1); // erase one character
            }
            continue; // don't let backspace delete before line start
        }
        if (c == '\r') c = '\n';
        buf[n++] = c;
        sys_meow(nullptr, &c, 1); // local echo
        if (c == '\n')
            break;
    }
    return static_cast<int>(n);
}

extern "C" void _start_impl(void) {
    char line[256];
    for (;;) {
        putstr(prompt);
        int n = readline(line, sizeof(line));
        if (n < 0) continue;
    }
}
