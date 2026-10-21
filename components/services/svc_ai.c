#include "ink_service.h"

/* AI trên máy: ESP-SR MultiNet7 kiểm tra đọc đúng từ. TODO: thêm esp-sr, nạp model từ phân vùng "model". */
const ink_service_def_t svc_ai_def = {
    .name = "svc_ai",
    .comp = INK_COMP_SVC_AI,
    .stack_size = 4096,
    .priority = 3,
};
