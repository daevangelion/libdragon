/**
 * @file rdram_bank.h
 * @brief Bank- and page-aware RDRAM arena allocator for Nintendo 64.
 *
 * Goal: minimise Base-RDRAM open-row thrash by placing concurrent access
 * streams (FB, Z, PI stream, audio, RSP work) in different 1 MiB banks and
 * packing sequential consumers within few 2 KiB pages.
 *
 * Not a global malloc replacement — carve regions (BSS / top-of-RAM) into
 * rba and allocate with an access *class*. Residual general heap can stay
 * on libdragon malloc in bank 0.
 *
 * Portable C99. Host unit tests need no libdragon. On console, pair with
 * PhysicalAddr / UncachedAddr as needed.
 */
#ifndef RDRAM_BANK_H
#define RDRAM_BANK_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Geometry (N64 Base RDRAM / RI)                                             */
/* -------------------------------------------------------------------------- */

#define RBA_BANK_SHIFT   20
#define RBA_BANK_SIZE    (1u << RBA_BANK_SHIFT)   /* 1 MiB */
#define RBA_PAGE_SHIFT   11
#define RBA_PAGE_SIZE    (1u << RBA_PAGE_SHIFT)   /* 2 KiB open row */
#define RBA_MAX_BANKS    8
#define RBA_DMA_ALIGN    16
#define RBA_CACHE_ALIGN  16

/* -------------------------------------------------------------------------- */
/* Access classes                                                             */
/* -------------------------------------------------------------------------- */

typedef enum rba_class {
    RBA_CLASS_GENERAL = 0, /**< Small CPU data; pack densely */
    RBA_CLASS_FB,          /**< Color framebuffer(s) */
    RBA_CLASS_Z,           /**< Depth buffer — never share bank with FB */
    RBA_CLASS_STREAM,      /**< PI DMA destinations, stream chunks */
    RBA_CLASS_AUDIO,       /**< AI rings / wav scratch */
    RBA_CLASS_RSP,         /**< Uncached verts, DLs, matrices */
    RBA_CLASS_COUNT
} rba_class_t;

/* -------------------------------------------------------------------------- */
/* Flags                                                                      */
/* -------------------------------------------------------------------------- */

/** Force alignment up to RBA_PAGE_SIZE (always on for size >= page). */
#define RBA_F_PAGE_ALIGN     (1u << 0)
/** Prefer placement at bank base when size is large. */
#define RBA_F_BANK_ALIGN     (1u << 1)
/** Caller will use uncached path (KSEG1); still RDRAM bank-sensitive. */
#define RBA_F_UNCACHED_HINT  (1u << 2)
/** Allow allocation spanning two banks (discouraged; default off). */
#define RBA_F_SPAN_OK        (1u << 3)
/** Fail rather than place in a non-preferred bank. */
#define RBA_F_STRICT_BANK    (1u << 4)
/** Zero memory on alloc (host/debug; optional on target). */
#define RBA_F_ZERO           (1u << 5)

/* -------------------------------------------------------------------------- */
/* Requests / stats                                                           */
/* -------------------------------------------------------------------------- */

typedef struct rba_req {
    uint32_t    size;        /**< Bytes requested */
    uint32_t    align;       /**< Min alignment (0 → class default) */
    rba_class_t class;       /**< Access class / role */
    uint32_t    flags;       /**< RBA_F_* */
    uint8_t     prefer_bank; /**< 0..7 or 0xFF for policy default */
    uint8_t     avoid_mask;  /**< Bit i set → do not use bank i */
} rba_req_t;

typedef struct rba_stats {
    uint32_t alloc_ok;
    uint32_t alloc_fail;
    uint32_t free_ok;
    uint32_t bytes_live;
    uint32_t bytes_peak;
    uint32_t policy_reject;  /**< STRICT_BANK / exclusive conflicts */
    uint32_t bank_live[RBA_MAX_BANKS];
    uint32_t bank_peak[RBA_MAX_BANKS];
} rba_stats_t;

typedef struct rba_region {
    uintptr_t base;   /**< Virtual or physical base (opaque cookie) */
    uint32_t  size;
    uint8_t   bank;   /**< Owning bank id 0..7 */
} rba_region_t;

typedef struct rba_config {
    rba_region_t regions[RBA_MAX_BANKS * 2];
    unsigned     nregions;
    /** Bitmask: bank reserved exclusive for class (see rba_reserve_bank). */
    uint8_t      exclusive[RBA_CLASS_COUNT];
    /** Ordered preference list per class; 0xFF terminates. */
    uint8_t      prefer[RBA_CLASS_COUNT][RBA_MAX_BANKS + 1];
} rba_config_t;

/* -------------------------------------------------------------------------- */
/* Lifecycle                                                                  */
/* -------------------------------------------------------------------------- */

/** Fill config with 4 MiB stock policy (banks 0–3 roles). Regions empty. */
void rba_config_default_4m(rba_config_t *cfg);

/** Fill config with 8 MiB expansion policy (banks 0–7). Regions empty. */
void rba_config_default_8m(rba_config_t *cfg);

/**
 * Register a contiguous region managed by rba.
 * Bank is inferred from N64 phys mapping: `(base & 0x1FFFFFFF) >> 20`.
 * On host tests / non-identity maps, use rba_add_region_in_bank().
 * @return 0 ok, -1 full/invalid.
 */
int rba_add_region(rba_config_t *cfg, uintptr_t base, uint32_t size);

/**
 * Register a region and force its logical bank id (0..7).
 * Required when the pointer's address bits are not N64 physical.
 */
int rba_add_region_in_bank(rba_config_t *cfg, uintptr_t base, uint32_t size, uint8_t bank);

/** Reserve bank exclusively for a class (bit in exclusive[class]). */
void rba_reserve_bank(rba_config_t *cfg, rba_class_t cls, uint8_t bank);

/** Initialise global allocator from config (copies policy + builds freelists). */
int rba_init(const rba_config_t *cfg);

/** Tear down; does not free underlying BSS/carve — only internal bookkeeping. */
void rba_shutdown(void);

bool rba_is_init(void);

/* -------------------------------------------------------------------------- */
/* Alloc / free                                                               */
/* -------------------------------------------------------------------------- */

void *rba_alloc(const rba_req_t *req);
void  rba_free(void *ptr);

/** Convenience wrappers */
void *rba_alloc_class(rba_class_t cls, uint32_t size, uint32_t flags);
void *rba_alloc_fb(uint32_t size);
void *rba_alloc_z(uint32_t size);
void *rba_alloc_stream(uint32_t size);
void *rba_alloc_rsp(uint32_t size);

/* -------------------------------------------------------------------------- */
/* Queries                                                                    */
/* -------------------------------------------------------------------------- */

static inline uint8_t rba_bank_of_phys(uint32_t phys)
{
    return (uint8_t)((phys >> RBA_BANK_SHIFT) & (RBA_MAX_BANKS - 1));
}

static inline uint32_t rba_page_of_phys(uint32_t phys)
{
    return (phys >> RBA_PAGE_SHIFT) & 0x1FFu;
}

/** Phys = virt & 0x1FFFFFFF for KSEG0/1; identity otherwise. */
uint32_t rba_phys(const void *ptr);
uint8_t  rba_bank_of(const void *ptr);
bool     rba_contains(const void *ptr);
uint32_t rba_block_size(const void *ptr);

void rba_get_stats(rba_stats_t *out);
/** Bytes free in bank (sum of free blocks). */
uint32_t rba_bank_free(uint8_t bank);

/**
 * Debug: check hard isolation rules for two live pointers.
 * Returns 0 if ok, -1 if FB/Z same bank (or other conflict).
 */
int rba_check_pair(const void *a, rba_class_t ca, const void *b, rba_class_t cb);

#ifdef __cplusplus
}
#endif

#endif /* RDRAM_BANK_H */
