/* Thiết kế: docs/design/esp-idf-api.md */

/*
 * Dựa theo: examples/peripherals/adc/oneshot_read. Không dùng driver/adc.h, esp_adc_cal.h (đã xoá trong v6.1).
 * Đo pin phải dùng ADC1, vì ADC2 tranh với WiFi.
 */
#include "ink_periph_examples.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_check.h"

static const char *TAG = "periph_adc";

esp_err_t ink_periph_examples_adc_read_mv(adc_channel_t channel, int *out_mv)
{
    adc_oneshot_unit_handle_t unit;
    const adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_cfg, &unit), TAG, "adc_oneshot_new_unit");

    adc_cali_handle_t cali = NULL;
    const adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    esp_err_t err = adc_oneshot_config_channel(unit, channel, &chan_cfg);
    if (err == ESP_OK) {
        const adc_cali_curve_fitting_config_t cali_cfg = {
            .unit_id = ADC_UNIT_1,
            .chan = channel,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        err = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali);
    }
    int raw = 0;
    if (err == ESP_OK) {
        err = adc_oneshot_read(unit, channel, &raw);
    }
    if (err == ESP_OK) {
        err = adc_cali_raw_to_voltage(cali, raw, out_mv);
    }
    if (cali != NULL) {
        adc_cali_delete_scheme_curve_fitting(cali);
    }
    adc_oneshot_del_unit(unit);
    return err;
}
