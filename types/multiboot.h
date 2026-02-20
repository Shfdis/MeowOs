#pragma once
#include "types.h"

#define MULTIBOOT2_TAG_END         0
#define MULTIBOOT2_TAG_CMDLINE     1
#define MULTIBOOT2_TAG_BOOTLOADER  2
#define MULTIBOOT2_TAG_MODULE      3
#define MULTIBOOT2_TAG_BASIC_MEM   4
#define MULTIBOOT2_TAG_BIOS_BOOT   5
#define MULTIBOOT2_TAG_MEMORY_MAP  6
#define MULTIBOOT2_TAG_VBE         7
#define MULTIBOOT2_TAG_FRAMEBUFFER 8
#define MULTIBOOT2_TAG_ELF_SYMS    9

extern "C" {
    typedef struct {
        uint32_t total_size;
        uint32_t reserved;
    } multiboot_header_t;

    typedef struct {
        multiboot_header_t header;
    } __attribute__((packed)) multiboot_info_t;
}

struct multiboot_tag_header_t {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

struct multiboot2_tag_basic_mem_t {
    multiboot_tag_header_t header;
    uint32_t mem_lower;
    uint32_t mem_upper;
} __attribute__((packed));

struct multiboot2_mmap_entry_t {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));

struct multiboot2_tag_mmap_t {
    multiboot_tag_header_t header;
    uint32_t entry_size;
    uint32_t entry_version;
} __attribute__((packed));

#define MULTIBOOT2_MEMORY_AVAILABLE    1
#define MULTIBOOT2_MEMORY_RESERVED     2
#define MULTIBOOT2_MEMORY_ACPI_RECLAIM 3
#define MULTIBOOT2_MEMORY_NVS          4
#define MULTIBOOT2_MEMORY_BADRAM       5
