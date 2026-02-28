#include "syscall.h"
#include "out.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    char names[32][SYS_LIST_NAME_LEN];
    int64_t n = sys_list(reinterpret_cast<char*>(names), 32);
    if (n < 0) {
        putstr("list failed\n");
        sys_drop(1);
    }
    for (int i = 0; i < n; ++i) {
        putstr(names[i]);
        putstr("\n");
    }
    sys_drop(0);
}
