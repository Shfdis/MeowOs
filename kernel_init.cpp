#include "kernel_init.h"
#include "page_orchestrator.h"
#include "syscall_handler.h"
#include "types/idt.h"
#include "keyboard.h"

kernel_basic_info_t kernel_basic_info{};

void* operator new(unsigned long, void* p) { return p; }

static char orchestrator_storage[sizeof(PageOrchestrator)] __attribute__((aligned(alignof(PageOrchestrator))));


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

    kernel_basic_info.page_orchestrator = new (orchestrator_storage) PageOrchestrator(
        bitmap_start,
        kernel_basic_info.total_pages
    );

    for (int i = 0; i < 25; ++i) {
        for (int j = 0; j < 80; ++j) {
            kernel_basic_info.frame_buffer[i][j] = 0;
        }
    }

    IDT::init();
    IDT::load();
    Keyboard::init();
    enable_interrupts();
    initialize_syscalls();
    int cur_line = 0;
    int cur_column = 0;
    int white = 0xF0 << 8;
    kernel_basic_info.frame_buffer[cur_line][cur_column] = white;
    while (true) {
        char c = Keyboard::getchar();
        if (c != 0) {
            if (c == '\n' || c == '\r') {
                kernel_basic_info.frame_buffer[cur_line][cur_column] = 0;
                cur_line++;
                cur_column = 0;
            }
            else if (c == '\b') {
                kernel_basic_info.frame_buffer[cur_line][cur_column] = 0;
                cur_column--;
                if (cur_column == -1) {
                    cur_line--;
                    cur_column++;
                    while (kernel_basic_info.frame_buffer[cur_line][cur_column] != 0) {
                        cur_column++;
                    }
                }
            }
            else {
                kernel_basic_info.frame_buffer[cur_line][cur_column++] = c | 0xF00;
            }
        }
        kernel_basic_info.frame_buffer[cur_line][cur_column] = white;
    }

    halt();
}
