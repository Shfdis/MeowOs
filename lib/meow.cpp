#include "syscall.h"
#include "out.h"

static constexpr uint32_t BUF_SIZE = 4096;

int main(int argc, char** argv) {
    if (argc < 2) {
        putstr("meow: usage meow <file>\n");
        sys_drop(1);
    }
    const char* filename = argv[1];
    char buf[BUF_SIZE];
    int64_t n = sys_pet(filename, buf, BUF_SIZE);
    if (n < 0) {
        putstr("meow: read failed\n");
        sys_drop(1);
    }
    sys_meow(nullptr, buf, static_cast<uint32_t>(n));
    sys_drop(0);
}
