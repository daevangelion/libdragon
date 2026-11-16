#include "tlbcode.h"
#include "cop0.h"
#include "debug.h"
#include "dma.h"
#include "interrupt.h"
#include "n64sys.h"
#include "rompak_internal.h"
#include <string.h>

/* Unlike assertf, malformed executable input must stay fatal under NDEBUG. */
static __attribute__((noreturn)) void tlbcode_fatal(const char *message)
{
    debug_assert_func_f(__FILE__, __LINE__, __func__, "valid TLB code blob", "%s", message);
}
#define REQUIRE(condition, message) \
    do { if (!(condition)) tlbcode_fatal(message); } while (0)

/* Weak directory symbols keep the original linker script and boot path intact. */
extern const tlbcode_unit_t __tlbcode_directory[] __attribute__((weak));
extern const tlbcode_unit_t __tlbcode_directory_end[] __attribute__((weak));
extern const uint8_t __tlbcode_build_id[] __attribute__((weak));
extern const uint8_t __text_start[], __text_end[];
extern const uint8_t __tlbcode_boot_start[] __attribute__((weak));
extern const uint8_t __tlbcode_boot_end[] __attribute__((weak));

#define MAX_UNITS 16
#define MAX_PAGES 62
#define MAX_PAIRS (MAX_PAGES / 2)

typedef struct {
    uint32_t offset;
    void *const *frames;
    uint8_t slots[MAX_PAIRS];
    bool loaded;
} unit_state_t;
static unit_state_t state[MAX_UNITS];
static uint32_t used_slots;
static pi_addr_t blob;
static bool busy;

static unsigned unit_count(void)
{
    if (!__tlbcode_directory) return 0;
    return __tlbcode_directory_end - __tlbcode_directory;
}

static int unit_index(const tlbcode_unit_t *unit)
{
    for (unsigned i = 0; i < unit_count(); i++)
        if (unit == &__tlbcode_directory[i]) return i;
    return -1;
}

static int address_index(uint32_t address)
{
    for (unsigned i = 0; i < unit_count(); i++) {
        const tlbcode_unit_t *u = &__tlbcode_directory[i];
        if (address >= u->virtual_base &&
            address - u->virtual_base < u->page_count * TLBCODE_PAGE_SIZE)
            return i;
    }
    return -1;
}

const tlbcode_unit_t *tlbcode_find(const char *name)
{
    for (unsigned i = 0; i < unit_count(); i++)
        if (!strcmp(name, __tlbcode_directory[i].name))
            return &__tlbcode_directory[i];
    return NULL;
}

bool tlbcode_address_known(uint32_t address)
{
    return (address >= (uint32_t)__text_start && address < (uint32_t)__text_end) ||
        (address >= (uint32_t)__tlbcode_boot_start && address < (uint32_t)__tlbcode_boot_end) ||
        address_index(address) >= 0;
}

bool tlbcode_address_valid(uint32_t address)
{
    if ((address >= (uint32_t)__text_start && address < (uint32_t)__text_end) ||
        (address >= (uint32_t)__tlbcode_boot_start && address < (uint32_t)__tlbcode_boot_end))
        return true;
    int i = address_index(address);
    return i >= 0 && state[i].loaded;
}

const char *tlbcode_unloaded_name(uint32_t address)
{
    int i = address_index(address);
    return i >= 0 && !state[i].loaded ? __tlbcode_directory[i].name : NULL;
}

unsigned tlbcode_wired_entries(void)
{
    return C0_WIRED();
}

bool tlbcode_is_loaded(const tlbcode_unit_t *unit)
{
    int i = unit_index(unit);
    return i >= 0 && state[i].loaded;
}

static void read_blob(void *dst, size_t size, pi_addr_t address)
{
    data_cache_hit_writeback_invalidate(dst, size);
    dma_wait_finished(dma_read_async(dst, address, size));
}

void tlbcode_init(void)
{
    if (blob) return;
    unsigned count = unit_count();
    REQUIRE(count && count <= MAX_UNITS, "TLB code requires N64_TLBCODE=1");
    size_t size;
    pi_addr_t address = rompak_search_ext(".tlbcode", &size);
    REQUIRE(address && size >= 48 + count * 48 && size <= 0x10000000 - (address & 0xFFFFFFF),
        "Invalid TLB code blob");
    struct __attribute__((aligned(16))) {
        char magic[8];
        uint8_t build_id[32];
        uint32_t count, size;
    } header;
    read_blob(&header, sizeof(header), address);
    REQUIRE(!memcmp(header.magic, "TLBCODE1", 8) && header.count == count &&
        header.size == size, "Invalid TLB code header");
    REQUIRE(!memcmp(header.build_id, __tlbcode_build_id, 32),
        "TLB code resident ELF build hash mismatch");
    uint32_t last_end = (48 + count * 48 + 4095) & ~4095;
    for (unsigned i = 0; i < count; i++) {
        struct __attribute__((aligned(16))) {
            tlbcode_unit_t unit;
            uint32_t offset, size;
        } record;
        read_blob(&record, sizeof(record), address + 48 + i * 48);
        REQUIRE(!memcmp(&record.unit, &__tlbcode_directory[i], sizeof(record.unit)) &&
            record.unit.page_count && record.unit.page_count <= MAX_PAGES &&
            !(record.unit.virtual_base & 8191) && !(record.offset & 4095) &&
            record.offset >= last_end && record.offset <= size &&
            record.size == record.unit.page_count * TLBCODE_PAGE_SIZE &&
            record.size <= size - record.offset, "Invalid TLB code unit");
        state[i].offset = record.offset;
        last_end = record.offset + record.size;
    }
    unsigned wired = tlbcode_wired_entries();
    REQUIRE(wired && wired < 32, "Invalid TLB code wired mapping");
    used_slots = (1u << wired) - 1;
    blob = address;
}

/* Caller holds the interrupt lock. Restore EntryHi because it contains ASID.
 * PageMask is always restored so later TLB users do not inherit our page size.
 */
static void map_pair(unsigned slot, uint32_t va, uint32_t even, uint32_t odd)
{
    uint32_t hi = C0_ENTRYHI(), mask = C0_PAGEMASK();
    C0_WRITE_INDEX(slot);
    C0_WRITE_PAGEMASK(0);
    C0_WRITE_ENTRYHI(va);
    C0_WRITE_ENTRYLO0(even);
    C0_WRITE_ENTRYLO1(odd);
    C0_TLBWI();
    C0_WRITE_ENTRYHI(hi);
    C0_WRITE_PAGEMASK(mask);
    asm volatile("sync" ::: "memory");
}

bool tlbcode_load(const tlbcode_unit_t *unit, void *const frames[], size_t count)
{
    int index = unit_index(unit);
    if (!blob || busy || index < 0 || state[index].loaded || !frames ||
        count != unit->page_count) return false;
    unsigned pairs = (count + 1) / 2;
    unsigned free_slots = 32 - __builtin_popcount(used_slots);
    if (pairs > free_slots) return false;
    for (unsigned i = 0; i < count; i++) {
        uint32_t address = (uint32_t)frames[i];
        if ((address & 4095) || address < 0x80000000 ||
            address - 0x80000000 > (unsigned)get_memory_size() - TLBCODE_PAGE_SIZE)
            return false;
        for (unsigned j = 0; j < i; j++)
            if (frames[i] == frames[j]) return false;
        for (unsigned j = 0; j < unit_count(); j++)
            if (state[j].loaded)
                for (unsigned k = 0; k < __tlbcode_directory[j].page_count; k++)
                    if (frames[i] == state[j].frames[k]) return false;
    }
    busy = true;
    for (unsigned i = 0; i < count; i++) {
        data_cache_hit_writeback_invalidate(frames[i], TLBCODE_PAGE_SIZE);
        dma_wait_finished(dma_read_async(frames[i],
            blob + state[index].offset + i * TLBCODE_PAGE_SIZE, TLBCODE_PAGE_SIZE));
    }
    disable_interrupts();
    for (unsigned i = 0; i < pairs; i++) {
        unsigned slot = __builtin_ctz(~used_slots);
        used_slots |= 1u << slot;
        state[index].slots[i] = slot;
        uint32_t even = (PhysicalAddr(frames[i * 2]) >> 6) | 0x1B;
        uint32_t odd = i * 2 + 1 < count ?
            (PhysicalAddr(frames[i * 2 + 1]) >> 6) | 0x1B : 1;
        map_pair(slot, unit->virtual_base + i * 8192, even, odd);
    }
    /* Hit operations need the mapping; do these before publishing loaded. */
    inst_cache_hit_invalidate((void*)unit->virtual_base, count * TLBCODE_PAGE_SIZE);
    asm volatile("sync" ::: "memory");
    state[index].frames = frames;
    state[index].loaded = true;
    busy = false;
    enable_interrupts();
    return true;
}

bool tlbcode_unload(const tlbcode_unit_t *unit)
{
    int index = unit_index(unit);
    if (busy || index < 0 || !state[index].loaded) return false;
    disable_interrupts();
    /* Invalidate both caches while VA translations still exist. */
    inst_cache_hit_invalidate((void*)unit->virtual_base, unit->page_count * TLBCODE_PAGE_SIZE);
    data_cache_hit_invalidate((void*)unit->virtual_base, unit->page_count * TLBCODE_PAGE_SIZE);
    for (unsigned i = 0; i < (unit->page_count + 1) / 2; i++) {
        unsigned slot = state[index].slots[i];
        map_pair(slot, 0x40000000 + slot * 8192, 0, 0);
        used_slots &= ~(1u << slot);
    }
    state[index].loaded = false;
    state[index].frames = NULL;
    enable_interrupts();
    return true;
}
