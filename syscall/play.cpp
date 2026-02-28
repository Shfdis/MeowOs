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
constexpr uint64_t STACK_TOP_VIRT = 0x2000000;
constexpr uint32_t MAX_PLAY_SIZE = 256 * 1024;
constexpr uint32_t MAX_CMDLINE = 256;
constexpr uint32_t MAX_ARGC = 32;
}

extern fs::FileSystem* g_fs;
extern "C" void process_restore_and_switch_to_ctx(cpu_context_t* to_ctx, uint64_t new_cr3);

static uint32_t parse_cmdline(char* line_buf, uint32_t line_len, const char* argv_out[MAX_ARGC]) {
    uint32_t argc = 0;
    uint32_t i = 0;
    while (i < line_len && argc < MAX_ARGC) {
        while (i < line_len && (line_buf[i] == ' ' || line_buf[i] == '\t'))
            ++i;
        if (i >= line_len || line_buf[i] == '\0')
            break;
        argv_out[argc++] = &line_buf[i];
        while (i < line_len && line_buf[i] != '\0' && line_buf[i] != ' ' && line_buf[i] != '\t')
            ++i;
        if (i < line_len && line_buf[i] != '\0') {
            line_buf[i] = '\0';
            ++i;
        }
    }
    return argc;
}

static bool setup_argc_argv(Process* proc, uint64_t stack_top, uint32_t argc, const char* argv[]) {
    size_t string_len = 0;
    for (uint32_t i = 0; i < argc; ++i) {
        const char* s = argv[i];
        while (*s) { ++string_len; ++s; }
        ++string_len;
    }
    size_t ptr_size = static_cast<size_t>(argc + 1) * 8;
    size_t total = 8 + 8 + ptr_size + string_len;
    total = (total + 15) & ~15ULL;
    if (total > 4096 * 4)
        return false;
    uint64_t base = stack_top - total;

    char* buf = static_cast<char*>(kmalloc(total));
    if (!buf)
        return false;
    size_t off = 0;
    *reinterpret_cast<uint64_t*>(buf + off) = static_cast<uint64_t>(argc);
    off += 8;
    *reinterpret_cast<uint64_t*>(buf + off) = base + 16;
    off += 8;
    uint64_t strings_at = base + 16 + ptr_size;
    for (uint32_t i = 0; i < argc; ++i) {
        *reinterpret_cast<uint64_t*>(buf + off) = strings_at;
        const char* s = argv[i];
        while (*s)
            ++s;
        size_t len = static_cast<size_t>(s - argv[i]) + 1;
        strings_at += len;
        off += 8;
    }
    *reinterpret_cast<uint64_t*>(buf + off) = 0;
    off += 8;
    for (uint32_t i = 0; i < argc; ++i) {
        const char* s = argv[i];
        while (*s)
            buf[off++] = *s++;
        buf[off++] = '\0';
    }
    bool ok = proc->write_at(base, buf, total);
    kfree(buf);
    if (ok)
        proc->context.rsp = base;
    return ok;
}

uint64_t syscall_play(uint64_t a0, uint64_t a1, uint64_t a2) {
    (void)a2;
    const char* cmdline_ptr = reinterpret_cast<const char*>(a0);
    bool spawn = (a1 != 0);
    if (!g_fs || !g_fs->is_mounted())
        return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::NotMounted));
    if (!cmdline_ptr)
        return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::InvalidArg));

    char line_buf[MAX_CMDLINE];
    uint32_t line_len = 0;
    for (; line_len < MAX_CMDLINE && cmdline_ptr[line_len] != '\0'; ++line_len)
        line_buf[line_len] = cmdline_ptr[line_len];
    if (line_len >= MAX_CMDLINE)
        line_len = MAX_CMDLINE - 1;
    line_buf[line_len] = '\0';

    const char* argv[MAX_ARGC];
    uint32_t argc = parse_cmdline(line_buf, line_len + 1, argv);
    if (argc == 0)
        return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::InvalidArg));

    char name_buf[fs::MAX_NAME_LEN];
    const char* prog = argv[0];
    uint32_t name_i = 0;
    for (; name_i < fs::MAX_NAME_LEN - 1 && prog[name_i] != '\0'; ++name_i)
        name_buf[name_i] = prog[name_i];
    name_buf[name_i] = '\0';

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
        if (!setup_argc_argv(proc, STACK_TOP_VIRT, argc, argv)) {
            kfree(buffer);
            delete proc;
            return static_cast<uint64_t>(static_cast<int64_t>(fs::Error::NoSpace));
        }
        kfree(buffer);
        scheduler.add_process(*proc);
        return static_cast<uint64_t>(proc->pid);
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
