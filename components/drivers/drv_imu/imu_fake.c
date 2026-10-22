#include "drv_imu_chip.h"

/* Máy nằm yên, màn hình hướng lên: z = +1000 mg. */
static ink_accel_t s_value = { 0, 0, 1000 };

void imu_fake_set(int16_t x_mg, int16_t y_mg, int16_t z_mg)
{
    s_value = (ink_accel_t){ x_mg, y_mg, z_mg };
}

static esp_err_t fake_read(void *ctx, ink_accel_t *out)
{
    *out = s_value;
    return ESP_OK;
}

static const ink_imu_ops_t s_ops = {
    .read_accel = fake_read,
};

const drv_imu_chip_t imu_fake_chip = {
    .ops = &s_ops,
};
