#include <libdragon.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition, ...) do { \
    if (!(condition)) { fprintf(stderr, __VA_ARGS__); abort(); } \
} while (0)

/* The only caller is main, exactly once, before the init unit is unloaded. */
__attribute__((noinline, noclone, section(".text.tlbcode_example_init")))
static void example_init(void)
{
    CHECK(dfs_init(DFS_DEFAULT_LOCATION) == DFS_ESUCCESS, "DFS init failed");
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    profile_init(NULL);
}

#if N64_TLBCODE
static void *frames[62];
static void *load_unit(const tlbcode_unit_t *unit)
{
    CHECK(unit && unit->page_count <= 62, "Invalid example unit");
    void *region = memalign(4096, unit->page_count * 4096);
    CHECK(region, "Not enough memory for %s", unit->name);
    for (unsigned i = 0; i < unit->page_count; i++)
        frames[i] = (uint8_t*)region + i * 4096;
    memset(region, 0xA5, unit->page_count * 4096);
    uint32_t start = TICKS_READ();
    CHECK(tlbcode_load(unit, frames, unit->page_count), "Cannot load %s", unit->name);
    uint32_t ticks = TICKS_READ() - start;
    debugf("TLBCODE load %s: %lu pages, %lu entries, %lu ticks, %llu us; wired=%u\n",
        unit->name, unit->page_count, (unit->page_count + 1) / 2,
        (unsigned long)ticks, (unsigned long long)TIMER_MICROS(ticks), tlbcode_wired_entries());
    return region;
}
#endif

static unsigned shown;
static void on_frame(void *ctx, int frame, float seconds, fmv_control_t *ctrl)
{
    shown++;
    debugf("TLBCODE decoded/shown frame %d time %.3f\n", frame, seconds);
    if (shown >= 24) ctrl->stop(ctrl);
}

int main(void)
{
    debug_init_emulog();
#if N64_TLBCODE
    tlbcode_init();
    const tlbcode_unit_t *init = tlbcode_find("init");
    void *region = load_unit(init);
#endif
    example_init();
#if N64_TLBCODE
    CHECK(tlbcode_unload(init), "Cannot unload init");
    free(region);
    const tlbcode_unit_t *fmv = tlbcode_find("fmv");
    region = load_unit(fmv);
#endif
    video_register_codec(&h264_codec);
    void __h264_profile_init(void);
    __h264_profile_init();
    fmv_play("rom:/clip.h264", &(fmv_parms_t){
        .disable_audio = true,
        .disable_display_init = true,
        .osd_callback = on_frame,
    });
    rspq_wait();
#if N64_TLBCODE
    CHECK(tlbcode_unload(fmv), "Cannot unload fmv");
    size_t bytes = fmv->page_count * 4096;
    uintptr_t released = (uintptr_t)region;
    free(region);
    region = memalign(4096, bytes);
    CHECK(region && (uintptr_t)region == released, "Allocator did not return the released unit frames");
    volatile uint32_t *words = region;
    for (unsigned i = 0; i < bytes / 4; i++) words[i] = i ^ 0xC001C0DE;
    for (unsigned i = 0; i < bytes / 4; i++)
        CHECK(words[i] == (i ^ 0xC001C0DE), "Reclaimed frame mismatch");
    debugf("TLBCODE unloaded fmv; allocated/touched %u reclaimed bytes; %u frames shown PASS\n",
        (unsigned)bytes, shown);
    free(region);
#else
    debugf("resident FMV: %u frames shown PASS\n", shown);
#endif
    for (;;) {}
}
