/**
 * @file rba_ld.h
 * @brief libdragon integration for rdram_bank (bank-aware arenas).
 *
 * Call rba_ld_init() once early in main() before display_init() / heavy malloc.
 * Patched surface_alloc / display_get_zbuf route large graphics buffers through rba.
 */
#ifndef RBA_LD_H
#define RBA_LD_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rba_ld_config {
    /** Bytes reserved for color framebuffer arena (0 = default). */
    uint32_t fb_arena_bytes;
    /** Bytes reserved for Z-buffer arena (0 = default). */
    uint32_t z_arena_bytes;
    /** Bytes reserved for PI/RSP/stream arena (0 = default). */
    uint32_t stream_arena_bytes;
    /** If true, skip carve and only init empty rba (tests). */
    bool     dry_run;
} rba_ld_config_t;

/**
 * Carve bank-oriented arenas via sbrk / sbrk_top and initialise rba.
 * Safe to call multiple times (no-op after first success).
 * @return 0 on success, -1 on failure (falls back to stock allocators).
 */
int rba_ld_init(const rba_ld_config_t *cfg);

/** True if rba_ld_init succeeded and rba is ready. */
bool rba_ld_ready(void);

/**
 * Allocate uncached graphics RAM for a surface-sized buffer.
 * Large buffers (>= threshold) prefer FB class; medium prefer STREAM.
 * Returns KSEG1 uncached pointer, or NULL.
 */
void *rba_ld_alloc_surface(size_t size);

/** Free pointer from rba_ld_alloc_surface or rba_ld_alloc_z. */
void rba_ld_free(void *uncached_ptr);

/** Allocate Z-buffer (RBA_CLASS_Z), uncached. */
void *rba_ld_alloc_z(size_t size);

/** Allocate stream/DMA chunk (RBA_CLASS_STREAM), uncached. */
void *rba_ld_alloc_stream(size_t size);

/** Debug: print arena banks and free stats (debugf). */
void rba_ld_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* RBA_LD_H */
