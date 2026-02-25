#include "syscall/impl.h"
#include "syscall/syscall.h"
#include "types/cpu.h"
#include "process.h"
#include "scheduler.h"
#include "drivers/keyboard.h"
#include "fs/filesystem.h"
#include "fs/fs_error.h"
#include "fs/fs_structs.h"

extern fs::FileSystem* g_fs;
extern "C" bool in_syscall;
extern "C" void save_context_and_switch_to(cpu_context_t* save_ctx, void* resume_rip,
                                           cpu_context_t* next_ctx, uint64_t next_cr3);

uint64_t syscall_pet(uint64_t a0, uint64_t a1, uint64_t a2) {
    const char* filename_ptr = reinterpret_cast<const char*>(a0);
    void* buffer = reinterpret_cast<void*>(a1);
    uint32_t max_size = static_cast<uint32_t>(a2);
    if (buffer == nullptr || max_size == 0)
        return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::InvalidArg));
    if (filename_ptr == nullptr) {
        if (!Keyboard::has_char()) {
            Process* blocked = current_process;
            keyboard_wait_add(blocked);
            blocked->state = ProcessState::Blocked;
            Process* next = scheduler.pick_next_runnable();
            if (next == nullptr) {
                enable_interrupts();
                while (!Keyboard::has_char())
                    halt();
                disable_interrupts();
                next = scheduler.pick_next_runnable();
                if (next != nullptr) {
                    current_process = next;
                    in_syscall = false;
                    save_context_and_switch_to(&blocked->context, &&pet_keyboard_resume,
                        &next->context, next->get_cr3());
                }
            }
            if (next != nullptr) {
                current_process = next;
                in_syscall = false;
                save_context_and_switch_to(&blocked->context, &&pet_keyboard_resume,
                    &next->context, next->get_cr3());
            }
        }
    pet_keyboard_resume:
        {
            char* dst = reinterpret_cast<char*>(buffer);
            char c = Keyboard::getchar();
            dst[0] = c;
            return 1;
        }
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
