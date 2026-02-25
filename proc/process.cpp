#include "process.h"
#include "types/kernel_info.h"

namespace {

constexpr uint64_t PAGE_SIZE = 4096;
constexpr uint64_t IDENTITY_MAPPED_END = 32ULL * 1024 * 1024;

constexpr uint64_t PTE_KERNEL = 0x03;
constexpr uint64_t PTE_USER = 0x07;

constexpr uint64_t STACK_PAGES = 4;
constexpr uint64_t STACK_SIZE = STACK_PAGES * PAGE_SIZE;
constexpr uint64_t STACK_TOP_VIRT = 0x2000000;
constexpr uint64_t STACK_BASE_VIRT = STACK_TOP_VIRT - STACK_SIZE;
constexpr uint64_t GUARD_PAGE_VIRT = STACK_BASE_VIRT - PAGE_SIZE;
constexpr uint64_t HEAP_START_VIRT = 0x2000000;

PageOrchestrator& get_orchestrator() {
    return *kernel_basic_info.page_orchestrator;
}

inline uint64_t virt_to_phys(const void* ptr) {
    return reinterpret_cast<uint64_t>(ptr);
}

uint64_t* get_or_alloc_table(uint64_t* entry, uint64_t flags) {
    if ((*entry) & 1) {
        return reinterpret_cast<uint64_t*>(*entry & ~0xFFFULL);
    }
    void* page = get_orchestrator().get_page();
    if (!page)
        return nullptr;
    get_orchestrator().set_page(page);
    uint64_t phys = virt_to_phys(page);
    *entry = phys | flags;
    uint64_t* table = reinterpret_cast<uint64_t*>(page);
    for (uint64_t i = 0; i < 512; ++i)
        table[i] = 0;
    return table;
}

bool map_page(uint64_t* pml4, uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    uint64_t pml4_i = (vaddr >> 39) & 0x1FF;
    uint64_t pdpt_i = (vaddr >> 30) & 0x1FF;
    uint64_t pd_i = (vaddr >> 21) & 0x1FF;
    uint64_t pt_i = (vaddr >> 12) & 0x1FF;

    uint64_t* pdpt = get_or_alloc_table(&pml4[pml4_i], PTE_KERNEL);
    if (!pdpt) return false;
    uint64_t* pd = get_or_alloc_table(&pdpt[pdpt_i], PTE_KERNEL);
    if (!pd) return false;
    uint64_t* pt = get_or_alloc_table(&pd[pd_i], PTE_KERNEL);
    if (!pt) return false;

    pt[pt_i] = (paddr & ~0xFFFULL) | flags;
    return true;
}

void unmap_page(uint64_t* pml4, uint64_t vaddr) {
    uint64_t pml4_i = (vaddr >> 39) & 0x1FF;
    uint64_t pdpt_i = (vaddr >> 30) & 0x1FF;
    uint64_t pd_i = (vaddr >> 21) & 0x1FF;
    uint64_t pt_i = (vaddr >> 12) & 0x1FF;
    uint64_t* pdpt = reinterpret_cast<uint64_t*>(pml4[pml4_i] & ~0xFFFULL);
    uint64_t* pd = reinterpret_cast<uint64_t*>(pdpt[pdpt_i] & ~0xFFFULL);
    uint64_t* pt = reinterpret_cast<uint64_t*>(pd[pd_i] & ~0xFFFULL);
    pt[pt_i] = 0;
}

void release_tables(uint64_t* table, unsigned level) {
    if (level == 0) return;
    for (uint64_t i = 0; i < 512; ++i) {
        if (!(table[i] & 1)) continue;
        uint64_t* next = reinterpret_cast<uint64_t*>(table[i] & ~0xFFFULL);
        release_tables(next, level - 1);
        get_orchestrator().release_page(next);
    }
}

void release_pml4_hierarchy(uint64_t* pml4) {
    for (uint64_t i = 0; i < 512; ++i) {
        if (!(pml4[i] & 1)) continue;
        uint64_t* pdpt = reinterpret_cast<uint64_t*>(pml4[i] & ~0xFFFULL);
        release_tables(pdpt, 2);
        get_orchestrator().release_page(pdpt);
    }
}

}

uint64_t Process::next_pid = 1;

void MemoryMapping::add_region(uint64_t base, uint64_t size, uint8_t permissions) {
    if (count >= MAX_REGIONS) return;
    regions[count].base = base;
    regions[count].size = size;
    regions[count].permissions = permissions;
    ++count;
}

Process::Process() : state(ProcessState::Runnable), wait_queue_next(nullptr), pml4(nullptr), mapped_pages(0), heap_start(HEAP_START_VIRT), heap_end(HEAP_START_VIRT), stack_top(0), stack_size(0), pid(0) {
    context = {};
    void* pml4_page = get_orchestrator().get_page();
    if (!pml4_page) return;
    get_orchestrator().set_page(pml4_page);
    pml4 = reinterpret_cast<uint64_t*>(pml4_page);
    for (uint64_t i = 0; i < 512; ++i)
        pml4[i] = 0;

    map_kernel_memory();

    for (uint64_t i = 0; i < STACK_PAGES; ++i) {
        uint64_t vaddr = STACK_BASE_VIRT + i * PAGE_SIZE;
        void* phys_page = get_orchestrator().get_page();
        if (!phys_page) return;
        get_orchestrator().set_page(phys_page);
        if (!map_page(pml4, vaddr, virt_to_phys(phys_page), PTE_USER))
            return;
        ++mapped_pages;
    }
    memory_mapping.add_region(STACK_BASE_VIRT, STACK_SIZE, static_cast<uint8_t>(PTE_USER));

    stack_top = STACK_TOP_VIRT;
    stack_size = STACK_SIZE;
    context.rsp = stack_top;
    context.ss = 0x10;
    context.rip = 0;
    context.cs = 0x08;
    context.rflags = 0x202;
    pid = next_pid++;
}

Process::~Process() {
    if (!pml4) return;
    release_pml4_hierarchy(pml4);
    get_orchestrator().release_page(pml4);
    pml4 = nullptr;
}

void Process::map_kernel_memory() {
    uint64_t kernel_start = reinterpret_cast<uint64_t>(_kernel_start);
    uint64_t kernel_end = reinterpret_cast<uint64_t>(_kernel_end);
    uint64_t map_end = IDENTITY_MAPPED_END;
    for (uint64_t v = 0; v < map_end; v += PAGE_SIZE) {
        if (!map_page(pml4, v, v, PTE_KERNEL))
            return;
    }
    memory_mapping.add_region(0, map_end, static_cast<uint8_t>(PTE_KERNEL));
}

uint64_t Process::get_cr3() const {
    return virt_to_phys(pml4);
}

void Process::switch_to() {
    (void)this;
}

extern "C" void process_switch_to_asm(cpu_context_t* from, cpu_context_t* to, uint64_t new_cr3);

void Process::switch_to(Process& from, Process& to) {
    process_switch_to_asm(&from.context, &to.context, to.get_cr3());
}

uint64_t Process::brk(uint64_t addr) {
    if (addr < heap_start)
        return heap_end;
    uint64_t page_aligned = (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    int64_t diff = static_cast<int64_t>(page_aligned) - static_cast<int64_t>(heap_end);
    int64_t pages = diff / static_cast<int64_t>(PAGE_SIZE);
    if (pages != 0) {
        if (sbrk_pages(pages) < 0)
            return heap_end;
    }
    return heap_end;
}

uint64_t Process::sbrk(int64_t increment) {
    uint64_t prev = heap_end;
    if (increment == 0)
        return prev;
    int64_t pages = (increment + static_cast<int64_t>(PAGE_SIZE) - 1) / static_cast<int64_t>(PAGE_SIZE);
    if (sbrk_pages(pages) < 0)
        return prev;
    return prev;
}

int64_t Process::sbrk_pages(int64_t n_pages) {
    if (n_pages == 0)
        return static_cast<int64_t>(heap_end);

    if (n_pages > 0) {
        for (int64_t i = 0; i < n_pages; ++i) {
            void* phys_page = get_orchestrator().get_page();
            if (!phys_page)
                return -1;
            get_orchestrator().set_page(phys_page);
            if (!map_page(pml4, heap_end, virt_to_phys(phys_page), PTE_USER)) {
                get_orchestrator().release_page(phys_page);
                return -1;
            }
            heap_end += PAGE_SIZE;
            ++mapped_pages;
        }
    } else {
        int64_t to_unmap = -n_pages;
        for (int64_t i = 0; i < to_unmap; ++i) {
            if (heap_end <= heap_start)
                break;
            heap_end -= PAGE_SIZE;
            unmap_page(pml4, heap_end);
            --mapped_pages;
        }
    }
    return static_cast<int64_t>(heap_end);
}

bool Process::load_binary(const void* data, uint32_t size, uint64_t entry_point) {
    if (!data || size == 0 || pml4 == nullptr)
        return false;
    uint32_t n_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    const char* src = static_cast<const char*>(data);
    uint64_t vaddr = heap_start;
    for (uint32_t i = 0; i < n_pages; ++i) {
        void* phys_page = get_orchestrator().get_page();
        if (!phys_page)
            return false;
        get_orchestrator().set_page(phys_page);
        uint32_t chunk = (i + 1) * PAGE_SIZE <= size ? PAGE_SIZE : (size - i * PAGE_SIZE);
        char* dst = static_cast<char*>(phys_page);
        for (uint32_t j = 0; j < chunk; ++j)
            dst[j] = src[i * PAGE_SIZE + j];
        if (chunk < PAGE_SIZE) {
            for (uint32_t j = chunk; j < PAGE_SIZE; ++j)
                dst[j] = 0;
        }
        if (!map_page(pml4, vaddr, virt_to_phys(phys_page), PTE_USER)) {
            get_orchestrator().release_page(phys_page);
            return false;
        }
        vaddr += PAGE_SIZE;
        heap_end = vaddr;
        ++mapped_pages;
    }
    memory_mapping.add_region(heap_start, n_pages * PAGE_SIZE, static_cast<uint8_t>(PTE_USER));
    context.rip = entry_point;
    context.rsp = stack_top;
    return true;
}
