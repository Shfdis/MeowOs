#pragma once
#include "fs/fs_structs.h"
#include "fs/fs_error.h"
#include "fs/disk_io.h"
#include "fs/block_allocator.h"

namespace fs {

class FileSystem {
public:
    FileSystem() = default;

    Error format(uint64_t disk_size_bytes);
    Error mount();
    bool is_mounted() const { return mounted_; }
    bool is_formatted();
    uint64_t detect_disk_size();

    Error create_file(const char* name);
    Error write_file(const char* name, const void* data, uint32_t size);
    Error read_file(const char* name, void* buffer, uint32_t max_size, uint32_t* out_size);
    Error delete_file(const char* name);
    int list_files(char names[][MAX_NAME_LEN], int max);

private:
    Error find_file(const char* name, uint64_t& out_offset, FileHeader& out_header, uint64_t* prev_offset);
    Error find_tail(uint64_t& out_offset, FileHeader& out_header);
    Error read_header_at(uint64_t offset, FileHeader& out);
    Error write_header_at(uint64_t offset, const FileHeader& hdr);
    Error link_new_file(uint64_t new_header_offset);

    DiskIO disk_;
    BlockAllocator allocator_;
    Superblock superblock_;
    bool mounted_ = false;
};

}
