#pragma once

#include <stddef.h>

/* Số liệu heap cho giám sát (docs/adr/0003-cap-phat-dong.md). */
typedef struct {
    size_t internal_free;     /* RAM nội còn trống (byte) */
    size_t internal_min_free; /* mức thấp nhất từng chạm tới kể từ khi khởi động */
    size_t internal_largest;  /* khối liền lớn nhất: nhỏ dần nghĩa là heap đang phân mảnh */
    size_t dma_free;
    size_t psram_free;
    size_t psram_min_free;
} ink_heap_stats_t;

void ink_heap_get(ink_heap_stats_t *out);
