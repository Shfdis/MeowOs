#pragma once
#include "types/types.h"

namespace fs {

static constexpr uint32_t FS_MAGIC = 0x4C465331;
static constexpr uint32_t MAX_NAME_LEN = 64;
static constexpr uint32_t SECTOR_SIZE = 512;

struct Superblock {
    uint32_t magic;
    uint32_t block_size;
    uint64_t first_file;
    uint64_t free_space;
    uint64_t disk_size;
    uint8_t reserved[480];
} __attribute__((packed));

static_assert(sizeof(Superblock) == 512, "Superblock must be exactly 512 bytes");

struct FileHeader {
    char name[MAX_NAME_LEN];
    uint32_t size;
    uint64_t next_header;
    uint64_t data_offset;
    uint8_t in_use;
    uint8_t reserved[3];
} __attribute__((packed));

static constexpr uint64_t FILE_HEADER_SIZE = sizeof(FileHeader);
static constexpr uint64_t SUPERBLOCK_SIZE = sizeof(Superblock);

}
