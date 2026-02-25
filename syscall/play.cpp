#include "syscall/impl.h"
#include "syscall/syscall.h"
#include "process.h"
#include "scheduler.h"
#include "fs/filesystem.h"
#include "fs/fs_error.h"
#include "heap.h"
#include "fs/fs_structs.h"

namespace {
constexpr uint64_t HEAP_START_VIRT = 0x2000000;
constexpr uint32_t MAX_PLAY_SIZE = 256 * 1024;
}

extern fs::FileSystem* g_fs;
extern "C" void process_restore_and_switch_to_ctx(cpu_context_t* to_ctx, uint64_t new_cr3);

uint64_t syscall_play(uint64_t a0, uint64_t a1, uint64_t a2) {
    (void)a2;
    const char* filename_ptr = reinterpret_cast<const char*>(a0);
    bool spawn = (a1 != 0);
    if (!g_fs || !g_fs->is_mounted())
        return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::NotMounted));
    if (!filename_ptr)
        return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::InvalidArg));
    char name_buf[fs::MAX_NAME_LEN];
    for (uint32_t i = 0; i < fs::MAX_NAME_LEN; ++i) {
        name_buf[i] = filename_ptr[i];
        if (filename_ptr[i] == '\0')
            break;
        if (i == fs::MAX_NAME_LEN - 1)
            name_buf[i] = '\0';
    }
    void* buffer = kmalloc(MAX_PLAY_SIZE);
    if (!buffer)
        return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::NoSpace));
    uint32_t bytes_read = 0;
    fs::Error err = g_fs->read_file(name_buf, buffer, MAX_PLAY_SIZE, &bytes_read);
    if (err != fs::Error::Ok) {
        kfree(buffer);
        return static_cast<uint64_t>(static_cast<int64_t>(err));
    }
    if (spawn) {
        if (scheduler.process_count >= Scheduler::MAX_PROCESSES) {
            kfree(buffer);
            return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::NoSpace));
        }
        Process* proc = new Process();
        if (!proc->pml4) {
            kfree(buffer);
            delete proc;
            return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::NoSpace));
        }
        if (!proc->load_binary(buffer, bytes_read, HEAP_START_VIRT)) {
            kfree(buffer);
            delete proc;
            return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::IOError));
        }
        kfree(buffer);
        scheduler.add_process(*proc);
        return 0;
    }
    if (!current_process) {
        kfree(buffer);
        return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::InvalidArg));
    }
    if (!current_process->load_binary(buffer, bytes_read, HEAP_START_VIRT)) {
        kfree(buffer);
        return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::IOError));
    }
    kfree(buffer);
    process_restore_and_switch_to_ctx(&current_process->context, current_process->get_cr3());
    for (;;)
        asm volatile("hlt");
}
