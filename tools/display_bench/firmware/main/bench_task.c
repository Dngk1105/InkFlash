/* Thiết kế: docs/design/display/bench.md */

/* Task bench: nhận gói từ PC, nút và timer của chuỗi; mọi việc vẽ làm ở đây (đường raw) hoặc giao cho ink_display. */
#include "bench.h"

#include "esp_app_desc.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ink_bsp.h"
#include "ink_platform.h"
#include <string.h>

static const char *TAG = "bench";

#define PACKET_QUEUE_LENGTH 2
#define EVENT_QUEUE_LENGTH  16
#define TASK_STACK_BYTES    6144
#define TASK_PRIORITY       5
#define RESTART_DELAY_MS    100 /* để ACK kịp ra USB trước khi khởi động lại */

static bench_path_t     s_path;
static QueueHandle_t    s_packets;
static QueueHandle_t    s_events;
static QueueSetHandle_t s_queue_set;

/* ---- Đọc ghi số little-endian ---- */

static uint16_t read_u16(const uint8_t *bytes) {
    return (uint16_t)(bytes[0] | (bytes[1] << 8));
}

typedef struct {
    uint8_t bytes[1536];
    size_t  length;
} writer_t;

static void put_u8(writer_t *writer, uint8_t value) {
    if (writer->length < sizeof(writer->bytes)) { writer->bytes[writer->length++] = value; }
}

static void put_u16(writer_t *writer, uint16_t value) {
    put_u8(writer, (uint8_t)value);
    put_u8(writer, (uint8_t)(value >> 8));
}

static void put_u32(writer_t *writer, uint32_t value) {
    put_u16(writer, (uint16_t)value);
    put_u16(writer, (uint16_t)(value >> 16));
}

static uint32_t read_u32(const uint8_t *bytes) {
    return (uint32_t)read_u16(bytes) | ((uint32_t)read_u16(&bytes[2]) << 16);
}

static void put_text(writer_t *writer, const char *text) {
    const size_t length = strnlen(text, 255);
    put_u8(writer, (uint8_t)length);
    for (size_t i = 0; i < length; i++) { put_u8(writer, (uint8_t)text[i]); }
}

/* ---- Khối cài đặt (bench.md, "Giao thức") ---- */

#define OPTIONS_PRESET_BYTES (1 + INK_HAL_DISPLAY_MAX_BANDS * 3 + 5)
#define OPTIONS_TAIL_BYTES   (5 + 4 * 5) /* 5 ngưỡng nợ, rồi active_ms, edge_ms, light_ms, strong_ms, power_off_ms (u32) */
#define OPTIONS_BYTES        ((INK_DISPLAY_INTENT_COUNT + INK_DISPLAY_CLEANUP_COUNT) * OPTIONS_PRESET_BYTES + OPTIONS_TAIL_BYTES)

/* Một bản ghi preset: cleanup không có debt_weight và marks_edges nên ghi 0. */
static void put_preset(writer_t *writer, const ink_display_preset_t *preset, uint8_t debt_weight, bool marks_edges) {
    put_u8(writer, preset->band_count);
    for (int band = 0; band < INK_HAL_DISPLAY_MAX_BANDS; band++) {
        put_u8(writer, (uint8_t)preset->bands[band].max_celsius);
        put_u16(writer, preset->bands[band].profile_id);
    }
    put_u8(writer, preset->config.partial_mode);
    put_u8(writer, preset->config.scan_outside_window);
    put_u8(writer, preset->config.drive_border);
    put_u8(writer, debt_weight);
    put_u8(writer, marks_edges);
}

static const uint8_t *read_preset(const uint8_t *cursor, ink_display_preset_t *preset, uint8_t *debt_weight, bool *marks_edges) {
    preset->band_count = *cursor++;
    for (int band = 0; band < INK_HAL_DISPLAY_MAX_BANDS; band++) {
        preset->bands[band] = (ink_hal_display_band_t){.max_celsius = (int8_t)cursor[0], .profile_id = read_u16(&cursor[1])};
        cursor += 3;
    }
    preset->config = (ink_hal_display_draw_config_t){
        .partial_mode = cursor[0] != 0, .scan_outside_window = cursor[1] != 0, .drive_border = cursor[2] != 0};
    if (debt_weight != nullptr) { *debt_weight = cursor[3]; }
    if (marks_edges != nullptr) { *marks_edges = cursor[4] != 0; }
    return cursor + 5;
}

/* Index bản ghi = giá trị enum: draw theo ink_display_intent_t (AUTO không dùng), cleanup theo ink_display_cleanup_t (INVALID không dùng).
 */
static void put_options(writer_t *writer, const ink_display_options_t *options) {
    for (int intent = 0; intent < INK_DISPLAY_INTENT_COUNT; intent++) {
        const ink_display_draw_preset_t *draw = &options->draw_presets[intent];
        put_preset(writer, &draw->preset, draw->debt_weight, draw->marks_edges);
    }
    for (int cleanup = 0; cleanup < INK_DISPLAY_CLEANUP_COUNT; cleanup++) {
        put_preset(writer, &options->cleanup_presets[cleanup], 0, false);
    }
    put_u8(writer, options->auto_ui_percent);
    put_u8(writer, options->debt_light);
    put_u8(writer, options->debt_page);
    put_u8(writer, options->debt_ceiling);
    put_u8(writer, options->debt_ceiling_max);
    put_u32(writer, options->active_ms);
    put_u32(writer, options->edge_ms);
    put_u32(writer, options->light_ms);
    put_u32(writer, options->strong_ms);
    put_u32(writer, options->power_off_ms);
}

static esp_err_t read_options(const uint8_t *payload, size_t length, ink_display_options_t *out_options) {
    ESP_RETURN_ON_FALSE(length >= OPTIONS_BYTES, ESP_ERR_INVALID_ARG, TAG, "options %u", (unsigned)length);
    const uint8_t *cursor = payload;
    for (int intent = 0; intent < INK_DISPLAY_INTENT_COUNT; intent++) {
        ink_display_draw_preset_t *draw = &out_options->draw_presets[intent];
        cursor                          = read_preset(cursor, &draw->preset, &draw->debt_weight, &draw->marks_edges);
    }
    for (int cleanup = 0; cleanup < INK_DISPLAY_CLEANUP_COUNT; cleanup++) {
        cursor = read_preset(cursor, &out_options->cleanup_presets[cleanup], nullptr, nullptr);
    }
    out_options->auto_ui_percent  = cursor[0];
    out_options->debt_light       = cursor[1];
    out_options->debt_page        = cursor[2];
    out_options->debt_ceiling     = cursor[3];
    out_options->debt_ceiling_max = cursor[4];
    cursor += 5;
    out_options->active_ms    = read_u32(&cursor[0]);
    out_options->edge_ms      = read_u32(&cursor[4]);
    out_options->light_ms     = read_u32(&cursor[8]);
    out_options->strong_ms    = read_u32(&cursor[12]);
    out_options->power_off_ms = read_u32(&cursor[16]);
    return ESP_OK;
}

/* ---- Gói gửi lên ---- */

static void send_info(uint8_t seq) {
    const ink_hal_display_capabilities_t *capabilities = bench_draw_capabilities();
    static writer_t                       writer; /* lớn: không để trên stack */
    writer = (writer_t){};
    put_u8(&writer, s_path);
    put_u16(&writer, capabilities->width);
    put_u16(&writer, capabilities->height);
    put_u8(&writer, capabilities->x_align);
    put_u8(&writer, BENCH_SLOT_COUNT);
    put_u8(&writer, (uint8_t)capabilities->profile_count);
    put_text(&writer, esp_app_get_description()->version);
    for (size_t i = 0; i < capabilities->profile_count; i++) {
        const ink_hal_display_profile_info_t *profile = &capabilities->profiles[i];
        put_u16(&writer, profile->id);
        put_u16(&writer, (uint16_t)profile->draw_ms);
        put_u8(&writer, profile->source);
        put_text(&writer, profile->name);
    }
    put_options(&writer, bench_draw_options());
    put_u8(&writer, ink_bsp_display_psr_second());
    put_u8(&writer, ink_bsp_display_vcom_override());
    put_u8(&writer, ink_bsp_display_power_off_frames());
    put_u8(&writer, ink_bsp_display_scan_down() ? 1 : 0);
    put_u8(&writer, ink_bsp_display_tcon());
    put_u8(&writer, ink_bsp_display_btst_c());
    put_u8(&writer, ink_bsp_display_psr_first());
    bench_link_send(BENCH_INFO, seq, writer.bytes, writer.length);
}

static void send_drawn(const bench_drawn_t *drawn) {
    writer_t writer = {};
    put_u8(&writer, drawn->slot);
    put_u8(&writer, drawn->action);
    put_u8(&writer, drawn->intent);
    put_u8(&writer, (uint8_t)drawn->temperature_celsius);
    put_u8(&writer, drawn->temperature_valid);
    put_u8(&writer, drawn->max_debt);
    put_u8(&writer, drawn->ceiling);
    put_u8(&writer, drawn->is_frame);
    put_u16(&writer, drawn->profile_id);
    put_u8(&writer, drawn->cleanup);
    put_u8(&writer, 0);
    put_u32(&writer, (uint32_t)drawn->error);
    put_u16(&writer, drawn->window.x);
    put_u16(&writer, drawn->window.y);
    put_u16(&writer, drawn->window.width);
    put_u16(&writer, drawn->window.height);
    put_u32(&writer, drawn->flagged_count);
    put_u32(&writer, drawn->input_ms);
    put_u32(&writer, drawn->queue_ms);
    put_u32(&writer, drawn->decide_ms);
    put_u32(&writer, drawn->driver.spi_us);
    put_u32(&writer, drawn->driver.panel_us);
    put_u32(&writer, drawn->driver.post_scan_us);
    put_u32(&writer, drawn->driver.power_us);
    bench_link_send(BENCH_DRAWN, 0, writer.bytes, writer.length);
}

static void send_gesture(const ink_gesture_t *gesture, uint32_t time_ms) {
    writer_t writer = {};
    put_u8(&writer, gesture->modifier);
    put_u8(&writer, gesture->button);
    put_u8(&writer, gesture->type);
    put_u8(&writer, 0);
    put_u32(&writer, time_ms);
    bench_link_send(BENCH_GESTURE, 0, writer.bytes, writer.length);
}

/* ---- Vẽ một slot theo đường đang chạy ---- */

/* Cách vẽ ở đường raw, lấy từ gói DRAW gần nhất; ý định và cờ ở đường pipeline, lấy từ gói SUBMIT hoặc PLAY. */
static bench_raw_settings_t s_raw = {.old_source = BENCH_OLD_SHOWN, .window_kind = BENCH_WINDOW_CHANGED};

static struct {
    ink_display_intent_t intent;
    uint8_t              flags;
} s_pipeline = {.intent = INK_DISPLAY_INTENT_AUTO};

/* Profile và cấu hình vẽ mặc định của đường raw: preset TEXT của driver. */
static void init_raw_settings(void) {
    const ink_hal_display_capabilities_t   *capabilities = bench_draw_capabilities();
    const ink_hal_display_preset_default_t *text         = &capabilities->draw_defaults[INK_DISPLAY_INTENT_TEXT];
    s_raw.config                                         = text->config;
    for (size_t i = 0; i < capabilities->profile_count; i++) {
        if (text->band_count > 0 && capabilities->profiles[i].id == text->bands[text->band_count - 1].profile_id) {
            s_raw.profile = (uint8_t)i;
        }
    }
}

/* ---- Tự tắt nguồn ở đường raw ---- */
/* Driver không tự tắt nguồn analog sau DRF; ở đường pipeline ink_display lo, ở đường raw task bench lo. */

#define RAW_POWER_OFF_MS 4000

static uint32_t             s_raw_power_off_ms = RAW_POWER_OFF_MS; /* 0 = chỉ tắt khi PC gửi POWER_OFF */
static ink_platform_timer_t s_power_timer;

static void on_power_timer(void *user_data) {
    (void)user_data;
    const bench_event_message_t event = {.type = BENCH_EVENT_POWER_TIMER};
    xQueueSend(s_events, &event, 0);
}

static void restart_power_timer(void) {
    ink_platform_timer_stop(s_power_timer);
    if (s_raw_power_off_ms > 0) { ink_platform_timer_start(s_power_timer, s_raw_power_off_ms); }
}

/* Tắt nguồn và báo bằng gói DRAWN (việc 7), để trang thấy lúc nguồn tắt. */
static esp_err_t raw_power_off(void) {
    ink_platform_timer_stop(s_power_timer);
    const bench_drawn_t drawn = {.action = INK_DISPLAY_ACTION_POWER_OFF, .intent = 255, .error = bench_draw_power_off()};
    send_drawn(&drawn);
    return drawn.error;
}

/* POWER_OFF: trống = tắt ngay; 4 byte = đặt thời gian tự tắt (0 = không tự tắt). */
static esp_err_t handle_power_off(const uint8_t *payload, size_t length) {
    ESP_RETURN_ON_FALSE(s_path == BENCH_PATH_RAW, ESP_ERR_INVALID_STATE, TAG, "power off: raw only");
    if (length >= 4) {
        s_raw_power_off_ms = read_u32(payload);
        restart_power_timer();
        return ESP_OK;
    }
    return raw_power_off();
}

/* Vẽ slot; đường raw gửi DRAWN ngay, đường pipeline gửi khi ink_display báo. Trả false nếu không vẽ được. */
static bool draw_slot(uint8_t slot, uint32_t input_at_ms) {
    if (s_path == BENCH_PATH_PIPELINE) {
        const esp_err_t error = bench_draw_submit(slot, input_at_ms, s_pipeline.intent, s_pipeline.flags);
        if (error != ESP_OK) { ESP_LOGE(TAG, "submit: %s", esp_err_to_name(error)); }
        return error == ESP_OK;
    }

    const uint32_t  start_ms = ink_platform_now_ms();
    bench_drawn_t   drawn;
    const esp_err_t error = bench_draw_raw(slot, &s_raw, &drawn);
    if (error != ESP_OK) { return false; }
    if (input_at_ms != 0) { drawn.input_ms = start_ms - input_at_ms; }
    send_drawn(&drawn);
    restart_power_timer();
    return true;
}

/* ---- Chuỗi khung ---- */

static struct {
    uint8_t  slots[BENCH_MAX_SEQUENCE];
    uint8_t  count;
    uint8_t  index;        /* khung vừa vẽ */
    uint16_t interval_ms;  /* từ lúc vẽ xong tới lúc giao khung sau */
    uint8_t  repeats_left; /* 0 = lặp mãi */
    bool     running;
} s_sequence;

static ink_platform_timer_t s_sequence_timer;

static void on_sequence_timer(void *user_data) {
    (void)user_data;
    const bench_event_message_t event = {.type = BENCH_EVENT_SEQUENCE_NEXT};
    xQueueSend(s_events, &event, 0);
}

static void stop_sequence(void) {
    s_sequence.running = false;
    ink_platform_timer_stop(s_sequence_timer);
}

/* Khung vừa vẽ xong: hẹn khung sau nếu chuỗi còn chạy. */
static void schedule_next_step(void) {
    if (s_sequence.running) { ink_platform_timer_start(s_sequence_timer, s_sequence.interval_ms); }
}

static void step_sequence(void) {
    if (!s_sequence.running) { return; }
    uint8_t next = (uint8_t)(s_sequence.index + 1);
    if (next >= s_sequence.count) {
        next = 0;
        if (s_sequence.repeats_left == 1) {
            stop_sequence();
            ESP_LOGI(TAG, "chuỗi xong");
            return;
        }
        if (s_sequence.repeats_left > 1) { s_sequence.repeats_left--; }
    }
    s_sequence.index = next;
    if (!draw_slot(s_sequence.slots[next], 0)) {
        stop_sequence();
        return;
    }
    if (s_path == BENCH_PATH_RAW) { schedule_next_step(); }
}

static esp_err_t start_sequence(const uint8_t *payload, size_t length) {
    ESP_RETURN_ON_FALSE(length >= 6, ESP_ERR_INVALID_ARG, TAG, "play");
    const uint8_t count = payload[5];
    ESP_RETURN_ON_FALSE(count > 0 && count <= BENCH_MAX_SEQUENCE && length >= 6u + count, ESP_ERR_INVALID_ARG, TAG, "play count");
    stop_sequence();
    s_sequence.interval_ms  = read_u16(&payload[0]);
    s_sequence.repeats_left = payload[2];
    s_pipeline.intent       = payload[3] < INK_DISPLAY_INTENT_COUNT ? payload[3] : INK_DISPLAY_INTENT_AUTO;
    s_pipeline.flags        = payload[4];
    s_sequence.count        = count;
    memcpy(s_sequence.slots, &payload[6], count);
    s_sequence.index   = (uint8_t)(count - 1); /* bước đầu nhảy về 0 */
    s_sequence.running = true;
    if (s_sequence.repeats_left > 0) { s_sequence.repeats_left++; } /* vòng đầu chưa tính */
    step_sequence();
    return ESP_OK;
}

/* Nút A: khung sau trong chuỗi, B: khung trước; dừng chuỗi đang tự chạy. */
static void step_by_button(bool forward, uint32_t input_at_ms) {
    if (s_sequence.count == 0) { return; }
    stop_sequence();
    const uint8_t count = s_sequence.count;
    s_sequence.index    = forward ? (uint8_t)((s_sequence.index + 1) % count) : (uint8_t)((s_sequence.index + count - 1) % count);
    draw_slot(s_sequence.slots[s_sequence.index], input_at_ms);
}

/* ---- Gói từ PC ---- */

static esp_err_t handle_set_path(uint8_t seq, const uint8_t *payload, size_t length);
static esp_err_t handle_button_emulate(const uint8_t *payload, size_t length);
static void      handle_button_timer_step(void);

static esp_err_t handle_draw(const uint8_t *payload, size_t length) {
    ESP_RETURN_ON_FALSE(length >= 16, ESP_ERR_INVALID_ARG, TAG, "draw");
    s_raw = (bench_raw_settings_t){
        .profile     = payload[1],
        .old_source  = (bench_old_source_t)payload[2],
        .old_slot    = payload[3],
        .config      = {.partial_mode = payload[4] != 0, .scan_outside_window = payload[5] != 0, .drive_border = payload[6] != 0},
        .window_kind = (bench_window_kind_t)payload[7],
        .window      = {read_u16(&payload[8]), read_u16(&payload[10]), read_u16(&payload[12]), read_u16(&payload[14])},
    };
    bench_drawn_t drawn;
    ESP_RETURN_ON_ERROR(bench_draw_raw(payload[0], &s_raw, &drawn), TAG, "raw");
    send_drawn(&drawn);
    restart_power_timer();
    return ESP_OK;
}

static esp_err_t handle_submit(const uint8_t *payload, size_t length) {
    ESP_RETURN_ON_FALSE(length >= 1, ESP_ERR_INVALID_ARG, TAG, "submit");
    if (length >= 3) {
        s_pipeline.intent = payload[1] < INK_DISPLAY_INTENT_COUNT ? payload[1] : INK_DISPLAY_INTENT_AUTO;
        s_pipeline.flags  = payload[2];
    }
    return draw_slot(payload[0], 0) ? ESP_OK : ESP_FAIL;
}

static esp_err_t handle_set_options(const uint8_t *payload, size_t length) {
    ink_display_options_t options = *bench_draw_options();
    ESP_RETURN_ON_ERROR(read_options(payload, length, &options), TAG, "options");
    return bench_draw_set_options(&options);
}

/* OTP qua nhiều gói OTP: offset (2) rồi dữ liệu. */
#define OTP_CHUNK_BYTES 1024

static esp_err_t handle_otp_dump(uint8_t seq) {
    ESP_RETURN_ON_FALSE(s_path == BENCH_PATH_RAW, ESP_ERR_INVALID_STATE, TAG, "otp: raw only");
    static uint8_t otp[INK_HAL_DISPLAY_MAX_BANDS * 0xE9 + 0x2A + 64];
    size_t         length = 0;
    ESP_RETURN_ON_ERROR(ink_bsp_display_read_otp(otp, sizeof(otp), &length), TAG, "otp");
    for (size_t offset = 0; offset < length; offset += OTP_CHUNK_BYTES) {
        static writer_t writer;
        writer             = (writer_t){};
        const size_t chunk = length - offset < OTP_CHUNK_BYTES ? length - offset : OTP_CHUNK_BYTES;
        put_u16(&writer, (uint16_t)offset);
        for (size_t i = 0; i < chunk; i++) { put_u8(&writer, otp[offset + i]); }
        bench_link_send(BENCH_OTP, seq, writer.bytes, writer.length);
    }
    return ESP_OK;
}

/* PSR byte 2, rồi VCOM_DC thay thế, số khung PFS, hướng quét gate (1 = quét xuống, UD = 0) byte TCON, byte 3 của BTST và PSR byte 1 thay
 * thế (0 = theo driver) nếu gói có. */
static esp_err_t handle_set_panel(const uint8_t *payload, size_t length) {
    ESP_RETURN_ON_FALSE(length >= 1, ESP_ERR_INVALID_ARG, TAG, "panel");
    ESP_RETURN_ON_ERROR(ink_bsp_display_set_psr_second(payload[0]), TAG, "psr");
    if (length >= 2) { ESP_RETURN_ON_ERROR(ink_bsp_display_set_vcom_override(payload[1]), TAG, "vcom"); }
    if (length >= 3) { ESP_RETURN_ON_ERROR(ink_bsp_display_set_power_off_frames(payload[2]), TAG, "pfs"); }
    if (length >= 4) { ESP_RETURN_ON_ERROR(ink_bsp_display_set_scan_down(payload[3] != 0), TAG, "ud"); }
    if (length >= 5) { ESP_RETURN_ON_ERROR(ink_bsp_display_set_tcon(payload[4]), TAG, "tcon"); }
    if (length >= 6) { ESP_RETURN_ON_ERROR(ink_bsp_display_set_btst_c(payload[5]), TAG, "btst"); }
    return length >= 7 ? ink_bsp_display_set_psr_first(payload[6]) : ESP_OK;
}

static void handle_read_temperature(uint8_t seq) {
    int8_t          celsius = 0;
    const esp_err_t error   = bench_draw_read_temperature(&celsius);
    writer_t        writer  = {};
    put_u8(&writer, (uint8_t)celsius);
    put_u8(&writer, 0);
    put_u16(&writer, 0);
    put_u32(&writer, (uint32_t)error);
    bench_link_send(BENCH_TEMP, seq, writer.bytes, writer.length);
}

/* ACK gửi trước khi vẽ để PC không chờ hết thời gian quét. */
static void handle_packet(const bench_packet_t *packet) {
    const uint8_t *payload = packet->payload;
    const size_t   length  = packet->length;
    esp_err_t      error   = ESP_OK;
    switch (packet->type) {
        case BENCH_HELLO:
            send_info(packet->seq);
            return;
        case BENCH_SET_PATH:
            error = handle_set_path(packet->seq, payload, length);
            break;
        case BENCH_UPLOAD:
            error = length >= BENCH_UPLOAD_HEADER
                ? bench_draw_upload(payload[0], read_u16(&payload[2]), &payload[BENCH_UPLOAD_HEADER], length - BENCH_UPLOAD_HEADER)
                : ESP_ERR_INVALID_ARG;
            break;
        case BENCH_DRAW:
            if (s_path != BENCH_PATH_RAW) {
                error = ESP_ERR_INVALID_STATE;
                break;
            }
            stop_sequence();
            bench_link_ack(packet->seq, ESP_OK);
            error = handle_draw(payload, length);
            if (error != ESP_OK) { ESP_LOGE(TAG, "draw: %s", esp_err_to_name(error)); }
            return;
        case BENCH_SUBMIT:
            stop_sequence();
            error = handle_submit(payload, length);
            break;
        case BENCH_PLAY:
            bench_link_ack(packet->seq, ESP_OK);
            error = start_sequence(payload, length);
            if (error != ESP_OK) { ESP_LOGE(TAG, "play: %s", esp_err_to_name(error)); }
            return;
        case BENCH_STOP:
            stop_sequence();
            break;
        case BENCH_POWER_OFF:
            error = handle_power_off(payload, length);
            break;
        case BENCH_PROFILE_LOAD:
            error = ink_bsp_display_load_profile(payload, length);
            break;
        case BENCH_SET_OPTIONS:
            error = handle_set_options(payload, length);
            break;
        case BENCH_READ_TEMP:
            handle_read_temperature(packet->seq);
            return;
        case BENCH_OTP_DUMP:
            error = handle_otp_dump(packet->seq);
            break;
        case BENCH_SET_PANEL:
            error = handle_set_panel(payload, length);
            break;
        case BENCH_BUTTON_EMULATE:
            error = handle_button_emulate(payload, length);
            break;
        default:
            error = ESP_ERR_NOT_SUPPORTED;
            break;
    }
    bench_link_ack(packet->seq, error);
}

/* ---- Đường vẽ giữ qua esp_restart() ---- */

#define PATH_MAGIC 0xBE7C0DE5

static RTC_NOINIT_ATTR uint32_t s_saved_magic;
static RTC_NOINIT_ATTR uint32_t s_saved_path;

bench_path_t bench_saved_path(void) {
    return s_saved_magic == PATH_MAGIC && s_saved_path == BENCH_PATH_PIPELINE ? BENCH_PATH_PIPELINE : BENCH_PATH_RAW;
}

/* Đổi đường thì khởi động lại, vì ink_display giữ driver suốt đời. */
static esp_err_t handle_set_path(uint8_t seq, const uint8_t *payload, size_t length) {
    ESP_RETURN_ON_FALSE(length >= 1 && payload[0] <= BENCH_PATH_PIPELINE, ESP_ERR_INVALID_ARG, TAG, "path");
    if (payload[0] == s_path) { return ESP_OK; }
    s_saved_magic = PATH_MAGIC;
    s_saved_path  = payload[0];
    bench_link_ack(seq, ESP_OK);
    vTaskDelay(pdMS_TO_TICKS(RESTART_DELAY_MS));
    esp_restart();
}

/* ---- Sự kiện ---- */

static uint32_t s_last_edge_ms; /* thời điểm edge nút gần nhất, làm mốc đo khi nút chuyển khung */

static void handle_input(const ink_input_message_t *message) {
    if (message->type == INK_INPUT_MESSAGE_BUTTON_EDGE) { s_last_edge_ms = message->button_edge.time_ms; }
    ink_gesture_output_t gestures;
    ink_input_handle(message, &gestures);
    for (uint8_t i = 0; i < gestures.count; i++) {
        const ink_gesture_t *gesture = &gestures.items[i];
        send_gesture(gesture, s_last_edge_ms);
        const bool plain_press = gesture->type == INK_GESTURE_TYPE_PRESS && gesture->modifier == INK_GESTURE_MODIFIER_NONE;
        if (plain_press && gesture->button == INK_GESTURE_BUTTON_A) { step_by_button(true, s_last_edge_ms); }
        if (plain_press && gesture->button == INK_GESTURE_BUTTON_B) { step_by_button(false, s_last_edge_ms); }
    }
}

static void handle_event(const bench_event_message_t *event) {
    switch (event->type) {
        case BENCH_EVENT_INPUT:
            handle_input(&event->input);
            break;
        case BENCH_EVENT_SEQUENCE_NEXT:
            step_sequence();
            break;
        case BENCH_EVENT_DRAWN: {
            bench_drawn_t drawn;
            bench_draw_from_report(&event->report, &drawn);
            send_drawn(&drawn);
            if (drawn.is_frame) { schedule_next_step(); }
            break;
        }
        case BENCH_EVENT_POWER_TIMER:
            raw_power_off();
            break;
        case BENCH_EVENT_BUTTON_TIMER:
            handle_button_timer_step();
            break;
    }
    if (uxQueueMessagesWaiting(s_events) == 0) {
        ink_gesture_output_t gestures;
        ink_input_on_queue_empty(false, &gestures);
    }
}

static void task_main(void *user_data) {
    (void)user_data;
    static bench_packet_t packet; /* lớn: không để trên stack */
    bench_event_message_t event;
    for (;;) {
        const QueueSetMemberHandle_t ready = xQueueSelectFromSet(s_queue_set, portMAX_DELAY);
        if (ready == s_packets && xQueueReceive(s_packets, &packet, 0) == pdTRUE) { handle_packet(&packet); }
        if (ready == s_events && xQueueReceive(s_events, &event, 0) == pdTRUE) { handle_event(&event); }
    }
}

/* ---- Nút ---- */

static void post_input(const ink_input_message_t *message) {
    const bench_event_message_t event = {.type = BENCH_EVENT_INPUT, .input = *message};
    xQueueSend(s_events, &event, 0);
}

static void IRAM_ATTR post_input_from_isr(const ink_input_message_t *message) {
    const bench_event_message_t event = {.type = BENCH_EVENT_INPUT, .input = *message};
    BaseType_t                  woken = pdFALSE;
    xQueueSendFromISR(s_events, &event, &woken);
    portYIELD_FROM_ISR(woken);
}

/* ---- Virtual buttons and emulation ---- */

#define DEFAULT_CLICK_MS    50
#define DEFAULT_HOLD_MS     600
#define DOUBLE_CLICK_GAP_MS 80

typedef struct {
    const ink_hal_button_device_t *physical;
    ink_hal_button_callback_t      callback;
    void                          *user_data;
    bool                           emulated_pressed;
} virtual_button_t;

static virtual_button_t            s_virtual_buttons[INK_GESTURE_BUTTON_COUNT];
static ink_hal_button_operations_t s_virtual_button_ops;
static ink_hal_button_device_t     s_virtual_button_devices[INK_GESTURE_BUTTON_COUNT];
static ink_platform_timer_t        s_button_timer;

typedef struct {
    uint8_t               button_mask;
    bench_button_action_t action;
    uint8_t               step;
    uint16_t              duration_ms;
} emulate_state_t;

static emulate_state_t s_emulate;

static void set_virtual_button_level(uint8_t button_index, bool pressed) {
    if (button_index >= INK_GESTURE_BUTTON_COUNT) { return; }
    virtual_button_t *vb = &s_virtual_buttons[button_index];
    if (vb->emulated_pressed == pressed) { return; }
    vb->emulated_pressed = pressed;
    const ink_input_message_t message = {
        .type = INK_INPUT_MESSAGE_BUTTON_EDGE,
        .button_edge = {
            .button  = ink_gesture_button_of_index(button_index),
            .level   = pressed ? INK_GESTURE_LEVEL_PRESSED : INK_GESTURE_LEVEL_RELEASED,
            .time_ms = ink_platform_now_ms(),
        },
    };
    post_input(&message);
}

static void apply_virtual_button_mask(uint8_t button_mask, bool pressed) {
    if (button_mask & INK_GESTURE_BUTTON_A) { set_virtual_button_level(0, pressed); }
    if (button_mask & INK_GESTURE_BUTTON_B) { set_virtual_button_level(1, pressed); }
}

static void on_button_timer(void *user_data) {
    (void)user_data;
    const bench_event_message_t event = {.type = BENCH_EVENT_BUTTON_TIMER};
    xQueueSend(s_events, &event, 0);
}

static void handle_button_timer_step(void) {
    switch (s_emulate.action) {
        case BENCH_BUTTON_ACTION_CLICK:
        case BENCH_BUTTON_ACTION_HOLD:
            apply_virtual_button_mask(s_emulate.button_mask, false);
            s_emulate.button_mask = 0;
            break;
        case BENCH_BUTTON_ACTION_DOUBLE_CLICK:
            if (s_emulate.step == 1) {
                apply_virtual_button_mask(s_emulate.button_mask, false);
                s_emulate.step = 2;
                ESP_ERROR_CHECK(ink_platform_timer_start(s_button_timer, DOUBLE_CLICK_GAP_MS));
            } else if (s_emulate.step == 2) {
                apply_virtual_button_mask(s_emulate.button_mask, true);
                s_emulate.step = 3;
                ESP_ERROR_CHECK(ink_platform_timer_start(s_button_timer, s_emulate.duration_ms));
            } else if (s_emulate.step == 3) {
                apply_virtual_button_mask(s_emulate.button_mask, false);
                s_emulate.button_mask = 0;
                s_emulate.step        = 0;
            }
            break;
        default:
            break;
    }
}

static esp_err_t handle_button_emulate(const uint8_t *payload, size_t length) {
    ESP_RETURN_ON_FALSE(length >= 2, ESP_ERR_INVALID_ARG, TAG, "button_emulate length");
    const uint8_t               button_mask = payload[0];
    const bench_button_action_t action      = (bench_button_action_t)payload[1];
    uint16_t                    duration_ms = 0;
    if (length >= 4) { duration_ms = read_u16(&payload[2]); }
    ESP_RETURN_ON_FALSE(button_mask >= 1 && button_mask <= INK_GESTURE_BUTTON_AB, ESP_ERR_INVALID_ARG, TAG, "button mask");

    ESP_ERROR_CHECK(ink_platform_timer_stop(s_button_timer));
    if (s_emulate.button_mask != 0) {
        apply_virtual_button_mask(s_emulate.button_mask, false);
        s_emulate.button_mask = 0;
        s_emulate.step        = 0;
    }

    switch (action) {
        case BENCH_BUTTON_ACTION_RELEASE:
            apply_virtual_button_mask(button_mask, false);
            break;
        case BENCH_BUTTON_ACTION_PRESS:
            apply_virtual_button_mask(button_mask, true);
            s_emulate.button_mask = button_mask;
            s_emulate.action      = action;
            break;
        case BENCH_BUTTON_ACTION_CLICK:
            if (duration_ms == 0) { duration_ms = DEFAULT_CLICK_MS; }
            s_emulate.button_mask = button_mask;
            s_emulate.action      = action;
            s_emulate.duration_ms = duration_ms;
            apply_virtual_button_mask(button_mask, true);
            ESP_ERROR_CHECK(ink_platform_timer_start(s_button_timer, duration_ms));
            break;
        case BENCH_BUTTON_ACTION_HOLD:
            if (duration_ms == 0) { duration_ms = DEFAULT_HOLD_MS; }
            s_emulate.button_mask = button_mask;
            s_emulate.action      = action;
            s_emulate.duration_ms = duration_ms;
            apply_virtual_button_mask(button_mask, true);
            ESP_ERROR_CHECK(ink_platform_timer_start(s_button_timer, duration_ms));
            break;
        case BENCH_BUTTON_ACTION_DOUBLE_CLICK:
            if (duration_ms == 0) { duration_ms = DEFAULT_CLICK_MS; }
            s_emulate.button_mask = button_mask;
            s_emulate.action      = action;
            s_emulate.duration_ms = duration_ms;
            s_emulate.step        = 1;
            apply_virtual_button_mask(button_mask, true);
            ESP_ERROR_CHECK(ink_platform_timer_start(s_button_timer, duration_ms));
            break;
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }
    return ESP_OK;
}

static esp_err_t virtual_button_read(void *context, bool *out_pressed) {
    if (context == nullptr || out_pressed == nullptr) { return ESP_ERR_INVALID_ARG; }
    const virtual_button_t *vb               = (const virtual_button_t *)context;
    bool                    physical_pressed = false;
    if (vb->physical != nullptr && vb->physical->operations != nullptr && vb->physical->operations->read != nullptr) {
        const esp_err_t err = vb->physical->operations->read(vb->physical->context, &physical_pressed);
        if (err != ESP_OK) { return err; }
    }
    *out_pressed = vb->emulated_pressed || physical_pressed;
    return ESP_OK;
}

static void IRAM_ATTR on_physical_button_edge(void *user_data, bool pressed) {
    const virtual_button_t *vb = (const virtual_button_t *)user_data;
    if (vb != nullptr && vb->callback != nullptr) { vb->callback(vb->user_data, pressed || vb->emulated_pressed); }
}

static esp_err_t virtual_button_set_callback(void *context, ink_hal_button_callback_t callback, void *user_data) {
    if (context == nullptr) { return ESP_ERR_INVALID_ARG; }
    virtual_button_t *vb = (virtual_button_t *)context;
    vb->callback         = callback;
    vb->user_data        = user_data;
    if (vb->physical != nullptr && vb->physical->operations != nullptr && vb->physical->operations->set_callback != nullptr) {
        return vb->physical->operations->set_callback(vb->physical->context, on_physical_button_edge, vb);
    }
    return ESP_OK;
}

/* ---- Khởi động ---- */

static esp_err_t create_queues(void) {
    s_packets   = xQueueCreate(PACKET_QUEUE_LENGTH, sizeof(bench_packet_t));
    s_events    = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(bench_event_message_t));
    s_queue_set = xQueueCreateSet(PACKET_QUEUE_LENGTH + EVENT_QUEUE_LENGTH);
    ESP_RETURN_ON_FALSE(s_packets && s_events && s_queue_set, ESP_ERR_NO_MEM, TAG, "queues");
    xQueueAddToSet(s_packets, s_queue_set);
    xQueueAddToSet(s_events, s_queue_set);
    return ESP_OK;
}

esp_err_t bench_task_start(bench_path_t path) {
    s_path = path;
    ESP_RETURN_ON_ERROR(create_queues(), TAG, "queues");
    ESP_RETURN_ON_ERROR(bench_link_start(s_packets), TAG, "link");

    const ink_hal_display_device_t *display = nullptr;
    ESP_RETURN_ON_ERROR(ink_bsp_display(&display), TAG, "display");
    ESP_RETURN_ON_ERROR(bench_draw_init(path, display, s_events), TAG, "draw");
    init_raw_settings();
    ESP_RETURN_ON_ERROR(ink_platform_timer_create(on_sequence_timer, nullptr, "sequence", &s_sequence_timer), TAG, "timer");
    ESP_RETURN_ON_ERROR(ink_platform_timer_create(on_power_timer, nullptr, "power", &s_power_timer), TAG, "timer");

    const ink_hal_button_device_t *buttons      = nullptr;
    size_t                         button_count = 0;
    ESP_RETURN_ON_ERROR(ink_bsp_buttons(&buttons, &button_count), TAG, "buttons");
    ESP_RETURN_ON_FALSE(button_count == INK_GESTURE_BUTTON_COUNT, ESP_ERR_INVALID_STATE, TAG, "buttons");

    s_virtual_button_ops = (ink_hal_button_operations_t){
        .read         = virtual_button_read,
        .set_callback = virtual_button_set_callback,
    };
    for (size_t i = 0; i < INK_GESTURE_BUTTON_COUNT; i++) {
        s_virtual_buttons[i] = (virtual_button_t){
            .physical         = &buttons[i],
            .emulated_pressed = false,
        };
        s_virtual_button_devices[i] = (ink_hal_button_device_t){
            .operations   = &s_virtual_button_ops,
            .context      = &s_virtual_buttons[i],
            .capabilities = buttons[i].capabilities,
        };
    }
    ESP_RETURN_ON_ERROR(ink_platform_timer_create(on_button_timer, nullptr, "btn_emu", &s_button_timer), TAG, "btn timer");
    ESP_RETURN_ON_ERROR(ink_input_start(s_virtual_button_devices, INK_GESTURE_BUTTON_COUNT, post_input, post_input_from_isr), TAG,
                        "input");

    const BaseType_t created = xTaskCreate(task_main, "bench", TASK_STACK_BYTES, nullptr, TASK_PRIORITY, nullptr);
    ESP_RETURN_ON_FALSE(created == pdPASS, ESP_ERR_NO_MEM, TAG, "task");
    ESP_LOGI(TAG, "đường %s", path == BENCH_PATH_RAW ? "raw" : "pipeline");
    return ESP_OK;
}
