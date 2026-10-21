#include "ink_service.h"

/*
 * Storage + SRS: deck, ReviewLog (chỉ ghi thêm, có CRC), lập lịch FSRS.
 * TODO: mount LittleFS trên phân vùng "storage", định dạng ReviewLog, FSRS.
 */
static void on_start(void)
{
}

const ink_service_def_t svc_storage_def = {
    .name = "svc_storage",
    .comp = INK_COMP_SVC_STORAGE,
    .stack_size = 4096,
    .priority = 4,
    .on_start = on_start,
};
