#pragma once
#include "fs/fs_structs.h"
#include "fs/fs_error.h"
#include "fs/disk_io.h"

namespace fs {

class BlockAllocator {
public:
    BlockAllocator() = default;

    void init(DiskIO& disk, Superblock& superblock);

    Error allocate(uint32_t bytes, uint64_t& out_offset);
    void set_free_space(uint64_t offset) { superblock_->free_space = offset; }
    uint64_t free_space() const { return superblock_ ? superblock_->free_space : 0; }
    uint64_t disk_size() const { return superblock_ ? superblock_->disk_size : 0; }

private:
    bool flush_superblock();

    DiskIO* disk_ = nullptr;
    Superblock* superblock_ = nullptr;
};

}
