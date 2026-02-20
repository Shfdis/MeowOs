#pragma once
#include "types/multiboot.h"
#include "types/kernel_info.h"

extern "C" {
    void kernel_init(int magic, multiboot_info_t* multiboot_info);
}
