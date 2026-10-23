#include "ink_service.h"
#include "svc_internal.h"

/*
 * Transfer: nhận deck và media từ công cụ PC (tools/inkflash.py) qua USB Serial/JTAG, gói tin có CRC,
 * gửi lại khi lỗi, ghi qua storage service. CHƯA VIẾT — giai đoạn 4.
 */
static const ink_service_def_t s_def = {
    .name = "svc_transfer",
    .comp = INK_COMP_SVC_TRANSFER,
    .stack_size = 4096,
    .priority = 3,
};

esp_err_t svc_transfer_start(void)
{
    return ink_service_start(&s_def, NULL);
}
