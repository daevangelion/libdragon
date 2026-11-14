/**
 * @file rdram_bank.c
 * @brief Bank-aware freelist allocator (segregated size classes per bank).
 */

#include "rdram_bank.h"

#include <string.h>

/* -------------------------------------------------------------------------- */
/* Internal block header (precedes user payload)                              */
/* -------------------------------------------------------------------------- */

typedef struct rba_hdr {
    uint32_t magic;
    uint32_t size;       /* user payload size */
    uint32_t total;      /* header + payload + pad */
    uint8_t  bank;
    uint8_t  class;
    uint8_t  free;
    uint8_t  _pad;
    struct rba_hdr *next; /* freelist link when free; else NULL */
} rba_hdr_t;

#define RBA_MAGIC      0x52424148u /* 'RBAH' */
#define RBA_HDR_SIZE   ((uint32_t)sizeof(rba_hdr_t))
#define RBA_MIN_TOTAL  (RBA_HDR_SIZE + 16u)

/* Size classes: page, 4K, 8K, 16K, 32K, 64K, overflow */
#define RBA_NCLASS 7
static const uint32_t rba_size_class[RBA_NCLASS] = {
    2048, 4096, 8192, 16384, 32768, 65536, 0xFFFFFFFFu
};

typedef struct {
    uintptr_t base;
    uint32_t  size;
    uint8_t   bank;
    rba_hdr_t *free_lists[RBA_NCLASS];
} rba_bank_state_t;

static struct {
    int              ready;
    rba_config_t     cfg;
    rba_bank_state_t banks[RBA_MAX_BANKS];
    uint8_t          bank_present; /* bitmask */
    rba_stats_t      stats;
} g_rba;

/* -------------------------------------------------------------------------- */
/* Helpers                                                                    */
/* -------------------------------------------------------------------------- */

static uint32_t align_up(uint32_t v, uint32_t a)
{
    if (a <= 1)
        return v;
    return (v + a - 1u) & ~(a - 1u);
}

static int size_class_index(uint32_t total)
{
    for (int i = 0; i < RBA_NCLASS; i++) {
        if (total <= rba_size_class[i])
            return i;
    }
    return RBA_NCLASS - 1;
}

static uint32_t default_align(rba_class_t cls, uint32_t size, uint32_t flags)
{
    uint32_t a = RBA_CACHE_ALIGN;
    if (cls == RBA_CLASS_STREAM || cls == RBA_CLASS_AUDIO || cls == RBA_CLASS_RSP)
        a = RBA_DMA_ALIGN;
    if (cls == RBA_CLASS_FB || cls == RBA_CLASS_Z || (flags & RBA_F_PAGE_ALIGN) ||
        size >= RBA_PAGE_SIZE)
        a = RBA_PAGE_SIZE;
    if (flags & RBA_F_BANK_ALIGN)
        a = RBA_BANK_SIZE;
    return a;
}

static rba_hdr_t *ptr_to_hdr(void *ptr)
{
    if (!ptr)
        return NULL;
    return (rba_hdr_t *)((uint8_t *)ptr - RBA_HDR_SIZE);
}

static void *hdr_to_ptr(rba_hdr_t *h)
{
    return (uint8_t *)h + RBA_HDR_SIZE;
}

static void freelist_push(rba_bank_state_t *bs, rba_hdr_t *h)
{
    int sc = size_class_index(h->total);
    h->free = 1;
    h->next = bs->free_lists[sc];
    bs->free_lists[sc] = h;
}

static rba_hdr_t *freelist_take(rba_bank_state_t *bs, uint32_t need_total)
{
    int sc0 = size_class_index(need_total);
    for (int sc = sc0; sc < RBA_NCLASS; sc++) {
        rba_hdr_t *prev = NULL;
        rba_hdr_t *h = bs->free_lists[sc];
        while (h) {
            if (h->total >= need_total) {
                if (prev)
                    prev->next = h->next;
                else
                    bs->free_lists[sc] = h->next;
                h->next = NULL;
                h->free = 0;
                return h;
            }
            prev = h;
            h = h->next;
        }
    }
    return NULL;
}

static int region_bank(uintptr_t base)
{
    /* Use low 29 bits as phys-like */
    uint32_t phys = (uint32_t)(base & 0x1FFFFFFFu);
    return (int)rba_bank_of_phys(phys);
}

/* -------------------------------------------------------------------------- */
/* Config defaults                                                            */
/* -------------------------------------------------------------------------- */

void rba_config_default_4m(rba_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    /* prefer lists: 0xFF terminator */
    /* GENERAL → 0 */
    cfg->prefer[RBA_CLASS_GENERAL][0] = 0;
    cfg->prefer[RBA_CLASS_GENERAL][1] = 0xFF;
    /* FB → 1 */
    cfg->prefer[RBA_CLASS_FB][0] = 1;
    cfg->prefer[RBA_CLASS_FB][1] = 0xFF;
    /* Z → 2 */
    cfg->prefer[RBA_CLASS_Z][0] = 2;
    cfg->prefer[RBA_CLASS_Z][1] = 0xFF;
    /* STREAM/AUDIO/RSP → 3, then 0 */
    cfg->prefer[RBA_CLASS_STREAM][0] = 3;
    cfg->prefer[RBA_CLASS_STREAM][1] = 0;
    cfg->prefer[RBA_CLASS_STREAM][2] = 0xFF;
    cfg->prefer[RBA_CLASS_AUDIO][0] = 3;
    cfg->prefer[RBA_CLASS_AUDIO][1] = 0;
    cfg->prefer[RBA_CLASS_AUDIO][2] = 0xFF;
    cfg->prefer[RBA_CLASS_RSP][0] = 3;
    cfg->prefer[RBA_CLASS_RSP][1] = 0;
    cfg->prefer[RBA_CLASS_RSP][2] = 0xFF;

    rba_reserve_bank(cfg, RBA_CLASS_FB, 1);
    rba_reserve_bank(cfg, RBA_CLASS_Z, 2);
}

void rba_config_default_8m(rba_config_t *cfg)
{
    rba_config_default_4m(cfg);
    /* FB can also use 4; Z use 5; stream 6,7 */
    cfg->prefer[RBA_CLASS_FB][0] = 1;
    cfg->prefer[RBA_CLASS_FB][1] = 4;
    cfg->prefer[RBA_CLASS_FB][2] = 0xFF;
    cfg->prefer[RBA_CLASS_Z][0] = 2;
    cfg->prefer[RBA_CLASS_Z][1] = 5;
    cfg->prefer[RBA_CLASS_Z][2] = 0xFF;
    cfg->prefer[RBA_CLASS_STREAM][0] = 3;
    cfg->prefer[RBA_CLASS_STREAM][1] = 6;
    cfg->prefer[RBA_CLASS_STREAM][2] = 7;
    cfg->prefer[RBA_CLASS_STREAM][3] = 0;
    cfg->prefer[RBA_CLASS_STREAM][4] = 0xFF;
    cfg->prefer[RBA_CLASS_AUDIO][0] = 3;
    cfg->prefer[RBA_CLASS_AUDIO][1] = 6;
    cfg->prefer[RBA_CLASS_AUDIO][2] = 0xFF;
    cfg->prefer[RBA_CLASS_RSP][0] = 3;
    cfg->prefer[RBA_CLASS_RSP][1] = 7;
    cfg->prefer[RBA_CLASS_RSP][2] = 0;
    cfg->prefer[RBA_CLASS_RSP][3] = 0xFF;
    rba_reserve_bank(cfg, RBA_CLASS_FB, 4);
    rba_reserve_bank(cfg, RBA_CLASS_Z, 5);
}

void rba_reserve_bank(rba_config_t *cfg, rba_class_t cls, uint8_t bank)
{
    if (!cfg || cls >= RBA_CLASS_COUNT || bank >= RBA_MAX_BANKS)
        return;
    cfg->exclusive[cls] |= (uint8_t)(1u << bank);
}

int rba_add_region_in_bank(rba_config_t *cfg, uintptr_t base, uint32_t size, uint8_t bank)
{
    if (!cfg || size < RBA_MIN_TOTAL || bank >= RBA_MAX_BANKS)
        return -1;
    if (cfg->nregions >= (unsigned)(sizeof(cfg->regions) / sizeof(cfg->regions[0])))
        return -1;
    rba_region_t *r = &cfg->regions[cfg->nregions++];
    r->base = base;
    r->size = size;
    r->bank = bank;
    return 0;
}

int rba_add_region(rba_config_t *cfg, uintptr_t base, uint32_t size)
{
    return rba_add_region_in_bank(cfg, base, size, (uint8_t)region_bank(base));
}

/* -------------------------------------------------------------------------- */
/* Init                                                                       */
/* -------------------------------------------------------------------------- */

int rba_init(const rba_config_t *cfg)
{
    if (!cfg || cfg->nregions == 0)
        return -1;
    memset(&g_rba, 0, sizeof(g_rba));
    g_rba.cfg = *cfg;

    for (unsigned i = 0; i < cfg->nregions; i++) {
        const rba_region_t *r = &cfg->regions[i];
        uint8_t bank = r->bank;
        if (bank >= RBA_MAX_BANKS)
            continue;
        rba_bank_state_t *bs = &g_rba.banks[bank];
        /* One region per bank for simplicity; merge if same bank already set */
        if (bs->size == 0) {
            bs->base = r->base;
            bs->size = r->size;
            bs->bank = bank;
        } else {
            /* Only accept contiguous extension */
            uintptr_t end = bs->base + bs->size;
            if (r->base == end)
                bs->size += r->size;
            else if (r->base + r->size == bs->base) {
                bs->base = r->base;
                bs->size += r->size;
            } else {
                /* skip non-contiguous second region in same bank */
                continue;
            }
        }
        g_rba.bank_present |= (uint8_t)(1u << bank);
    }

    for (int b = 0; b < RBA_MAX_BANKS; b++) {
        if (!(g_rba.bank_present & (1u << b)))
            continue;
        rba_bank_state_t *bs = &g_rba.banks[b];
        /* Align base of freelist header inside region */
        uintptr_t start = align_up((uint32_t)(bs->base & 0xFFFFFFFFu), RBA_CACHE_ALIGN);
        /* If base is large pointer, align via mask on address */
        start = (bs->base + (RBA_CACHE_ALIGN - 1u)) & ~(uintptr_t)(RBA_CACHE_ALIGN - 1u);
        if (start < bs->base)
            start = bs->base;
        uint32_t off = (uint32_t)(start - bs->base);
        if (off >= bs->size || bs->size - off < RBA_MIN_TOTAL)
            continue;
        rba_hdr_t *h = (rba_hdr_t *)(void *)start;
        h->magic = RBA_MAGIC;
        h->total = bs->size - off;
        h->size = h->total - RBA_HDR_SIZE;
        h->bank = (uint8_t)b;
        h->class = 0;
        h->free = 1;
        h->next = NULL;
        freelist_push(bs, h);
    }

    g_rba.ready = 1;
    return 0;
}

void rba_shutdown(void)
{
    memset(&g_rba, 0, sizeof(g_rba));
}

bool rba_is_init(void)
{
    return g_rba.ready != 0;
}

/* -------------------------------------------------------------------------- */
/* Bank selection                                                             */
/* -------------------------------------------------------------------------- */

static int bank_forbidden_for_class(rba_class_t cls, uint8_t bank)
{
    /* Exclusive banks owned by *other* classes */
    for (int c = 0; c < RBA_CLASS_COUNT; c++) {
        if (c == (int)cls)
            continue;
        if (g_rba.cfg.exclusive[c] & (1u << bank)) {
            /* FB exclusive blocks everyone else; Z exclusive blocks everyone else */
            if (c == RBA_CLASS_FB || c == RBA_CLASS_Z)
                return 1;
        }
    }
    /* Hard rule: FB cannot use Z's exclusive banks and vice versa */
    if (cls == RBA_CLASS_FB && (g_rba.cfg.exclusive[RBA_CLASS_Z] & (1u << bank)))
        return 1;
    if (cls == RBA_CLASS_Z && (g_rba.cfg.exclusive[RBA_CLASS_FB] & (1u << bank)))
        return 1;
    return 0;
}

static int build_candidates(const rba_req_t *req, uint8_t *out, int max_out)
{
    int n = 0;
    uint8_t seen = 0;

    /* 1) explicit prefer_bank */
    if (req->prefer_bank != 0xFF && req->prefer_bank < RBA_MAX_BANKS) {
        uint8_t b = req->prefer_bank;
        if ((g_rba.bank_present & (1u << b)) && !(req->avoid_mask & (1u << b)) &&
            !bank_forbidden_for_class(req->class, b)) {
            out[n++] = b;
            seen |= (uint8_t)(1u << b);
        }
    }

    /* 2) class preference list */
    const uint8_t *pref = g_rba.cfg.prefer[req->class];
    for (int i = 0; i < RBA_MAX_BANKS + 1 && pref[i] != 0xFF && n < max_out; i++) {
        uint8_t b = pref[i];
        if (b >= RBA_MAX_BANKS)
            break;
        if (seen & (1u << b))
            continue;
        if (!(g_rba.bank_present & (1u << b)))
            continue;
        if (req->avoid_mask & (1u << b))
            continue;
        if (bank_forbidden_for_class(req->class, b))
            continue;
        out[n++] = b;
        seen |= (uint8_t)(1u << b);
    }

    if (req->flags & RBA_F_STRICT_BANK)
        return n;

    /* 3) any remaining present banks */
    for (uint8_t b = 0; b < RBA_MAX_BANKS && n < max_out; b++) {
        if (seen & (1u << b))
            continue;
        if (!(g_rba.bank_present & (1u << b)))
            continue;
        if (req->avoid_mask & (1u << b))
            continue;
        if (bank_forbidden_for_class(req->class, b))
            continue;
        out[n++] = b;
        seen |= (uint8_t)(1u << b);
    }
    return n;
}

/* -------------------------------------------------------------------------- */
/* Alloc / free                                                               */
/* -------------------------------------------------------------------------- */

void *rba_alloc(const rba_req_t *req)
{
    if (!g_rba.ready || !req || req->size == 0 || req->class >= RBA_CLASS_COUNT) {
        if (g_rba.ready)
            g_rba.stats.alloc_fail++;
        return NULL;
    }

    uint32_t align = req->align ? req->align : default_align(req->class, req->size, req->flags);
    if (align < 4)
        align = 4;

    /*
     * Layout inside a free block:
     *   [ optional free lead ][ hdr ][ user payload ][ optional free tail ]
     * User pointer must be `align`-aligned. We over-ask by (align-1) so a
     * suitable place for hdr always exists, then split lead/tail back.
     */
    uint32_t need_total = RBA_HDR_SIZE + (align - 1u) + req->size;
    need_total = align_up(need_total, 8);

    uint8_t cands[RBA_MAX_BANKS];
    int nc = build_candidates(req, cands, RBA_MAX_BANKS);
    if (nc == 0) {
        g_rba.stats.alloc_fail++;
        g_rba.stats.policy_reject++;
        return NULL;
    }

    for (int i = 0; i < nc; i++) {
        uint8_t bank = cands[i];
        rba_bank_state_t *bs = &g_rba.banks[bank];
        rba_hdr_t *blk = freelist_take(bs, need_total);
        if (!blk)
            continue;

        uint8_t *blk_start = (uint8_t *)blk;
        uint8_t *blk_end = blk_start + blk->total;

        /* Earliest payload address with room for hdr before it */
        uintptr_t min_user = (uintptr_t)(blk_start + RBA_HDR_SIZE);
        uintptr_t user_u = (min_user + (align - 1u)) & ~(uintptr_t)(align - 1u);
        uint8_t *user = (uint8_t *)user_u;
        rba_hdr_t *h = (rba_hdr_t *)(user - RBA_HDR_SIZE);

        if (user + req->size > blk_end) {
            freelist_push(bs, blk);
            continue;
        }

        /* Leading free fragment [blk_start, h) */
        uint32_t lead = (uint32_t)((uint8_t *)h - blk_start);
        if (lead > 0) {
            if (lead >= RBA_MIN_TOTAL) {
                blk->magic = RBA_MAGIC;
                blk->total = lead;
                blk->size = lead - RBA_HDR_SIZE;
                blk->bank = bank;
                blk->free = 1;
                freelist_push(bs, blk);
            } else {
                /* Absorb tiny lead into allocated block (shift h back) — rare */
                h = blk;
                user = (uint8_t *)hdr_to_ptr(h);
                /* May violate align; reject and retry next bank/block */
                if (((uintptr_t)user & (align - 1u)) != 0) {
                    freelist_push(bs, blk);
                    continue;
                }
            }
        }

        uint8_t *payload_end = user + req->size;
        uint32_t used = (uint32_t)(payload_end - (uint8_t *)h);
        used = align_up(used, 8);
        if ((uint8_t *)h + used > blk_end)
            used = (uint32_t)(blk_end - (uint8_t *)h);

        /* Trailing free fragment */
        uint32_t tail = (uint32_t)(blk_end - ((uint8_t *)h + used));
        if (tail >= RBA_MIN_TOTAL) {
            rba_hdr_t *th = (rba_hdr_t *)((uint8_t *)h + used);
            th->magic = RBA_MAGIC;
            th->total = tail;
            th->size = tail - RBA_HDR_SIZE;
            th->bank = bank;
            th->class = 0;
            th->free = 1;
            th->next = NULL;
            freelist_push(bs, th);
        } else {
            used += tail; /* absorb tiny tail */
        }

        h->magic = RBA_MAGIC;
        h->size = req->size;
        h->total = used;
        h->bank = bank;
        h->class = (uint8_t)req->class;
        h->free = 0;
        h->next = NULL;

        void *ptr = hdr_to_ptr(h);
        if (req->flags & RBA_F_ZERO)
            memset(ptr, 0, req->size);

        g_rba.stats.alloc_ok++;
        g_rba.stats.bytes_live += req->size;
        if (g_rba.stats.bytes_live > g_rba.stats.bytes_peak)
            g_rba.stats.bytes_peak = g_rba.stats.bytes_live;
        g_rba.stats.bank_live[bank] += req->size;
        if (g_rba.stats.bank_live[bank] > g_rba.stats.bank_peak[bank])
            g_rba.stats.bank_peak[bank] = g_rba.stats.bank_live[bank];
        return ptr;
    }

    g_rba.stats.alloc_fail++;
    return NULL;
}

void rba_free(void *ptr)
{
    if (!g_rba.ready || !ptr)
        return;
    rba_hdr_t *h = ptr_to_hdr(ptr);
    if (h->magic != RBA_MAGIC || h->free)
        return;
    uint8_t bank = h->bank;
    if (bank >= RBA_MAX_BANKS)
        return;
    uint32_t sz = h->size;
    h->free = 1;
    freelist_push(&g_rba.banks[bank], h);
    g_rba.stats.free_ok++;
    if (g_rba.stats.bytes_live >= sz)
        g_rba.stats.bytes_live -= sz;
    if (g_rba.stats.bank_live[bank] >= sz)
        g_rba.stats.bank_live[bank] -= sz;
}

void *rba_alloc_class(rba_class_t cls, uint32_t size, uint32_t flags)
{
    rba_req_t r = {
        .size = size,
        .align = 0,
        .class = cls,
        .flags = flags,
        .prefer_bank = 0xFF,
        .avoid_mask = 0,
    };
    return rba_alloc(&r);
}

void *rba_alloc_fb(uint32_t size)
{
    return rba_alloc_class(RBA_CLASS_FB, size, RBA_F_PAGE_ALIGN | RBA_F_STRICT_BANK);
}

void *rba_alloc_z(uint32_t size)
{
    return rba_alloc_class(RBA_CLASS_Z, size, RBA_F_PAGE_ALIGN | RBA_F_STRICT_BANK);
}

void *rba_alloc_stream(uint32_t size)
{
    return rba_alloc_class(RBA_CLASS_STREAM, size, RBA_F_UNCACHED_HINT);
}

void *rba_alloc_rsp(uint32_t size)
{
    return rba_alloc_class(RBA_CLASS_RSP, size, RBA_F_UNCACHED_HINT);
}

/* -------------------------------------------------------------------------- */
/* Queries                                                                    */
/* -------------------------------------------------------------------------- */

uint32_t rba_phys(const void *ptr)
{
    uintptr_t v = (uintptr_t)ptr;
    if (v >= 0x80000000u && v < 0xC0000000u)
        return (uint32_t)(v & 0x1FFFFFFFu);
    return (uint32_t)(v & 0xFFFFFFFFu);
}

uint8_t rba_bank_of(const void *ptr)
{
    if (ptr && g_rba.ready) {
        rba_hdr_t *h = ptr_to_hdr((void *)ptr);
        if (h->magic == RBA_MAGIC)
            return h->bank;
    }
    return rba_bank_of_phys(rba_phys(ptr));
}

bool rba_contains(const void *ptr)
{
    if (!ptr || !g_rba.ready)
        return false;
    rba_hdr_t *h = ptr_to_hdr((void *)ptr);
    return h->magic == RBA_MAGIC && !h->free;
}

uint32_t rba_block_size(const void *ptr)
{
    if (!rba_contains(ptr))
        return 0;
    return ptr_to_hdr((void *)ptr)->size;
}

void rba_get_stats(rba_stats_t *out)
{
    if (!out)
        return;
    *out = g_rba.stats;
}

uint32_t rba_bank_free(uint8_t bank)
{
    if (!g_rba.ready || bank >= RBA_MAX_BANKS)
        return 0;
    uint32_t sum = 0;
    rba_bank_state_t *bs = &g_rba.banks[bank];
    for (int sc = 0; sc < RBA_NCLASS; sc++) {
        for (rba_hdr_t *h = bs->free_lists[sc]; h; h = h->next)
            sum += h->size;
    }
    return sum;
}

int rba_check_pair(const void *a, rba_class_t ca, const void *b, rba_class_t cb)
{
    if (!a || !b)
        return 0;
    uint8_t ba = rba_bank_of(a);
    uint8_t bb = rba_bank_of(b);
    if (ba != bb)
        return 0;
    if ((ca == RBA_CLASS_FB && cb == RBA_CLASS_Z) ||
        (ca == RBA_CLASS_Z && cb == RBA_CLASS_FB))
        return -1;
    if ((ca == RBA_CLASS_STREAM || ca == RBA_CLASS_AUDIO) &&
        (cb == RBA_CLASS_FB || cb == RBA_CLASS_Z))
        return -1;
    if ((cb == RBA_CLASS_STREAM || cb == RBA_CLASS_AUDIO) &&
        (ca == RBA_CLASS_FB || ca == RBA_CLASS_Z))
        return -1;
    return 0;
}
