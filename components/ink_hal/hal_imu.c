#include <math.h>
#include <string.h>
#include "ink_hal_imu.h"
#include "ink_health.h"

static const ink_imu_ops_t *s_ops;
static void *s_ctx;
static int8_t s_mount[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

esp_err_t ink_imu_attach(const ink_imu_ops_t *ops, void *ctx, const int8_t mount[3][3])
{
    if (!ops || !ops->read_accel) {
        return ESP_ERR_INVALID_ARG;
    }
    if (mount) {
        memcpy(s_mount, mount, sizeof(s_mount));
    }
    s_ctx = ctx;
    s_ops = ops;
    return ESP_OK;
}

bool ink_imu_present(void)
{
    return s_ops != NULL;
}

esp_err_t ink_imu_read_accel(ink_accel_t *out)
{
    if (!s_ops) {
        return ESP_ERR_NOT_FOUND;
    }
    ink_accel_t raw;
    esp_err_t err = s_ops->read_accel(s_ctx, &raw);
    ink_health_report(INK_COMP_IMU, err == ESP_OK);
    if (err != ESP_OK) {
        return err;
    }
    const int32_t in[3] = { raw.x_mg, raw.y_mg, raw.z_mg };
    int16_t *o[3] = { &out->x_mg, &out->y_mg, &out->z_mg };
    for (int r = 0; r < 3; r++) {
        *o[r] = (int16_t)(s_mount[r][0] * in[0] + s_mount[r][1] * in[1] + s_mount[r][2] * in[2]);
    }
    return ESP_OK;
}

esp_err_t ink_imu_read_orientation(ink_orient_t *out)
{
    if (!s_ops) {
        return ESP_ERR_NOT_FOUND;
    }
    if (s_ops->read_orientation) {
        esp_err_t err = s_ops->read_orientation(s_ctx, out);
        ink_health_report(INK_COMP_IMU, err == ESP_OK);
        return err;
    }
    ink_accel_t a;
    esp_err_t err = ink_imu_read_accel(&a);
    if (err != ESP_OK) {
        return err;
    }
    const float deg = 57.29578f;
    out->roll_deg = atan2f((float)a.y_mg, (float)a.z_mg) * deg;
    out->pitch_deg = atan2f(-(float)a.x_mg, sqrtf((float)a.y_mg * a.y_mg + (float)a.z_mg * a.z_mg)) * deg;
    out->heading_deg = 0;
    out->has_heading = false;
    return ESP_OK;
}
