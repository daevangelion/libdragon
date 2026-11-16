#ifndef __LIBDRAGON_TLBCODE_INTERNAL_H
#define __LIBDRAGON_TLBCODE_INTERNAL_H
#include <stdbool.h>
#include <stdint.h>
/* Do not pull the pager into ordinary ROMs just for debugger checks. */
extern bool tlbcode_address_valid(uint32_t address) __attribute__((weak));
extern bool tlbcode_address_known(uint32_t address) __attribute__((weak));
extern const char *tlbcode_unloaded_name(uint32_t address) __attribute__((weak));
#endif
