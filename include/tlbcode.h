/** @file tlbcode.h Explicit, opt-in VR4300 code-unit lifetime management. */
#ifndef __LIBDRAGON_TLBCODE_H
#define __LIBDRAGON_TLBCODE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define TLBCODE_PAGE_SIZE 4096

typedef struct {
    char name[32];
    uint32_t virtual_base;
    uint32_t page_count;
} tlbcode_unit_t;

/** Validate the rompak blob against the resident ELF build ID. Fatal on mismatch.
 * Call once from application code, before loading units. No heap allocation.
 * Load/unload must be serialized by the application, never called in interrupts.
 * The internal busy flag detects recursion, not concurrent threads; provide
 * an application mutex if multiple threads can manage unit lifetimes.
 */
void tlbcode_init(void);
const tlbcode_unit_t *tlbcode_find(const char *name);
/** Borrow page_count distinct, 4 KiB aligned cached KSEG0 RDRAM pages.
 * The array and pages remain caller-owned and must stay alive until unload.
 * Pages may be noncontiguous. They must not alias resident code/data/stack or
 * any other live allocation. On false, no mapping or ownership changes occur.
 * A successful unload returns ownership of all pages to the caller.
 */
bool tlbcode_load(const tlbcode_unit_t *unit, void *const frames[], size_t count);
bool tlbcode_unload(const tlbcode_unit_t *unit);
bool tlbcode_is_loaded(const tlbcode_unit_t *unit);
/** Number of resident wired entries; remaining slots are shared by loaded units. */
unsigned tlbcode_wired_entries(void);
/** True only for readable resident/loaded code; safe for debugger dereferences. */
bool tlbcode_address_valid(uint32_t address);
/** Includes unloaded units, for symbol-table routing without dereferencing code. */
bool tlbcode_address_known(uint32_t address);
/** Diagnostic name for an unloaded unit, or NULL. Safe before init. */
const char *tlbcode_unloaded_name(uint32_t address);
#ifdef __cplusplus
}
#endif
#endif
