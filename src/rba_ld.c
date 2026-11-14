/**
 * @file rba_ld.c
 * @brief Bootstrap bank arenas using libdragon dual-ended heap + rdram_bank.
 */
#include "rba_ld.h"
#include "rdram_bank.h"

#include <libdragon.h>
#include <malloc.h>
#include <string.h>

/* Internal heap top allocator (display.c uses this for stock Z). */
extern void *sbrk_top(int incr);

#ifndef RBA_LD_FB_DEFAULT
#define RBA_LD_FB_DEFAULT   (512 * 1024)
#endif
#ifndef RBA_LD_Z_DEFAULT
#define RBA_LD_Z_DEFAULT    (320 * 240 * 2 + 4096) /* one 320x240 z + pad */
#endif
#ifndef RBA_LD_STREAM_DEFAULT
#define RBA_LD_STREAM_DEFAULT (256 * 1024)
#endif
#ifndef RBA_LD_SURFACE_FB_THRESH
#define RBA_LD_SURFACE_FB_THRESH (160 * 120 * 2) /* ~half QVGA 16bpp → FB class */
#endif

static bool g_ready;
static bool g_failed;

static uint8_t bank_of_ptr(void *p)
{
    return rba_bank_of_phys(PhysicalAddr(p));
}

static int register_slab(rba_config_t *cfg, void *ptr, uint32_t size)
{
    if (!ptr || ptr == (void *)-1 || size == 0)
        return -1;
    /* Manage the cached alias inside rba; callers get UncachedAddr. */
    void *cached = CachedAddr(ptr);
    uint8_t bank = bank_of_ptr(cached);
    return rba_add_region_in_bank(cfg, (uintptr_t)cached, size, bank);
}

int rba_ld_init(const rba_ld_config_t *cfg)
{
    if (g_ready)
        return 0;
    if (g_failed)
        return -1;

    uint32_t fb_sz = (cfg && cfg->fb_arena_bytes) ? cfg->fb_arena_bytes : RBA_LD_FB_DEFAULT;
    uint32_t z_sz = (cfg && cfg->z_arena_bytes) ? cfg->z_arena_bytes : RBA_LD_Z_DEFAULT;
    uint32_t st_sz = (cfg && cfg->stream_arena_bytes) ? cfg->stream_arena_bytes
                                                       : RBA_LD_STREAM_DEFAULT;

    /* Round arenas to page size for open-row hygiene */
    fb_sz = (fb_sz + RBA_PAGE_SIZE - 1) & ~(RBA_PAGE_SIZE - 1);
    z_sz = (z_sz + RBA_PAGE_SIZE - 1) & ~(RBA_PAGE_SIZE - 1);
    st_sz = (st_sz + RBA_PAGE_SIZE - 1) & ~(RBA_PAGE_SIZE - 1);

    rba_config_t rc;
    if (get_memory_size() >= 0x7C0000)
        rba_config_default_8m(&rc);
    else
        rba_config_default_4m(&rc);

    if (cfg && cfg->dry_run) {
        /* No carve — for host tooling only */
        g_failed = true;
        return -1;
    }

    /*
     * Carve order:
     *  1) STREAM from top (high phys → high bank, often bank 3 on 4MB)
     *  2) Z from top (below stream; still high bank if small enough)
     *  3) FB from bottom heap (low phys → bank 0/1 after BSS)
     *
     * This recreates libdragon's Z-at-top idea and adds an explicit stream
     * slab, while FB stays low — maximising chance FB≠Z bank.
     */
    void *stream_slab = sbrk_top((int)st_sz);
    void *z_slab = sbrk_top((int)z_sz);

    /* Bottom FB arena: page-aligned via memalign, then register actual bank */
    void *fb_slab = memalign(RBA_PAGE_SIZE, fb_sz);
    if (!fb_slab) {
        /* Best-effort without FB arena — still try Z/stream */
        fb_sz = 0;
    } else {
        data_cache_hit_invalidate(fb_slab, fb_sz);
    }

    int nreg = 0;
    if (stream_slab != (void *)-1 && register_slab(&rc, stream_slab, st_sz) == 0)
        nreg++;
    if (z_slab != (void *)-1 && register_slab(&rc, z_slab, z_sz) == 0)
        nreg++;
    if (fb_slab && register_slab(&rc, fb_slab, fb_sz) == 0)
        nreg++;

    if (nreg == 0 || rba_init(&rc) != 0) {
        g_failed = true;
        debugf("rba_ld_init: failed to init arenas (nreg=%d)\n", nreg);
        return -1;
    }

    g_ready = true;
    debugf("rba_ld_init: ok regions=%d mem=%d\n", nreg, get_memory_size());
    rba_ld_dump();
    return 0;
}

bool rba_ld_ready(void)
{
    return g_ready && rba_is_init();
}

static void *to_uncached(void *p)
{
    if (!p)
        return NULL;
    data_cache_hit_invalidate(p, rba_block_size(p) ? rba_block_size(p) : 16);
    return UncachedAddr(p);
}

void *rba_ld_alloc_surface(size_t size)
{
    if (!rba_ld_ready() || size == 0)
        return NULL;

    void *p = NULL;
    if (size >= RBA_LD_SURFACE_FB_THRESH) {
        p = rba_alloc_fb((uint32_t)size);
        if (!p)
            p = rba_alloc_class(RBA_CLASS_FB, (uint32_t)size, RBA_F_PAGE_ALIGN);
    } else if (size >= 4096) {
        p = rba_alloc_stream((uint32_t)size);
    } else {
        p = rba_alloc_class(RBA_CLASS_GENERAL, (uint32_t)size, 0);
    }
    return to_uncached(p);
}

void *rba_ld_alloc_z(size_t size)
{
    if (!rba_ld_ready() || size == 0)
        return NULL;
    void *p = rba_alloc_z((uint32_t)size);
    if (!p)
        p = rba_alloc_class(RBA_CLASS_Z, (uint32_t)size, RBA_F_PAGE_ALIGN);
    return to_uncached(p);
}

void *rba_ld_alloc_stream(size_t size)
{
    if (!rba_ld_ready() || size == 0)
        return NULL;
    return to_uncached(rba_alloc_stream((uint32_t)size));
}

void rba_ld_free(void *uncached_ptr)
{
    if (!uncached_ptr)
        return;
    void *p = CachedAddr(uncached_ptr);
    if (rba_contains(p))
        rba_free(p);
    else
        free_uncached(uncached_ptr);
}

void rba_ld_dump(void)
{
    if (!rba_is_init()) {
        debugf("rba_ld: not init\n");
        return;
    }
    rba_stats_t st;
    rba_get_stats(&st);
    debugf("rba_ld: live=%lu peak=%lu ok=%lu fail=%lu policy_rej=%lu\n",
           (unsigned long)st.bytes_live, (unsigned long)st.bytes_peak,
           (unsigned long)st.alloc_ok, (unsigned long)st.alloc_fail,
           (unsigned long)st.policy_reject);
    for (int b = 0; b < RBA_MAX_BANKS; b++) {
        uint32_t freeb = rba_bank_free((uint8_t)b);
        if (freeb || st.bank_live[b])
            debugf("  bank %d: live=%lu free≈%lu peak=%lu\n", b,
                   (unsigned long)st.bank_live[b], (unsigned long)freeb,
                   (unsigned long)st.bank_peak[b]);
    }
}
