#pragma once
#include "types.h"

#define SYS_ALIVE 0
#define SYS_FEED  1
#define SYS_TIME  2
#define SYS_PLAY  3
#define SYS_PET   4
#define SYS_MEOW  5

static inline uint64_t syscall0(uint64_t num) {
    uint64_t ret;
    asm volatile("syscall" : "=a"(ret) : "a"(num) : "rcx", "r11", "memory");
    return ret;
}

static inline uint64_t syscall1(uint64_t num, uint64_t a0) {
    uint64_t ret;
    asm volatile("syscall" : "=a"(ret) : "a"(num), "D"(a0) : "rcx", "r11", "memory");
    return ret;
}

static inline uint64_t syscall3(uint64_t num, uint64_t a0, uint64_t a1, uint64_t a2) {
    uint64_t ret;
    asm volatile("syscall" : "=a"(ret) : "a"(num), "D"(a0), "S"(a1), "d"(a2) : "rcx", "r11", "memory");
    return ret;
}

static inline uint64_t sys_alive(void) {
    return syscall0(SYS_ALIVE);
}

static inline int64_t sys_feed(int64_t pages) {
    return static_cast<int64_t>(syscall1(SYS_FEED, static_cast<uint64_t>(pages)));
}

static inline uint64_t sys_time(void) {
    return syscall0(SYS_TIME);
}

static inline int64_t sys_play(const char* filename, int spawn) {
    return static_cast<int64_t>(syscall3(SYS_PLAY,
        reinterpret_cast<uint64_t>(filename),
        static_cast<uint64_t>(spawn ? 1 : 0),
        0));
}

static inline int64_t sys_pet(const char* filename, void* buf, uint32_t max_size) {
    return static_cast<int64_t>(syscall3(SYS_PET,
        reinterpret_cast<uint64_t>(filename),
        reinterpret_cast<uint64_t>(buf),
        static_cast<uint64_t>(max_size)));
}

static inline int64_t sys_meow(const char* filename, const void* buf, uint32_t size) {
    return static_cast<int64_t>(syscall3(SYS_MEOW,
        reinterpret_cast<uint64_t>(filename),
        reinterpret_cast<uint64_t>(buf),
        static_cast<uint64_t>(size)));
}
