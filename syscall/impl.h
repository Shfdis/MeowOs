#pragma once
#include "types/types.h"

uint64_t syscall_alive();
uint64_t syscall_feed(uint64_t a0);
uint64_t syscall_time();
uint64_t syscall_play(uint64_t a0, uint64_t a1, uint64_t a2);
uint64_t syscall_pet(uint64_t a0, uint64_t a1, uint64_t a2);
uint64_t syscall_meow(uint64_t a0, uint64_t a1, uint64_t a2);
uint64_t syscall_drop();
uint64_t syscall_list(uint64_t a0, uint64_t a1);
uint64_t syscall_wait(uint64_t a0);
