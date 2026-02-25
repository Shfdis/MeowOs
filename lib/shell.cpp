#include "syscall.h"
#include "out.h"
static int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { ++a; ++b; }
    return static_cast<unsigned char>(*a) - static_cast<unsigned char>(*b);
}

static int strncmp(const char* a, const char* b, unsigned n) {
    while (n && *a && *b && *a == *b) { --n; ++a; ++b; }
    if (n == 0) return 0;
    return static_cast<unsigned char>(*a) - static_cast<unsigned char>(*b);
}

static const char prompt[] = "meow here: ";


static int readline(char* buf, uint32_t max_size) {
    uint32_t n = 0;
    while (n + 1 < max_size) {
        char c = 0;
        int64_t got = sys_pet(nullptr, &c, 1);
        if (got < 0) return -1;
        if (got == 0) {
            continue;
        }
        if (c == '\b') {
            if (n > 0) {
                n--;
                sys_meow(nullptr, &c, 1);
            }
            continue;
        }
        if (c == '\r') c = '\n';
        buf[n++] = c;
        sys_meow(nullptr, &c, 1);
        if (c == '\n')
            break;
    }
    return static_cast<int>(n);
}

void process_command(const char* line) {
    if (strcmp(line, "help") == 0) {
        putstr("fuck me 2 times baby\n");
    } else if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
        sys_drop(0);
    } else if (strncmp(line, "exec ", 5) == 0) {
        const char* filename = line + 5;
        if (filename[0] == '\0') {
            putstr("exec: usage exec <file>\n");
            return;
        }
        int64_t ret = sys_play(filename, 1);
        if (ret < 0) {
            putstr("exec failed\n");
        }
    }
}

int main() {
    char line[256];
    for (;;) {
        putstr(prompt);
        int n = readline(line, sizeof(line));
        if (n < 0) continue;
        if (n > 0 && line[n - 1] == '\n')
            --n;
        line[n] = '\0';
        process_command(line);
    }
}
