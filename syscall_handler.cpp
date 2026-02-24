#include "syscall_handler.h"
#include "types/cpu.h"
#include "process.h"
#include "scheduler.h"
#include "fs/filesystem.h"
#include "fs/fs_error.h"
#include "heap.h"
#include "fs/fs_structs.h"
#include "drivers/keyboard.h"
#include "drivers/framebuffer.h"

Process* current_process = nullptr;

/** Set by syscall_entry around syscall_handler; timer must not save process context when true. */
extern "C" {
bool in_syscall = false;
}

extern fs::FileSystem* g_fs;
extern "C" void syscall_entry();

void initialize_syscalls() {
    uint32_t efer_lo = 0;
    uint32_t efer_hi = 0;
    asm volatile("rdmsr" : "=a"(efer_lo), "=d"(efer_hi) : "c"(0xC0000080));
    uint64_t efer = (static_cast<uint64_t>(efer_hi) << 32) | efer_lo;
    efer |= 1ULL; // EFER.SCE: enable SYSCALL/SYSRET
    wrmsr(0xC0000080, efer);
    uint64_t star = (static_cast<uint64_t>(0x10) << 48) | (static_cast<uint64_t>(0x08) << 32);
    wrmsr(0xC0000081, star);
    wrmsr(0xC0000082, reinterpret_cast<uint64_t>(&syscall_entry));
    wrmsr(0xC0000084, 0x200); 
}

enum class SyscallCodes : uint64_t {
    ALIVE = 0,
    FEED = 1,
    TIME = 2,
    PLAY = 3,
    PET = 4,
    MEOW = 5
};

namespace {
constexpr uint64_t HEAP_START_VIRT = 0x2000000;
constexpr uint32_t MAX_PLAY_SIZE = 256 * 1024;
}

uint64_t syscall_handler(uint64_t num, uint64_t a0, uint64_t a1,
uint64_t a2, uint64_t a3, uint64_t a4) {
    (void)a3; (void)a4;
    switch (num) {
        case static_cast<uint64_t>(SyscallCodes::ALIVE):
            return 1;
        case static_cast<uint64_t>(SyscallCodes::FEED):
            if (current_process)
                return static_cast<uint64_t>(current_process->sbrk_pages(static_cast<int64_t>(a0)));
            return static_cast<uint64_t>(-1);
        case static_cast<uint64_t>(SyscallCodes::TIME):
            return scheduler.get_ticks();
        case static_cast<uint64_t>(SyscallCodes::PLAY): {
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
            return 0;
        }
        case static_cast<uint64_t>(SyscallCodes::PET): {
            const char* filename_ptr = reinterpret_cast<const char*>(a0);
            void* buffer = reinterpret_cast<void*>(a1);
            uint32_t max_size = static_cast<uint32_t>(a2);
            if (buffer == nullptr || max_size == 0)
                return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::InvalidArg));
            if (filename_ptr == nullptr) {
                char* dst = reinterpret_cast<char*>(buffer);
                uint32_t n = 0;
                while (n < max_size) {
                    // SYSCALL path masks IF; re-enable IRQs while waiting for keyboard input.
                    enable_interrupts();
                    char c = Keyboard::getchar();
                    disable_interrupts();
                    dst[n++] = c;
                    if (c == '\n')
                        break;
                }
                return static_cast<uint64_t>(n);
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
            uint32_t bytes_read = 0;
            fs::Error err = g_fs->read_file(name_buf, buffer, max_size, &bytes_read);
            if (err != fs::Error::Ok)
                return static_cast<uint64_t>(static_cast<int64_t>(err));
            return static_cast<uint64_t>(bytes_read);
        }
        case static_cast<uint64_t>(SyscallCodes::MEOW): {
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
    }
    return static_cast<uint64_t>(-1);
}