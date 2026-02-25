#pragma once
#include "syscall.h"
static void putstr(const char* s) {
    const char* p = s;
    while (*p) ++p;
    sys_meow(nullptr, s, static_cast<uint32_t>(p - s));
}