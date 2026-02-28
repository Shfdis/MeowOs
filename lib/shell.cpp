#include "syscall.h"
#include "out.h"

static int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { ++a; ++b; }
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
    if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
        sys_drop(0);
    }
    int64_t pid = sys_play(line, 1);
    if (pid >= 0)
        sys_wait(pid);
    else
        putstr("failed\n");
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
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
