/* SPDX-License-Identifier: MIT */
/* Decoder-only allocation routing. It never replaces the process heap. */
#include "video_internal.h"
#include "debug.h"
#include "n64sys.h"
#include <malloc.h>
#include <stdlib.h>

static const video_allocator_t *active_allocator;

const video_allocator_t *__video_allocator_enter(const video_allocator_t *allocator)
{
    const video_allocator_t *previous = active_allocator;
    assertf(!allocator || (allocator->alloc && allocator->free), "Invalid video allocator");
    active_allocator = allocator;
    return previous;
}

void __video_allocator_leave(const video_allocator_t *previous)
{
    active_allocator = previous;
}

void *__video_alloc(size_t bytes, bool uncached)
{
    if (!active_allocator)
        return uncached ? malloc_uncached(bytes) : malloc(bytes);
    // Whole cache lines: uncached pictures must share none with other data.
    size_t size = (bytes + 15) & ~(size_t)15;
    void *memory = active_allocator->alloc(active_allocator->context, size, 16);
    if (!memory) return NULL;
    assertf(((uintptr_t)memory & 15) == 0, "Video allocator alignment");
    if (!uncached) return memory;
    // The caller may have used this memory through the cache before; a dirty
    // line written back later would overwrite RSP/uncached picture data.
    data_cache_hit_writeback_invalidate(memory, size);
    return UncachedAddr(memory);
}

void __video_free(void *memory, bool uncached)
{
    if (!memory) return;
    if (active_allocator) {
        active_allocator->free(active_allocator->context, uncached ? CachedAddr(memory) : memory);
    } else if (uncached) {
        free_uncached(memory);
    } else {
        free(memory);
    }
}
