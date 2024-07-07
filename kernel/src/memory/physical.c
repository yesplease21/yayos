#include "../kernel/limine.h"
#include "../klib/printf.h"
#include "physical.h"

__attribute__((used, section(".requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

pmm_entry_t *head = {0};

static void add_page(uint64_t base) {
    pmm_entry_t *new = (pmm_entry_t *)base;
    if (head == NULL) {
        head = new;
        return;
    }
    new->next = head;
    head = new;
}

void *palloc() {
    void *page_address = (void *)head;
    if (head == NULL) {
        kprintf(KRED "[critical] out of physical memory\n");
        return (void*)0;
    }
    head = head->next;
    return page_address;
}

void pfree(void *ptr) {
    add_page((uint64_t)ptr);
}

void init_pmm() {
    for (int i=0;i<memmap_request.response->entry_count;i++) {
        uint64_t entry_base = memmap_request.response->entries[i]->base;
        uint64_t entry_type = memmap_request.response->entries[i]->type;
        uint64_t entry_size = memmap_request.response->entries[i]->length;
        kprintf(KCYN "[info] entry %d in memmap: 0x%llx | type: %d | size: %llx\n", i, entry_base, entry_type, entry_size);
        if (entry_type == 0) {
            for (uint64_t k=0;k<entry_size;k+=0x1000) {
                add_page(entry_base+k+hhdm_request.response->offset);
            }
            kprintf(KCYN "[info] added range starting from %llx to %llx\n", entry_base, entry_base+entry_size);
        }
    }
}
