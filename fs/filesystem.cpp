#include "fs/filesystem.h"

namespace fs {

static void copy_name(char* dst, const char* src) {
    for (uint32_t i = 0; i < MAX_NAME_LEN; ++i) {
        dst[i] = src[i];
        if (src[i] == '\0')
            break;
    }
    dst[MAX_NAME_LEN - 1] = '\0';
}

static bool names_equal(const char* a, const char* b) {
    for (uint32_t i = 0; i < MAX_NAME_LEN; ++i) {
        if (a[i] != b[i])
            return false;
        if (a[i] == '\0')
            return true;
    }
    return true;
}

static uint32_t name_len(const char* s) {
    uint32_t i = 0;
    while (i < MAX_NAME_LEN && s[i] != '\0')
        ++i;
    return i;
}

Error FileSystem::read_header_at(uint64_t offset, FileHeader& out) {
    if (!disk_.read(offset, &out, FILE_HEADER_SIZE))
        return Error::IOError;
    return Error::Ok;
}

Error FileSystem::write_header_at(uint64_t offset, const FileHeader& hdr) {
    if (!disk_.write(offset, &hdr, FILE_HEADER_SIZE))
        return Error::IOError;
    return Error::Ok;
}

Error FileSystem::find_file(const char* name, uint64_t& out_offset, FileHeader& out_header, uint64_t* prev_offset) {
    if (!mounted_ || !name)
        return Error::InvalidArg;
    uint64_t cur = superblock_.first_file;
    uint64_t prev = 0;
    while (cur != 0) {
        Error e = read_header_at(cur, out_header);
        if (e != Error::Ok)
            return e;
        if (out_header.in_use && names_equal(out_header.name, name)) {
            out_offset = cur;
            if (prev_offset)
                *prev_offset = prev;
            return Error::Ok;
        }
        prev = cur;
        cur = out_header.next_header;
    }
    return Error::NotFound;
}

Error FileSystem::find_tail(uint64_t& out_offset, FileHeader& out_header) {
    if (!mounted_)
        return Error::InvalidArg;
    uint64_t cur = superblock_.first_file;
    if (cur == 0) {
        out_offset = 0;
        return Error::Ok;
    }
    for (;;) {
        Error e = read_header_at(cur, out_header);
        if (e != Error::Ok)
            return e;
        if (out_header.next_header == 0) {
            out_offset = cur;
            return Error::Ok;
        }
        cur = out_header.next_header;
    }
}

Error FileSystem::link_new_file(uint64_t new_header_offset) {
    if (superblock_.first_file == 0) {
        superblock_.first_file = new_header_offset;
        return disk_.write(0, &superblock_, sizeof(Superblock)) ? Error::Ok : Error::IOError;
    }
    FileHeader tail;
    uint64_t tail_offset;
    Error e = find_tail(tail_offset, tail);
    if (e != Error::Ok)
        return e;
    tail.next_header = new_header_offset;
    return write_header_at(tail_offset, tail);
}

Error FileSystem::format(uint64_t disk_size_bytes) {
    if (disk_size_bytes < SUPERBLOCK_SIZE + FILE_HEADER_SIZE)
        return Error::InvalidArg;
    disk_.set_disk_size(disk_size_bytes);
    superblock_.magic = FS_MAGIC;
    superblock_.block_size = SECTOR_SIZE;
    superblock_.first_file = 0;
    superblock_.free_space = SUPERBLOCK_SIZE;
    superblock_.disk_size = disk_size_bytes;
    if (!disk_.write(0, &superblock_, sizeof(Superblock)))
        return Error::IOError;
    mounted_ = false;
    return Error::Ok;
}

Error FileSystem::mount() {
    uint64_t disk_size = disk_.detect_disk_size();
    if (disk_size == 0)
        return Error::IOError;
    disk_.set_disk_size(disk_size);
    if (!disk_.read(0, &superblock_, sizeof(Superblock)))
        return Error::IOError;
    if (superblock_.magic != FS_MAGIC || superblock_.disk_size == 0)
        return Error::InvalidArg;
    allocator_.init(disk_, superblock_);
    mounted_ = true;
    return Error::Ok;
}

bool FileSystem::is_formatted() {
    uint64_t disk_size = disk_.detect_disk_size();
    if (disk_size == 0)
        return false;
    disk_.set_disk_size(disk_size);
    Superblock sb;
    if (!disk_.read(0, &sb, sizeof(Superblock)))
        return false;
    return sb.magic == FS_MAGIC && sb.disk_size > 0;
}

uint64_t FileSystem::detect_disk_size() {
    return disk_.detect_disk_size();
}

Error FileSystem::create_file(const char* name) {
    if (!mounted_ || !name)
        return Error::InvalidArg;
    if (name_len(name) == 0)
        return Error::InvalidArg;
    FileHeader probe;
    uint64_t dummy;
    if (find_file(name, dummy, probe, nullptr) == Error::Ok)
        return Error::AlreadyExists;
    uint64_t offset;
    Error e = allocator_.allocate(static_cast<uint32_t>(FILE_HEADER_SIZE), offset);
    if (e != Error::Ok)
        return e;
    FileHeader hdr;
    for (uint32_t i = 0; i < MAX_NAME_LEN; ++i)
        hdr.name[i] = '\0';
    copy_name(hdr.name, name);
    hdr.size = 0;
    hdr.next_header = 0;
    hdr.data_offset = 0;
    hdr.in_use = 1;
    e = write_header_at(offset, hdr);
    if (e != Error::Ok)
        return e;
    return link_new_file(offset);
}

Error FileSystem::write_file(const char* name, const void* data, uint32_t size) {
    if (!mounted_ || !name)
        return Error::InvalidArg;
    FileHeader hdr;
    uint64_t hdr_offset;
    Error e = find_file(name, hdr_offset, hdr, nullptr);
    if (e != Error::Ok)
        return e;
    uint64_t data_offset;
    e = allocator_.allocate(size, data_offset);
    if (e != Error::Ok)
        return e;
    if (size > 0 && data && !disk_.write(data_offset, data, size))
        return Error::IOError;
    hdr.size = size;
    hdr.data_offset = data_offset;
    return write_header_at(hdr_offset, hdr);
}

Error FileSystem::read_file(const char* name, void* buffer, uint32_t max_size, uint32_t* out_size) {
    if (!mounted_ || !name || !buffer || !out_size)
        return Error::InvalidArg;
    FileHeader hdr;
    uint64_t dummy;
    Error e = find_file(name, dummy, hdr, nullptr);
    if (e != Error::Ok)
        return e;
    uint32_t to_read = hdr.size;
    if (to_read > max_size)
        to_read = max_size;
    *out_size = to_read;
    if (to_read == 0)
        return Error::Ok;
    if (hdr.data_offset == 0)
        return Error::Ok;
    if (!disk_.read(hdr.data_offset, buffer, to_read))
        return Error::IOError;
    return Error::Ok;
}

Error FileSystem::delete_file(const char* name) {
    if (!mounted_ || !name)
        return Error::InvalidArg;
    FileHeader hdr;
    uint64_t hdr_offset;
    uint64_t prev_offset;
    Error e = find_file(name, hdr_offset, hdr, &prev_offset);
    if (e != Error::Ok)
        return e;
    hdr.in_use = 0;
    e = write_header_at(hdr_offset, hdr);
    if (e != Error::Ok)
        return e;
    if (prev_offset == 0) {
        superblock_.first_file = hdr.next_header;
        if (!disk_.write(0, &superblock_, sizeof(Superblock)))
            return Error::IOError;
    } else {
        FileHeader prev_hdr;
        if (read_header_at(prev_offset, prev_hdr) != Error::Ok)
            return Error::IOError;
        prev_hdr.next_header = hdr.next_header;
        if (write_header_at(prev_offset, prev_hdr) != Error::Ok)
            return Error::IOError;
    }
    return Error::Ok;
}

int FileSystem::list_files(char names[][MAX_NAME_LEN], int max) {
    if (!mounted_ || !names || max <= 0)
        return -1;
    int count = 0;
    uint64_t cur = superblock_.first_file;
    FileHeader hdr;
    while (cur != 0 && count < max) {
        if (read_header_at(cur, hdr) != Error::Ok)
            return -1;
        if (hdr.in_use) {
            copy_name(names[count], hdr.name);
            ++count;
        }
        cur = hdr.next_header;
    }
    return count;
}

}
