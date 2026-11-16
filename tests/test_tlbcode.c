#if N64_TLBCODE
#include <malloc.h>

static void *tlb_test_trace[16];
static int tlb_test_trace_count;
static volatile unsigned tlb_test_timers, tlb_test_vis;
static const uint32_t tlb_test_rodata[3076]
    __attribute__((section(".rodata.tlbcode_test"), used)) = {
        [0] = 17, [1] = 41, [2] = 103, [3075] = 271,
    };

__attribute__((noinline, noclone, section(".text.tlbcode_test")))
uint32_t tlbcode_test_function(unsigned index)
{
    tlb_test_trace_count = backtrace(tlb_test_trace, 16);
    uint32_t start = TICKS_READ();
    while ((uint32_t)(TICKS_READ() - start) < TICKS_FROM_MS(50)) {}
    /* Force the read to remain in the unit instead of constant folding. */
    return ((volatile const uint32_t*)tlb_test_rodata)[index == 3 ? 3075 : index & 3];
}

static void tlb_test_timer(int overflow) { tlb_test_timers++; }
static void tlb_test_vi(void) { tlb_test_vis++; }
static int tlb_test_recovered(void) { return 0; }

void test_tlbcode(TestContext *ctx)
{
    tlbcode_init();
    const tlbcode_unit_t *unit = tlbcode_find("test");
    ASSERT(unit, "test unit missing");
    ASSERT_EQUAL_UNSIGNED(unit->page_count, 4, "test unit must span two page pairs");
    void *first = memalign(4096, 32768);
    void *second = memalign(4096, 32768);
    ASSERT(first && second, "cannot allocate unit frames");
    DEFER(free(first));
    DEFER(free(second));
    void *frames[] = {first, (uint8_t*)first + 8192,
        (uint8_t*)first + 16384, (uint8_t*)first + 24576};
    DEFER(if (tlbcode_is_loaded(unit)) tlbcode_unload(unit));

    /* Dirty stale data in each target frame must not overwrite incoming DMA. */
    memset(first, 0xA5, 32768);
    memset(second, 0x5A, 32768);
    uint8_t *queued = malloc_uncached(65536);
    uint8_t *expected = malloc_uncached(65536);
    ASSERT(queued && expected, "cannot allocate concurrent PI buffers");
    DEFER(free_uncached(queued));
    DEFER(free_uncached(expected));
    dma_read(expected, 0x10000000, 65536);
    uint64_t ticket = dma_read_async(queued, 0x10000000, 65536);
    ASSERT(!dma_finished(ticket), "PI contention request finished before unit load");
    ASSERT(tlbcode_load(unit, frames, 4), "first load failed");
    dma_wait_finished(ticket);
    ASSERT(!memcmp(queued, expected, 65536), "concurrent PI DMA corrupted");
    ASSERT(!tlbcode_load(unit, frames, 4), "double load accepted");

    timer_init();
    DEFER(timer_close());
    timer_link_t *timer = new_timer(TICKS_FROM_MS(2), TF_CONTINUOUS, tlb_test_timer);
    DEFER(delete_timer(timer));
    register_VI_handler(tlb_test_vi);
    DEFER(unregister_VI_handler(tlb_test_vi));
    tlb_test_timers = tlb_test_vis = 0;
    ASSERT_EQUAL_UNSIGNED(tlbcode_test_function(2), 103, "unit rodata incorrect");
    ASSERT(tlb_test_timers && tlb_test_vis, "interrupts stopped in mapped code");
    bool named = false, caller = false;
    void symbol(void *arg, backtrace_frame_t *frame) {
        if (!strcmp(frame->func, "tlbcode_test_function")) named = true;
        if (named && !strcmp(frame->func, "test_tlbcode")) caller = true;
    }
    backtrace_symbols_cb(tlb_test_trace, tlb_test_trace_count, 0, symbol, NULL);
    ASSERT(named && caller, "backtrace did not walk from unit into resident caller");
    debugf("tlbcode interrupts: timer=%u VI=%u; unit->resident backtrace PASS\n",
        tlb_test_timers, tlb_test_vis);
    ASSERT(tlbcode_unload(unit), "unload failed");
    memset(first, 0xCC, 32768);
    frames[0] = second;
    frames[1] = (uint8_t*)second + 8192;
    frames[2] = (uint8_t*)second + 16384;
    frames[3] = (uint8_t*)second + 24576;
    ASSERT(tlbcode_load(unit, frames, 4), "reload in different frames failed");
    ASSERT_EQUAL_UNSIGNED(tlbcode_test_function(3), 271, "reload returned stale code/data");
    ASSERT(tlbcode_unload(unit), "second unload failed");

    volatile bool caught = false, diagnostic = false;
    void handler(exception_t *ex) {
        if (ex->code != EXCEPTION_CODE_TLB_LOAD_I_MISS) {
            exception_default_handler(ex);
            return;
        }
        caught = true;
        diagnostic = !strcmp(ex->info, "code unit test not loaded");
        ex->regs->epc = (uint32_t)tlb_test_recovered;
    }
    exception_handler_t old = register_exception_handler(handler);
    DEFER(register_exception_handler(old));
    tlbcode_test_function(0);
    ASSERT(caught && diagnostic, "unloaded unit missing named TLB exception");
    ASSERT(!tlbcode_address_valid(unit->virtual_base), "unloaded unit still readable");
    memset(second, 0xDD, 32768);
    debugf("tlbcode: load/reload, rodata, symbols, timer/VI, PI queue, unloaded exception PASS\n");
}

void test_tlbcode_dso_rejected(TestContext *ctx)
{
    ASSERT(!dlopen("rom:/dl_test_syms.dso", RTLD_LOCAL), "mapped executable accepted a DSO");
    const char *error = dlerror();
    ASSERT(error && strstr(error, "TLB-mapped executables"), "missing DSO rejection diagnostic");
}

static void test_tlbcode_dso_unsupported(TestContext *ctx)
{
    SKIP("DSO success cases are unsupported for TLB-mapped executables");
}
#endif
