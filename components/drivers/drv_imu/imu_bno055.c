#include "drv_imu_chip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hw_i2c.h"
#include "ink_log.h"

/* BNO055 (Bosch, IMU có fusion). Thanh ghi theo datasheet BNO055, trang 0. */
#define REG_CHIP_ID     0x00
#define REG_ACC_DATA    0x08 /* 6 byte: X, Y, Z (int16 LE) */
#define REG_EUL_DATA    0x1A /* 6 byte: heading, roll, pitch (int16 LE, 16 LSB = 1 độ) */
#define REG_PAGE_ID     0x07
#define REG_UNIT_SEL    0x3B
#define REG_OPR_MODE    0x3D
#define REG_PWR_MODE    0x3E
#define REG_SYS_TRIGGER 0x3F

#define CHIP_ID         0xA0
#define MODE_CONFIG     0x00
#define MODE_NDOF       0x0C
#define PWR_NORMAL      0x00
#define PWR_SUSPEND     0x02
#define UNIT_ACC_MG     0x01 /* gia tốc theo mg (1 LSB = 1 mg), góc theo độ */

#define XFER_TIMEOUT_MS 100

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_dev;

static esp_err_t rd(uint8_t reg, uint8_t *buf, size_t n)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, buf, n, XFER_TIMEOUT_MS);
}

static esp_err_t wr(uint8_t reg, uint8_t val)
{
    const uint8_t pkt[2] = { reg, val };
    return i2c_master_transmit(s_dev, pkt, sizeof(pkt), XFER_TIMEOUT_MS);
}

/* Sau khi cấp nguồn/reset, chip cần tới khoảng 650 ms mới trả lời. */
static esp_err_t wait_chip_id(void)
{
    for (int i = 0; i < 100; i++) {
        uint8_t id = 0;
        if (rd(REG_CHIP_ID, &id, 1) == ESP_OK && id == CHIP_ID) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t select_device(const drv_imu_cfg_t *cfg)
{
    /* Giống demo: không chỉ định địa chỉ thì thử 0x29 rồi 0x28. */
    const uint16_t candidates[] = { cfg->i2c_addr ? cfg->i2c_addr : 0x29, 0x28 };
    const int count = cfg->i2c_addr ? 1 : 2;
    for (int i = 0; i < count; i++) {
        if (i2c_master_probe(s_bus, candidates[i], 50) != ESP_OK) {
            continue;
        }
        if (s_dev) {
            i2c_master_bus_rm_device(s_dev);
            s_dev = NULL;
        }
        const i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = candidates[i],
            .scl_speed_hz = cfg->i2c_hz ? cfg->i2c_hz : 100000,
        };
        esp_err_t err = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev);
        if (err != ESP_OK) {
            return err;
        }
        if (wait_chip_id() == ESP_OK) {
            INK_LOGI(INK_COMP_IMU, INK_LOG_EVT_INIT, "BNO055 tai 0x%02x", candidates[i]);
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t bno_init(const drv_imu_cfg_t *cfg)
{
    esp_err_t err = hw_i2c_get_bus(cfg->sda, cfg->scl, &s_bus);
    if (err != ESP_OK) {
        return err;
    }
    err = select_device(cfg);
    if (err != ESP_OK) {
        return err;
    }
    /* Reset mềm, chờ chip sống lại, rồi cấu hình ở chế độ CONFIG. */
    if ((err = wr(REG_OPR_MODE, MODE_CONFIG)) != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(25));
    wr(REG_SYS_TRIGGER, 0x20);
    vTaskDelay(pdMS_TO_TICKS(30));
    if ((err = wait_chip_id()) != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    wr(REG_PWR_MODE, PWR_NORMAL);
    wr(REG_PAGE_ID, 0);
    wr(REG_UNIT_SEL, UNIT_ACC_MG);
    wr(REG_SYS_TRIGGER, 0x80); /* dùng thạch anh ngoài, như demo (setExtCrystalUse) */
    vTaskDelay(pdMS_TO_TICKS(10));
    if ((err = wr(REG_OPR_MODE, MODE_NDOF)) != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(25));
    return ESP_OK;
}

static esp_err_t bno_self_test(void)
{
    uint8_t id = 0;
    esp_err_t err = rd(REG_CHIP_ID, &id, 1);
    if (err != ESP_OK) {
        return err;
    }
    return id == CHIP_ID ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

static esp_err_t bno_suspend(void)
{
    wr(REG_OPR_MODE, MODE_CONFIG);
    vTaskDelay(pdMS_TO_TICKS(25));
    return wr(REG_PWR_MODE, PWR_SUSPEND);
}

static esp_err_t bno_resume(void)
{
    wr(REG_PWR_MODE, PWR_NORMAL);
    vTaskDelay(pdMS_TO_TICKS(10));
    esp_err_t err = wr(REG_OPR_MODE, MODE_NDOF);
    vTaskDelay(pdMS_TO_TICKS(25));
    return err;
}

static esp_err_t bno_read_accel(void *ctx, ink_accel_t *out)
{
    uint8_t b[6];
    esp_err_t err = rd(REG_ACC_DATA, b, sizeof(b));
    if (err != ESP_OK) {
        return err;
    }
    out->x_mg = (int16_t)(b[0] | (b[1] << 8));
    out->y_mg = (int16_t)(b[2] | (b[3] << 8));
    out->z_mg = (int16_t)(b[4] | (b[5] << 8));
    return ESP_OK;
}

static esp_err_t bno_read_orientation(void *ctx, ink_orient_t *out)
{
    uint8_t b[6];
    esp_err_t err = rd(REG_EUL_DATA, b, sizeof(b));
    if (err != ESP_OK) {
        return err;
    }
    out->heading_deg = (int16_t)(b[0] | (b[1] << 8)) / 16.0f;
    out->roll_deg = (int16_t)(b[2] | (b[3] << 8)) / 16.0f;
    out->pitch_deg = (int16_t)(b[4] | (b[5] << 8)) / 16.0f;
    out->has_heading = true;
    return ESP_OK;
}

static const ink_imu_ops_t s_ops = {
    .read_accel = bno_read_accel,
    .read_orientation = bno_read_orientation,
};

const drv_imu_chip_t imu_bno055_chip = {
    .ops = &s_ops,
    .init = bno_init,
    .self_test = bno_self_test,
    .suspend = bno_suspend,
    .resume = bno_resume,
};
