#include "syscall/impl.h"
#include "drivers/framebuffer.h"
#include "fs/filesystem.h"
#include "fs/fs_error.h"
#include "fs/fs_structs.h"

extern fs::FileSystem* g_fs;

uint64_t syscall_meow(uint64_t a0, uint64_t a1, uint64_t a2) {
    const char* filename_ptr = reinterpret_cast<const char*>(a0);
    const void* buffer = reinterpret_cast<const void*>(a1);
    uint32_t size = static_cast<uint32_t>(a2);
    if (size > 0 && buffer == nullptr)
        return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::InvalidArg));
    if (filename_ptr == nullptr) {
        g_framebuffer.write(buffer, size);
        return 0;
    }
    if (!g_fs || !g_fs->is_mounted())
        return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::NotMounted));
    char name_buf[fs::MAX_NAME_LEN];
    for (uint32_t i = 0; i < fs::MAX_NAME_LEN; ++i) {
        name_buf[i] = filename_ptr[i];
        if (filename_ptr[i] == '\0')
            break;
        if (i == fs::MAX_NAME_LEN - 1)
            name_buf[i] = '\0';
    }
    fs::Error err = g_fs->write_file(name_buf, buffer, size);
    if (err != fs::Error::Ok)
        return static_cast<uint64_t>(static_cast<int64_t>(err));
    return 0;
}
