#include "kernel_init.h"
#include "page_orchestrator.h"
#include "syscall_handler.h"
#include "types/idt.h"
#include "drivers/keyboard.h"
#include "drivers/framebuffer.h"
#include "debug_serial.h"
#include "timer.h"
#include "scheduler.h"
#include "fs/filesystem.h"
#include "fs/fs_error.h"
#include "heap.h"
#include "process.h"

extern "C" void process_restore_and_switch_to_ctx(cpu_context_t* to_ctx, uint64_t new_cr3);

kernel_basic_info_t kernel_basic_info{};

fs::FileSystem* g_fs = nullptr;

namespace {
constexpr uint32_t MAX_INIT_SIZE = 256 * 1024;
constexpr uint64_t HEAP_START_VIRT = 0x2000000;
}


void parse_multiboot(multiboot_info_t* multiboot_info) {
    uint32_t total = multiboot_info->header.total_size;
    char* p = reinterpret_cast<char*>(multiboot_info) + 8;

    while (p + 8 <= reinterpret_cast<char*>(multiboot_info) + total) {
        multiboot_tag_header_t* tag = reinterpret_cast<multiboot_tag_header_t*>(p);
        if (tag->type == MULTIBOOT2_TAG_END && tag->size == 8)
            break;
        switch (tag->type) {
            case MULTIBOOT2_TAG_CMDLINE:
                break;
            case MULTIBOOT2_TAG_BASIC_MEM: {
                multiboot2_tag_basic_mem_t* mem = reinterpret_cast<multiboot2_tag_basic_mem_t*>(p);
                kernel_basic_info.mem_begin = reinterpret_cast<char*>(0x400000);
                kernel_basic_info.mem_end = reinterpret_cast<char*>(static_cast<uint64_t>(mem->mem_upper) * 1024 + 0x400000ULL);
                break;
            }
            case MULTIBOOT2_TAG_MEMORY_MAP:
                break;
            case MULTIBOOT2_TAG_FRAMEBUFFER:
                break;
            default:
                break;
        }
        p += (tag->size + 7) & ~7;
    }
}

void kernel_init(int magic, multiboot_info_t* multiboot_info) {
    parse_multiboot(multiboot_info);
    kernel_basic_info.frame_buffer = reinterpret_cast<uint16_t(*)[80]>(0xB8000);
    g_framebuffer.init();

    char* bitmap_start = reinterpret_cast<char*>(
        (reinterpret_cast<uint64_t>(_kernel_end) + 0xFFF) & ~0xFFFULL
    );
    kernel_basic_info.pages_bitmap = bitmap_start;
    kernel_basic_info.pages_bitmap_size = PAGE_BITMAP_SIZE;

    for (uint64_t i = 0; i < PAGE_BITMAP_SIZE; ++i) {
        bitmap_start[i] = 0;
    }

    kernel_basic_info.total_pages = 16ULL * 1024 * 1024 * 1024 / 4096;

    uint64_t kernel_start_page = reinterpret_cast<uint64_t>(_kernel_start) / 4096;
    uint64_t kernel_end_page = (reinterpret_cast<uint64_t>(bitmap_start) + PAGE_BITMAP_SIZE + 4095) / 4096;

    for (uint64_t page = kernel_start_page; page < kernel_end_page; ++page) {
        bitmap_start[page / 8] |= (1 << (page % 8));
    }

    for (uint64_t page = 0; page < 256; ++page) {
        bitmap_start[page / 8] |= (1 << (page % 8));
    }

    kernel_basic_info.pml4_table = pml4_table;

    constexpr uint64_t identity_mapped_end = 32ULL * 1024 * 1024;
    char* heap_start = bitmap_start + PAGE_BITMAP_SIZE;
    void* heap_end_ptr = reinterpret_cast<void*>(identity_mapped_end);
    if (reinterpret_cast<uint64_t>(heap_start) < identity_mapped_end)
        heap_init(heap_start, heap_end_ptr);

    kernel_basic_info.page_orchestrator = new PageOrchestrator(
        bitmap_start,
        kernel_basic_info.total_pages
    );


    debug_serial::init();
    IDT::init();
    IDT::load();
    timer_init(10);
    scheduler.init();
    Keyboard::init();
    enable_interrupts();
    initialize_syscalls();

    g_fs = new fs::FileSystem();
    if (!g_fs->is_formatted()) {
        uint64_t disk_size = g_fs->detect_disk_size();
        if (disk_size == 0) {
            disk_size = 1024 * 1024;
        }
        g_fs->format(disk_size);
    }
    g_fs->mount();
    uint32_t t;
    if (g_fs->read_file("buffer", g_framebuffer.raw_buffer(), 80 * 25 * 2, &t) != fs::Error::Ok) {
        g_fs->create_file("buffer");
        g_framebuffer.clear();
    }

    void* init_buf = kmalloc(MAX_INIT_SIZE);
    if (init_buf && scheduler.process_count < Scheduler::MAX_PROCESSES) {
        uint32_t init_size = 0;
        fs::Error read_err = g_fs->read_file("init", init_buf, MAX_INIT_SIZE, &init_size);
        if (read_err == fs::Error::Ok && init_size > 0) {
            Process* proc = new Process();
            if (proc->pml4 && proc->load_binary(init_buf, init_size, HEAP_START_VIRT)) {
                kfree(init_buf);
                disable_interrupts();
                scheduler.add_process(*proc);
                Process* init_proc = scheduler.get_current();
                process_restore_and_switch_to_ctx(&init_proc->context, init_proc->get_cr3());
            } else {
                kfree(init_buf);
                delete proc;
            }
        } else if (init_buf) {
            kfree(init_buf);
        }
    } else if (init_buf) {
        kfree(init_buf);
    }

    while (true) {
        char c = Keyboard::getchar();
        g_framebuffer.putchar(c);
        g_fs->write_file("buffer", g_framebuffer.raw_buffer(), 80 * 25 * 2);
    }

    halt();
}
