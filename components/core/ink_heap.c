#include "ink_heap.h"
#include "esp_heap_caps.h"

void ink_heap_get(ink_heap_stats_t *out)
{
    out->internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    out->internal_min_free = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
    out->internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    out->dma_free = heap_caps_get_free_size(MALLOC_CAP_DMA);
    out->psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    out->psram_min_free = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
}
