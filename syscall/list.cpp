#include "syscall/impl.h"
#include "fs/filesystem.h"
#include "fs/fs_structs.h"

extern fs::FileSystem* g_fs;

uint64_t syscall_list(uint64_t a0, uint64_t a1) {
    if (!g_fs || !g_fs->is_mounted())
        return static_cast<uint64_t>(static_cast<int64_t>(-1));
    char* buffer = reinterpret_cast<char*>(a0);
    int max_files = static_cast<int>(a1);
    if (max_files <= 0)
        return 0;
    if (buffer == nullptr)
        return static_cast<uint64_t>(static_cast<int64_t>(-1));
    int count = g_fs->list_files(reinterpret_cast<char(*)[fs::MAX_NAME_LEN]>(buffer), max_files);
    if (count < 0)
        return static_cast<uint64_t>(static_cast<int64_t>(count));
    return static_cast<uint64_t>(count);
}
