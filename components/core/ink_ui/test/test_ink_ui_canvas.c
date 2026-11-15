/* Design: docs/design/ui.md */

#include "ink_ui.h"
#include "unity.h"

#include <stdio.h>
#include <string.h>

#define PANEL_WIDTH   240
#define PANEL_HEIGHT  416
#define ROW_BYTES     (PANEL_WIDTH / 8) /* 30 bytes */
#define FRAME_BYTES   (ROW_BYTES * PANEL_HEIGHT)
#define GUARD_BYTES   32
#define GUARD_PATTERN 0xAA

static const ink_display_frame_info_t s_info_k1 = {
    .width  = PANEL_WIDTH,
    .height = PANEL_HEIGHT,
    .format = INK_DISPLAY_PIXEL_FORMAT_K1,
};

void setUp(void) {
}

void tearDown(void) {
}

/* Purpose: canvas init must validate non-NULL pointers, non-zero dimensions, and supported pixel format (K1 only).
 * Does: passes NULL pointers, zero width/height, and an invalid pixel format to ink_ui_canvas_init(), then checks that valid parameters
 * return INK_UI_OK. */
static void test_init_validation(void) {
    ink_ui_canvas_t canvas;
    TEST_ASSERT_EQUAL(INK_UI_ERROR_ARGUMENT, ink_ui_canvas_init(nullptr, &s_info_k1, INK_UI_ROTATION_0));
    TEST_ASSERT_EQUAL(INK_UI_ERROR_ARGUMENT, ink_ui_canvas_init(&canvas, nullptr, INK_UI_ROTATION_0));

    const ink_display_frame_info_t zero_w = {.width = 0, .height = 100, .format = INK_DISPLAY_PIXEL_FORMAT_K1};
    TEST_ASSERT_EQUAL(INK_UI_ERROR_ARGUMENT, ink_ui_canvas_init(&canvas, &zero_w, INK_UI_ROTATION_0));

    const ink_display_frame_info_t zero_h = {.width = 100, .height = 0, .format = INK_DISPLAY_PIXEL_FORMAT_K1};
    TEST_ASSERT_EQUAL(INK_UI_ERROR_ARGUMENT, ink_ui_canvas_init(&canvas, &zero_h, INK_UI_ROTATION_0));

    const ink_display_frame_info_t bad_fmt = {.width = 100, .height = 100, .format = INK_DISPLAY_PIXEL_FORMAT_INVALID};
    TEST_ASSERT_EQUAL(INK_UI_ERROR_FORMAT, ink_ui_canvas_init(&canvas, &bad_fmt, INK_UI_ROTATION_0));

    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&canvas, &s_info_k1, INK_UI_ROTATION_0));
}

/* Purpose: drawing operations on an initialized canvas without a bound frame must be safe and do nothing.
 * Does: calls fill, stroke, line, and bitmap on an unbound canvas and verifies no crash or illegal memory access occurs. */
static void test_unbound_draw_safe(void) {
    ink_ui_canvas_t canvas;
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&canvas, &s_info_k1, INK_UI_ROTATION_0));

    /* None of these should crash or touch memory */
    ink_ui_fill(&canvas, (ink_ui_rect_t){0, 0, 10, 10}, 0, INK_UI_PAINT_BLACK);
    ink_ui_stroke(&canvas, (ink_ui_rect_t){0, 0, 10, 10}, 1, 0, INK_UI_PAINT_BLACK);
    ink_ui_line(&canvas, 0, 0, 10, 10, 1, INK_UI_PAINT_BLACK);

    const uint8_t         data[] = {0xFF};
    const ink_ui_bitmap_t bitmap = {.width = 8, .height = 1, .format = INK_UI_BITMAP_FORMAT_MASK1, .data = data};
    ink_ui_bitmap(&canvas, 0, 0, &bitmap, INK_UI_PAINT_BLACK);
}

/* Purpose: filling a rectangle with black paint must clear the corresponding bits in the K1 frame.
 * Does: fills an 8 × 1 px rect with INK_UI_PAINT_BLACK on a white frame and verifies byte 0 is 0x00 while all other bytes stay 0xFF. */
static void test_example2_fill_black(void) {
    ink_ui_canvas_t canvas;
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&canvas, &s_info_k1, INK_UI_ROTATION_0));

    uint8_t frame[FRAME_BYTES];
    memset(frame, 0xFF, sizeof(frame));
    ink_ui_canvas_bind(&canvas, frame);

    ink_ui_fill(&canvas, (ink_ui_rect_t){0, 0, 8, 1}, 0, INK_UI_PAINT_BLACK);

    TEST_ASSERT_EQUAL_HEX8(0x00, frame[0]);
    for (size_t i = 1; i < sizeof(frame); i++) { TEST_ASSERT_EQUAL_HEX8(0xFF, frame[i]); }
}

/* Purpose: a 90° clockwise canvas must swap bounds and map (0, 0) to the panel's top-right corner.
 * Does: initializes a 90° rotated canvas, checks swapped bounds, draws pixel (0, 0) in black, and verifies native byte 29 bit 0 is cleared
 * to 0xFE. */
static void test_example3_rotation_90(void) {
    ink_ui_canvas_t canvas;
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&canvas, &s_info_k1, INK_UI_ROTATION_90));

    const ink_ui_rect_t bounds = ink_ui_canvas_bounds(&canvas);
    TEST_ASSERT_EQUAL_INT16(PANEL_HEIGHT, bounds.width);
    TEST_ASSERT_EQUAL_INT16(PANEL_WIDTH, bounds.height);

    uint8_t frame[FRAME_BYTES];
    memset(frame, 0xFF, sizeof(frame));
    ink_ui_canvas_bind(&canvas, frame);

    /* Draw pixel (0, 0) of rotated canvas in black */
    ink_ui_fill(&canvas, (ink_ui_rect_t){0, 0, 1, 1}, 0, INK_UI_PAINT_BLACK);

    /* Native (x=239, y=0) is lowest bit of byte 29 -> 0xFF with bit 0 cleared = 0xFE */
    TEST_ASSERT_EQUAL_HEX8(0xFE, frame[29]);
    for (size_t i = 0; i < sizeof(frame); i++) {
        if (i != 29) { TEST_ASSERT_EQUAL_HEX8(0xFF, frame[i]); }
    }
}

/* Purpose: XOR paint mode must be self-inverting so applying it twice restores the original frame content.
 * Does: fills a 40 × 40 px rect with INK_UI_PAINT_XOR on a pattern buffer, checks that the buffer changed, fills the same rect again, and
 * verifies it matches the original. */
static void test_example4_xor_twice(void) {
    ink_ui_canvas_t canvas;
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&canvas, &s_info_k1, INK_UI_ROTATION_0));

    uint8_t frame[FRAME_BYTES];
    for (size_t i = 0; i < sizeof(frame); i++) { frame[i] = (uint8_t)(i ^ 0x5A); }
    uint8_t original[FRAME_BYTES];
    memcpy(original, frame, sizeof(frame));

    ink_ui_canvas_bind(&canvas, frame);
    const ink_ui_rect_t rect = {.x = 10, .y = 10, .width = 40, .height = 40};

    ink_ui_fill(&canvas, rect, 0, INK_UI_PAINT_XOR);
    TEST_ASSERT_NOT_EQUAL(0, memcmp(frame, original, sizeof(frame)));

    ink_ui_fill(&canvas, rect, 0, INK_UI_PAINT_XOR);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(original, frame, sizeof(frame));
}

/* Purpose: canvas rotation must map pixel (0, 0) to the correct panel corner for all four rotations.
 * Does: draws pixel (0, 0) in black on rotations 0°, 90°, 180°, and 270°, checking target bytes and bit masks (0x7F, 0xFE, 0xFE, 0x7F). */
static void test_pixel_all_four_rotations(void) {
    const ink_ui_rotation_t rots[4] = {INK_UI_ROTATION_0, INK_UI_ROTATION_90, INK_UI_ROTATION_180, INK_UI_ROTATION_270};
    /* Native target:
     * R0: (0, 0) -> byte 0, bit 7 (0x80 cleared -> 0x7F)
     * R90: (239, 0) -> byte 29, bit 0 (0x01 cleared -> 0xFE)
     * R180: (239, 415) -> byte 415 * 30 + 29 = 12479, bit 0 (0x01 cleared -> 0xFE)
     * R270: (0, 415) -> byte 415 * 30 + 0 = 12450, bit 7 (0x80 cleared -> 0x7F)
     */
    const size_t  target_bytes[4] = {0, 29, 415 * ROW_BYTES + 29, 415 * ROW_BYTES};
    const uint8_t target_vals[4]  = {0x7F, 0xFE, 0xFE, 0x7F};

    for (int r = 0; r < 4; r++) {
        ink_ui_canvas_t canvas;
        TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&canvas, &s_info_k1, rots[r]));
        uint8_t frame[FRAME_BYTES];
        memset(frame, 0xFF, sizeof(frame));
        ink_ui_canvas_bind(&canvas, frame);

        ink_ui_fill(&canvas, (ink_ui_rect_t){0, 0, 1, 1}, 0, INK_UI_PAINT_BLACK);
        TEST_ASSERT_EQUAL_HEX8(target_vals[r], frame[target_bytes[r]]);
    }
}

/* Purpose: draw operations must be confined to the clip window, and an empty clip rect must reset clipping to canvas bounds.
 * Does: sets clip to [8, 0, 8, 1], fills [0, 0, 24, 1] with black, verifies only byte 1 changes, resets clip with an empty rect, and
 * verifies filling [0, 0, 8, 1] clears byte 0. */
static void test_clip_window(void) {
    ink_ui_canvas_t canvas;
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&canvas, &s_info_k1, INK_UI_ROTATION_0));

    uint8_t frame[FRAME_BYTES];
    memset(frame, 0xFF, sizeof(frame));
    ink_ui_canvas_bind(&canvas, frame);

    /* Clip to [8, 0, 8, 1], then fill [0, 0, 24, 1] */
    ink_ui_canvas_set_clip(&canvas, (ink_ui_rect_t){8, 0, 8, 1});
    ink_ui_fill(&canvas, (ink_ui_rect_t){0, 0, 24, 1}, 0, INK_UI_PAINT_BLACK);

    /* Byte 0 (pixels 0..7) and byte 2 (pixels 16..23) must remain 0xFF; byte 1 (pixels 8..15) is 0x00 */
    TEST_ASSERT_EQUAL_HEX8(0xFF, frame[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, frame[1]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, frame[2]);

    /* Reset clip with empty rect -> whole canvas */
    ink_ui_canvas_set_clip(&canvas, (ink_ui_rect_t){0, 0, 0, 0});
    ink_ui_fill(&canvas, (ink_ui_rect_t){0, 0, 8, 1}, 0, INK_UI_PAINT_BLACK);
    TEST_ASSERT_EQUAL_HEX8(0x00, frame[0]);
}

/* Purpose: shapes with negative coordinates or sizes exceeding canvas bounds must clip cleanly without buffer overflow.
 * Does: surrounds a frame with guard bytes, draws fills, strokes, and lines crossing boundaries, and asserts guard bytes remain intact. */
static void test_overflow_and_negative_coords_guarded(void) {
    ink_ui_canvas_t canvas;
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&canvas, &s_info_k1, INK_UI_ROTATION_0));

    uint8_t buffer[GUARD_BYTES + FRAME_BYTES + GUARD_BYTES];
    memset(buffer, GUARD_PATTERN, sizeof(buffer));
    uint8_t *frame = buffer + GUARD_BYTES;

    ink_ui_canvas_bind(&canvas, frame);

    /* Draw with negative coordinates and oversized shapes */
    ink_ui_fill(&canvas, (ink_ui_rect_t){-50, -50, 100, 100}, 0, INK_UI_PAINT_BLACK);
    ink_ui_fill(&canvas, (ink_ui_rect_t){200, 400, 100, 100}, 0, INK_UI_PAINT_BLACK);
    ink_ui_stroke(&canvas, (ink_ui_rect_t){-20, -20, 80, 80}, 3, 5, INK_UI_PAINT_BLACK);
    ink_ui_line(&canvas, -50, -50, 500, 500, 4, INK_UI_PAINT_BLACK);

    /* Check guards are intact */
    for (size_t i = 0; i < GUARD_BYTES; i++) { TEST_ASSERT_EQUAL_HEX8(GUARD_PATTERN, buffer[i]); }
    for (size_t i = GUARD_BYTES + FRAME_BYTES; i < sizeof(buffer); i++) { TEST_ASSERT_EQUAL_HEX8(GUARD_PATTERN, buffer[i]); }
}

/* Purpose: gray fills must produce Bayer dither patterns whose white pixel proportion matches the brightness level.
 * Does: fills 32 × 32 px areas with gray levels 0, 255, and 128, verifying population counts of 0, 1024, and exactly 512 white pixels. */
static void test_dither_bayer(void) {
    ink_ui_canvas_t canvas;
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&canvas, &s_info_k1, INK_UI_ROTATION_0));

    uint8_t frame[FRAME_BYTES];
    ink_ui_canvas_bind(&canvas, frame);

    /* Dither 0: all black (0) */
    memset(frame, 0xFF, sizeof(frame));
    ink_ui_fill(&canvas, (ink_ui_rect_t){0, 0, 32, 32}, 0, INK_UI_PAINT_GRAY(0));
    /* For solid black, ink_ui_fill uses fill_solid -> 0 */
    uint32_t ones_0 = 0;
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 4; x++) { ones_0 += (uint32_t)__builtin_popcount(frame[y * ROW_BYTES + x]); }
    }
    TEST_ASSERT_EQUAL_UINT32(0, ones_0);

    /* Dither 255: all white (1) */
    memset(frame, 0x00, sizeof(frame));
    ink_ui_fill(&canvas, (ink_ui_rect_t){0, 0, 32, 32}, 0, INK_UI_PAINT_GRAY(255));
    uint32_t ones_255 = 0;
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 4; x++) { ones_255 += (uint32_t)__builtin_popcount(frame[y * ROW_BYTES + x]); }
    }
    TEST_ASSERT_EQUAL_UINT32(32 * 32, ones_255);

    /* Dither 128: exactly half pixels white (32 * 32 / 2 = 512) */
    memset(frame, 0x00, sizeof(frame));
    ink_ui_fill(&canvas, (ink_ui_rect_t){0, 0, 32, 32}, 0, INK_UI_PAINT_GRAY(128));
    uint32_t ones_128 = 0;
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 4; x++) { ones_128 += (uint32_t)__builtin_popcount(frame[y * ROW_BYTES + x]); }
    }
    TEST_ASSERT_EQUAL_UINT32(512, ones_128);
}

/* Purpose: a 1 px outline stroke must leave inner pixels untouched, and XOR stroke twice must restore original pixels.
 * Does: strokes a 24 × 10 px rect on a white frame, verifies inner byte 1 stays 0xFF while borders change, then strokes XOR twice with
 * rounded corners and verifies restoration. */
static void test_stroke_1px_outline_and_xor(void) {
    ink_ui_canvas_t canvas;
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&canvas, &s_info_k1, INK_UI_ROTATION_0));

    uint8_t frame[FRAME_BYTES];
    memset(frame, 0xFF, sizeof(frame));
    ink_ui_canvas_bind(&canvas, frame);

    const ink_ui_rect_t rect = {.x = 0, .y = 2, .width = 24, .height = 10};
    ink_ui_stroke(&canvas, rect, 1, 0, INK_UI_PAINT_BLACK);

    /* Center inside rect [8..15, y=3..10] corresponds to byte 1; must be untouched (0xFF) */
    for (int y = 3; y <= 10; y++) { TEST_ASSERT_EQUAL_HEX8(0xFF, frame[y * ROW_BYTES + 1]); }
    /* Edge bytes at y=3..10 have border pixels */
    TEST_ASSERT_EQUAL_HEX8(0x7F, frame[3 * ROW_BYTES + 0]);
    TEST_ASSERT_EQUAL_HEX8(0xFE, frame[3 * ROW_BYTES + 2]);

    /* Stroke XOR twice restores original */
    uint8_t original[FRAME_BYTES];
    memcpy(original, frame, sizeof(frame));
    ink_ui_stroke(&canvas, rect, 1, 3, INK_UI_PAINT_XOR);
    TEST_ASSERT_NOT_EQUAL(0, memcmp(frame, original, sizeof(frame)));
    ink_ui_stroke(&canvas, rect, 1, 3, INK_UI_PAINT_XOR);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(original, frame, sizeof(frame));
}

/* Purpose: MASK1 bitmaps must draw only set bits with the specified paint and leave zero bits untouched.
 * Does: draws an 8 × 1 px bitmap with bit pattern 0xA0 in black paint onto a 0xFF frame and verifies byte 0 becomes 0x5F. */
static void test_bitmap_mask1(void) {
    ink_ui_canvas_t canvas;
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&canvas, &s_info_k1, INK_UI_ROTATION_0));

    uint8_t frame[FRAME_BYTES];
    memset(frame, 0xFF, sizeof(frame));
    ink_ui_canvas_bind(&canvas, frame);

    /* 8x1 bitmap with pattern 10100000 = 0xA0 */
    const uint8_t         bm_data[] = {0xA0};
    const ink_ui_bitmap_t bitmap    = {.width = 8, .height = 1, .format = INK_UI_BITMAP_FORMAT_MASK1, .data = bm_data};

    ink_ui_bitmap(&canvas, 0, 0, &bitmap, INK_UI_PAINT_BLACK);

    /* In black paint, bit 1 is cleared to 0, bit 0 is left untouched (1)
     * Initial byte 0 was 11111111. 1s in mask: bit 7 and bit 5.
     * Result: 01011111 = 0x5F */
    TEST_ASSERT_EQUAL_HEX8(0x5F, frame[0]);
}

/* Purpose: rendering strokes, dither ramps, and text together must produce a valid visual frame exportable as PBM.
 * Does: draws a border, 5-level dither ramp, and 3-line Vietnamese text to a canvas, then exports the inverted frame bytes to a P4 PBM
 * file. */
static void test_preview_pbm(void) {
    ink_ui_canvas_t canvas;
    TEST_ASSERT_EQUAL(INK_UI_OK, ink_ui_canvas_init(&canvas, &s_info_k1, INK_UI_ROTATION_0));

    uint8_t frame[FRAME_BYTES];
    memset(frame, 0xFF, sizeof(frame)); /* White */
    ink_ui_canvas_bind(&canvas, frame);

    /* Frame border */
    ink_ui_stroke(&canvas, (ink_ui_rect_t){4, 4, PANEL_WIDTH - 8, PANEL_HEIGHT - 8}, 2, 4, INK_UI_PAINT_BLACK);

    /* Dither ramp */
    for (int i = 0; i < 5; i++) {
        const ink_ui_rect_t r = {.x = (int16_t)(16 + i * 40), .y = 30, .width = 36, .height = 30};
        ink_ui_fill(&canvas, r, 0, INK_UI_PAINT_GRAY(i * 64 > 255 ? 255 : i * 64));
        ink_ui_stroke(&canvas, r, 1, 0, INK_UI_PAINT_BLACK);
    }

    /* Vietnamese text */
    const ink_ui_text_style_t style = {
        .font      = &ink_ui_font_unifont_vi_16,
        .align     = INK_UI_ALIGN_CENTER,
        .max_lines = 4,
        .line_gap  = 4,
    };
    ink_ui_text_draw(&canvas, (ink_ui_rect_t){10, 80, PANEL_WIDTH - 20, 100}, "InkFlash Máy học\ntiếng Việt thông minh\nGNU Unifont 16px",
                     &style, INK_UI_PAINT_BLACK);

    /* Write PBM P4 file */
    FILE *fp = fopen("ink_ui_preview.pbm", "wb");
    TEST_ASSERT_NOT_NULL(fp);
    fprintf(fp, "P4\n%d %d\n", PANEL_WIDTH, PANEL_HEIGHT);
    for (size_t i = 0; i < FRAME_BYTES; i++) {
        const uint8_t pbm_byte = (uint8_t)~frame[i]; /* PBM: 1 is black, K1: 1 is white */
        fputc(pbm_byte, fp);
    }
    fclose(fp);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_validation);
    RUN_TEST(test_unbound_draw_safe);
    RUN_TEST(test_example2_fill_black);
    RUN_TEST(test_example3_rotation_90);
    RUN_TEST(test_example4_xor_twice);
    RUN_TEST(test_pixel_all_four_rotations);
    RUN_TEST(test_clip_window);
    RUN_TEST(test_overflow_and_negative_coords_guarded);
    RUN_TEST(test_dither_bayer);
    RUN_TEST(test_stroke_1px_outline_and_xor);
    RUN_TEST(test_bitmap_mask1);
    RUN_TEST(test_preview_pbm);
    return UNITY_END();
}
