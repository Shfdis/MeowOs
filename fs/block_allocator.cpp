#include "fs/block_allocator.h"

namespace fs {

void BlockAllocator::init(DiskIO& disk, Superblock& superblock) {
    disk_ = &disk;
    superblock_ = &superblock;
}

bool BlockAllocator::flush_superblock() {
    if (!disk_ || !superblock_)
        return false;
    return disk_->write(0, superblock_, sizeof(Superblock));
}

Error BlockAllocator::allocate(uint32_t bytes, uint64_t& out_offset) {
    if (!disk_ || !superblock_)
        return Error::InvalidArg;
    uint64_t start = superblock_->free_space;
    uint64_t end = start + bytes;
    if (end > superblock_->disk_size)
        return Error::NoSpace;
    out_offset = start;
    superblock_->free_space = end;
    if (!flush_superblock())
        return Error::IOError;
    return Error::Ok;
}

}
