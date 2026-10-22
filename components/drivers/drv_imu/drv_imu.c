#include "drv_imu.h"
#include "drv_imu_chip.h"
#include "ink_device.h"
#include "sdkconfig.h"

/* Mỗi chip xuất một biến drv_imu_chip_t; chọn biến theo Kconfig. */
#if CONFIG_INK_IMU_FAKE
extern const drv_imu_chip_t imu_fake_chip;
#define IMU_CHIP (&imu_fake_chip)
#elif CONFIG_INK_IMU_BNO055
extern const drv_imu_chip_t imu_bno055_chip;
#define IMU_CHIP (&imu_bno055_chip)
#endif

#ifdef IMU_CHIP
static drv_imu_cfg_t s_cfg;

static esp_err_t imu_init(void *ctx)
{
    if (IMU_CHIP->init) {
        esp_err_t err = IMU_CHIP->init(&s_cfg);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ink_imu_attach(IMU_CHIP->ops, NULL, (const int8_t(*)[3])s_cfg.mount);
}

static esp_err_t imu_self_test(void *ctx)
{
    return IMU_CHIP->self_test ? IMU_CHIP->self_test() : ESP_OK;
}

static esp_err_t imu_suspend(void *ctx)
{
    return IMU_CHIP->suspend ? IMU_CHIP->suspend() : ESP_OK;
}

static esp_err_t imu_resume(void *ctx)
{
    return IMU_CHIP->resume ? IMU_CHIP->resume() : ESP_OK;
}

/* Mức 1: dò và cấu hình lại chip. Mức 2 (tắt/bật nguồn) cần công tắc tải, breadboard chưa có. */
static esp_err_t imu_recover(void *ctx, int level)
{
    return level == 1 ? imu_init(ctx) : ESP_ERR_NOT_SUPPORTED;
}

static const ink_device_ops_t s_dev_ops = {
    .init = imu_init,
    .self_test = imu_self_test,
    .suspend = imu_suspend,
    .resume = imu_resume,
    .recover = imu_recover,
};
#endif

esp_err_t drv_imu_register(const drv_imu_cfg_t *cfg)
{
#ifdef IMU_CHIP
    s_cfg = *cfg;
    ink_device_t dev = {
        .name = "imu",
        .comp = INK_COMP_IMU,
        .ops = &s_dev_ops,
        .optional = true, /* máy vẫn chạy khi không có IMU, chỉ mất cử chỉ */
    };
    return ink_device_register(&dev);
#else
    (void)cfg;
    return ESP_OK; /* INK_IMU_NONE */
#endif
}
