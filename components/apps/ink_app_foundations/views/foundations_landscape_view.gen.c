/* Generated from Figma: SU7kpiOJEQ2DaBnUmDPBff node 57:100 (version 2424762519534052102); hash: sha256:916982c8773a7213 */
/* Design: docs/design/figma-to-code.md */
#include "foundations_landscape_view.gen.h"

static const ink_ui_text_style_t s_fixed_10_start = {
    .font = &ink_ui_font_fixed_10, .align = INK_UI_ALIGN_START, .max_lines = 0, .line_gap = 0};
static const ink_ui_text_style_t s_tiny5_16_start = {
    .font = &ink_ui_font_tiny5_16, .align = INK_UI_ALIGN_START, .max_lines = 0, .line_gap = 0};
static const ink_ui_text_style_t s_tiny5_24_start = {
    .font = &ink_ui_font_tiny5_24, .align = INK_UI_ALIGN_START, .max_lines = 0, .line_gap = 0};
static const ink_ui_text_style_t s_tiny5_8_start = {
    .font = &ink_ui_font_tiny5_8, .align = INK_UI_ALIGN_START, .max_lines = 0, .line_gap = 0};
static const ink_ui_text_style_t s_unifont_vi_16_start = {
    .font = &ink_ui_font_unifont_vi_16, .align = INK_UI_ALIGN_START, .max_lines = 0, .line_gap = 0};

void foundations_landscape_view_draw(ink_ui_canvas_t *canvas, const foundations_landscape_view_data_t *data) {
    (void)data;
    ink_ui_fill(canvas, ink_ui_canvas_bounds(canvas), 0, INK_UI_PAINT_WHITE);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 0, .y = 0, .width = 416, .height = 24}, 0, INK_UI_PAINT_BLACK);
    /* header: Auto Layout row, spacing 268 */
    const ink_ui_length_t header_lengths[2] = {
        {.fixed = 52, .flex = 0},
        {.fixed = 80, .flex = 0}
    };
    ink_ui_rect_t header_slots[2];
    ink_ui_rect_split(ink_ui_rect_inset((ink_ui_rect_t){.x = 0, .y = 0, .width = 416, .height = 24}, 8, 0), INK_UI_AXIS_ROW, 268,
                      header_lengths, 2, header_slots);
    /* FOUNDATIONS: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(header_slots[0], (ink_ui_size_t){.width = 52, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        "FOUNDATIONS", &s_tiny5_8_start, INK_UI_PAINT_WHITE);
    /* ULTRA-LIGHT 416x240: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(header_slots[1], (ink_ui_size_t){.width = 80, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_CENTER),
        "ULTRA-LIGHT 416x240", &s_tiny5_8_start, INK_UI_PAINT_WHITE);
    /* body: Auto Layout row, spacing 8 */
    const ink_ui_length_t body_lengths[3] = {
        {.fixed = 128, .flex = 0},
        {.fixed = 128, .flex = 0},
        {.fixed = 128, .flex = 0}
    };
    ink_ui_rect_t body_slots[3];
    ink_ui_rect_split(ink_ui_rect_inset((ink_ui_rect_t){.x = 0, .y = 24, .width = 416, .height = 216}, 8, 8), INK_UI_AXIS_ROW, 8,
                      body_lengths, 3, body_slots);
    /* col 1: Auto Layout column, spacing 4 */
    const ink_ui_length_t col_1_lengths[1] = {
        {.fixed = 168, .flex = 0}
    };
    ink_ui_rect_t col_1_slots[1];
    ink_ui_rect_split(
        ink_ui_rect_align(body_slots[0], (ink_ui_size_t){.width = 128, .height = 168}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_COLUMN, 4, col_1_lengths, 1, col_1_slots);
    /* 01 color: Auto Layout column, spacing 0 */
    const ink_ui_length_t n_01_color_lengths[2] = {
        {.fixed = 14,  .flex = 0},
        {.fixed = 154, .flex = 0}
    };
    ink_ui_rect_t n_01_color_slots[2];
    ink_ui_rect_split(col_1_slots[0], INK_UI_AXIS_COLUMN, 0, n_01_color_lengths, 2, n_01_color_slots);
    /* 01 COLOR: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(n_01_color_slots[0], (ink_ui_size_t){.width = 36, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "01 COLOR", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* color list: Auto Layout column, spacing 0 */
    const ink_ui_length_t color_list_lengths[11] = {
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0}
    };
    ink_ui_rect_t color_list_slots[11];
    ink_ui_rect_split(n_01_color_slots[1], INK_UI_AXIS_COLUMN, 0, color_list_lengths, 11, color_list_slots);
    /* row surface/bg: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_surface_bg_lengths[2] = {
        {.fixed = 12,  .flex = 0},
        {.fixed = 112, .flex = 0}
    };
    ink_ui_rect_t row_surface_bg_slots[2];
    ink_ui_rect_split(color_list_slots[0], INK_UI_AXIS_ROW, 4, row_surface_bg_lengths, 2, row_surface_bg_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 47, .width = 12, .height = 12}, 0, INK_UI_PAINT_WHITE);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 8, .y = 47, .width = 12, .height = 12}, 1, 0, INK_UI_PAINT_BLACK);
    /* surface/bg: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_surface_bg_slots[1], "surface/bg", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row surface/card: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_surface_card_lengths[2] = {
        {.fixed = 12,  .flex = 0},
        {.fixed = 112, .flex = 0}
    };
    ink_ui_rect_t row_surface_card_slots[2];
    ink_ui_rect_split(color_list_slots[1], INK_UI_AXIS_ROW, 4, row_surface_card_lengths, 2, row_surface_card_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 61, .width = 12, .height = 12}, 0, INK_UI_PAINT_WHITE);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 8, .y = 61, .width = 12, .height = 12}, 1, 0, INK_UI_PAINT_BLACK);
    /* surface/card: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_surface_card_slots[1], "surface/card", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row surface/selected: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_surface_selected_lengths[2] = {
        {.fixed = 12,  .flex = 0},
        {.fixed = 112, .flex = 0}
    };
    ink_ui_rect_t row_surface_selected_slots[2];
    ink_ui_rect_split(color_list_slots[2], INK_UI_AXIS_ROW, 4, row_surface_selected_lengths, 2, row_surface_selected_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 75, .width = 12, .height = 12}, 0, INK_UI_PAINT_WHITE);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 8, .y = 75, .width = 12, .height = 12}, 1, 0, INK_UI_PAINT_BLACK);
    /* surface/selected: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_surface_selected_slots[1], "surface/selected", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row text/primary: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_text_primary_lengths[2] = {
        {.fixed = 12,  .flex = 0},
        {.fixed = 112, .flex = 0}
    };
    ink_ui_rect_t row_text_primary_slots[2];
    ink_ui_rect_split(color_list_slots[3], INK_UI_AXIS_ROW, 4, row_text_primary_lengths, 2, row_text_primary_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 89, .width = 12, .height = 12}, 0, INK_UI_PAINT_BLACK);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 8, .y = 89, .width = 12, .height = 12}, 1, 0, INK_UI_PAINT_BLACK);
    /* text/primary: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_text_primary_slots[1], "text/primary", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row text/secondary: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_text_secondary_lengths[2] = {
        {.fixed = 12,  .flex = 0},
        {.fixed = 112, .flex = 0}
    };
    ink_ui_rect_t row_text_secondary_slots[2];
    ink_ui_rect_split(color_list_slots[4], INK_UI_AXIS_ROW, 4, row_text_secondary_lengths, 2, row_text_secondary_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 103, .width = 12, .height = 12}, 0, INK_UI_PAINT_BLACK);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 8, .y = 103, .width = 12, .height = 12}, 1, 0, INK_UI_PAINT_BLACK);
    /* text/secondary: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_text_secondary_slots[1], "text/secondary", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row border/subtle: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_border_subtle_lengths[2] = {
        {.fixed = 12,  .flex = 0},
        {.fixed = 112, .flex = 0}
    };
    ink_ui_rect_t row_border_subtle_slots[2];
    ink_ui_rect_split(color_list_slots[5], INK_UI_AXIS_ROW, 4, row_border_subtle_lengths, 2, row_border_subtle_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 117, .width = 12, .height = 12}, 0, INK_UI_PAINT_BLACK);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 8, .y = 117, .width = 12, .height = 12}, 1, 0, INK_UI_PAINT_BLACK);
    /* border/subtle: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_border_subtle_slots[1], "border/subtle", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row border/bold: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_border_bold_lengths[2] = {
        {.fixed = 12,  .flex = 0},
        {.fixed = 112, .flex = 0}
    };
    ink_ui_rect_t row_border_bold_slots[2];
    ink_ui_rect_split(color_list_slots[6], INK_UI_AXIS_ROW, 4, row_border_bold_lengths, 2, row_border_bold_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 131, .width = 12, .height = 12}, 0, INK_UI_PAINT_BLACK);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 8, .y = 131, .width = 12, .height = 12}, 1, 0, INK_UI_PAINT_BLACK);
    /* border/bold: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_border_bold_slots[1], "border/bold", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row mascot/body: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_mascot_body_lengths[2] = {
        {.fixed = 12,  .flex = 0},
        {.fixed = 112, .flex = 0}
    };
    ink_ui_rect_t row_mascot_body_slots[2];
    ink_ui_rect_split(color_list_slots[7], INK_UI_AXIS_ROW, 4, row_mascot_body_lengths, 2, row_mascot_body_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 145, .width = 12, .height = 12}, 0, INK_UI_PAINT_WHITE);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 8, .y = 145, .width = 12, .height = 12}, 1, 0, INK_UI_PAINT_BLACK);
    /* mascot/body: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_mascot_body_slots[1], "mascot/body", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row mascot/outline: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_mascot_outline_lengths[2] = {
        {.fixed = 12,  .flex = 0},
        {.fixed = 112, .flex = 0}
    };
    ink_ui_rect_t row_mascot_outline_slots[2];
    ink_ui_rect_split(color_list_slots[8], INK_UI_AXIS_ROW, 4, row_mascot_outline_lengths, 2, row_mascot_outline_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 159, .width = 12, .height = 12}, 0, INK_UI_PAINT_BLACK);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 8, .y = 159, .width = 12, .height = 12}, 1, 0, INK_UI_PAINT_BLACK);
    /* mascot/outline: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_mascot_outline_slots[1], "mascot/outline", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row accent/primary: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_accent_primary_lengths[2] = {
        {.fixed = 12,  .flex = 0},
        {.fixed = 112, .flex = 0}
    };
    ink_ui_rect_t row_accent_primary_slots[2];
    ink_ui_rect_split(color_list_slots[9], INK_UI_AXIS_ROW, 4, row_accent_primary_lengths, 2, row_accent_primary_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 173, .width = 12, .height = 12}, 0, INK_UI_PAINT_BLACK);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 8, .y = 173, .width = 12, .height = 12}, 1, 0, INK_UI_PAINT_BLACK);
    /* accent/primary: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_accent_primary_slots[1], "accent/primary", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row accent/danger: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_accent_danger_lengths[2] = {
        {.fixed = 12,  .flex = 0},
        {.fixed = 112, .flex = 0}
    };
    ink_ui_rect_t row_accent_danger_slots[2];
    ink_ui_rect_split(color_list_slots[10], INK_UI_AXIS_ROW, 4, row_accent_danger_lengths, 2, row_accent_danger_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 8, .y = 187, .width = 12, .height = 12}, 0, INK_UI_PAINT_BLACK);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 8, .y = 187, .width = 12, .height = 12}, 1, 0, INK_UI_PAINT_BLACK);
    /* accent/danger: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_accent_danger_slots[1], "accent/danger", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* col 2: Auto Layout column, spacing 4 */
    const ink_ui_length_t col_2_lengths[2] = {
        {.fixed = 98, .flex = 0},
        {.fixed = 54, .flex = 0}
    };
    ink_ui_rect_t col_2_slots[2];
    ink_ui_rect_split(
        ink_ui_rect_align(body_slots[1], (ink_ui_size_t){.width = 128, .height = 156}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_COLUMN, 4, col_2_lengths, 2, col_2_slots);
    /* 02 space: Auto Layout column, spacing 0 */
    const ink_ui_length_t n_02_space_lengths[2] = {
        {.fixed = 14, .flex = 0},
        {.fixed = 84, .flex = 0}
    };
    ink_ui_rect_t n_02_space_slots[2];
    ink_ui_rect_split(col_2_slots[0], INK_UI_AXIS_COLUMN, 0, n_02_space_lengths, 2, n_02_space_slots);
    /* 02 SPACE: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(n_02_space_slots[0], (ink_ui_size_t){.width = 36, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "02 SPACE", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* space list: Auto Layout column, spacing 0 */
    const ink_ui_length_t space_list_lengths[6] = {
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0}
    };
    ink_ui_rect_t space_list_slots[6];
    ink_ui_rect_split(
        ink_ui_rect_align(n_02_space_slots[1], (ink_ui_size_t){.width = 57, .height = 84}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_COLUMN, 0, space_list_lengths, 6, space_list_slots);
    /* row space/2xs: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_space_2xs_lengths[2] = {
        {.fixed = 32, .flex = 0},
        {.fixed = 19, .flex = 0}
    };
    ink_ui_rect_t row_space_2xs_slots[2];
    ink_ui_rect_split(
        ink_ui_rect_align(space_list_slots[0], (ink_ui_size_t){.width = 55, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_ROW, 4, row_space_2xs_lengths, 2, row_space_2xs_slots);
    /* cell: Auto Layout row, spacing 0 */
    const ink_ui_length_t cell_lengths[1] = {
        {.fixed = 2, .flex = 0}
    };
    ink_ui_rect_t cell_slots[1];
    ink_ui_rect_split(row_space_2xs_slots[0], INK_UI_AXIS_ROW, 0, cell_lengths, 1, cell_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 144, .y = 49, .width = 2, .height = 8}, 0, INK_UI_PAINT_BLACK);
    /* 2xs 2: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_space_2xs_slots[1], "2xs 2", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row space/xs: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_space_xs_lengths[2] = {
        {.fixed = 32, .flex = 0},
        {.fixed = 15, .flex = 0}
    };
    ink_ui_rect_t row_space_xs_slots[2];
    ink_ui_rect_split(
        ink_ui_rect_align(space_list_slots[1], (ink_ui_size_t){.width = 51, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_ROW, 4, row_space_xs_lengths, 2, row_space_xs_slots);
    /* cell: Auto Layout row, spacing 0 */
    const ink_ui_length_t cell_2_lengths[1] = {
        {.fixed = 4, .flex = 0}
    };
    ink_ui_rect_t cell_2_slots[1];
    ink_ui_rect_split(row_space_xs_slots[0], INK_UI_AXIS_ROW, 0, cell_2_lengths, 1, cell_2_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 144, .y = 63, .width = 4, .height = 8}, 0, INK_UI_PAINT_BLACK);
    /* xs 4: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_space_xs_slots[1], "xs 4", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row space/sm: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_space_sm_lengths[2] = {
        {.fixed = 32, .flex = 0},
        {.fixed = 17, .flex = 0}
    };
    ink_ui_rect_t row_space_sm_slots[2];
    ink_ui_rect_split(
        ink_ui_rect_align(space_list_slots[2], (ink_ui_size_t){.width = 53, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_ROW, 4, row_space_sm_lengths, 2, row_space_sm_slots);
    /* cell: Auto Layout row, spacing 0 */
    const ink_ui_length_t cell_3_lengths[1] = {
        {.fixed = 8, .flex = 0}
    };
    ink_ui_rect_t cell_3_slots[1];
    ink_ui_rect_split(row_space_sm_slots[0], INK_UI_AXIS_ROW, 0, cell_3_lengths, 1, cell_3_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 144, .y = 77, .width = 8, .height = 8}, 0, INK_UI_PAINT_BLACK);
    /* sm 8: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_space_sm_slots[1], "sm 8", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row space/md: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_space_md_lengths[2] = {
        {.fixed = 32, .flex = 0},
        {.fixed = 21, .flex = 0}
    };
    ink_ui_rect_t row_space_md_slots[2];
    ink_ui_rect_split(space_list_slots[3], INK_UI_AXIS_ROW, 4, row_space_md_lengths, 2, row_space_md_slots);
    /* cell: Auto Layout row, spacing 0 */
    const ink_ui_length_t cell_4_lengths[1] = {
        {.fixed = 16, .flex = 0}
    };
    ink_ui_rect_t cell_4_slots[1];
    ink_ui_rect_split(row_space_md_slots[0], INK_UI_AXIS_ROW, 0, cell_4_lengths, 1, cell_4_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 144, .y = 91, .width = 16, .height = 8}, 0, INK_UI_PAINT_BLACK);
    /* md 16: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_space_md_slots[1], "md 16", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row space/lg: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_space_lg_lengths[2] = {
        {.fixed = 32, .flex = 0},
        {.fixed = 17, .flex = 0}
    };
    ink_ui_rect_t row_space_lg_slots[2];
    ink_ui_rect_split(
        ink_ui_rect_align(space_list_slots[4], (ink_ui_size_t){.width = 53, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_ROW, 4, row_space_lg_lengths, 2, row_space_lg_slots);
    /* cell: Auto Layout row, spacing 0 */
    const ink_ui_length_t cell_5_lengths[1] = {
        {.fixed = 24, .flex = 0}
    };
    ink_ui_rect_t cell_5_slots[1];
    ink_ui_rect_split(row_space_lg_slots[0], INK_UI_AXIS_ROW, 0, cell_5_lengths, 1, cell_5_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 144, .y = 105, .width = 24, .height = 8}, 0, INK_UI_PAINT_BLACK);
    /* lg 24: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_space_lg_slots[1], "lg 24", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* row space/xl: Auto Layout row, spacing 4 */
    const ink_ui_length_t row_space_xl_lengths[2] = {
        {.fixed = 32, .flex = 0},
        {.fixed = 17, .flex = 0}
    };
    ink_ui_rect_t row_space_xl_slots[2];
    ink_ui_rect_split(
        ink_ui_rect_align(space_list_slots[5], (ink_ui_size_t){.width = 53, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_ROW, 4, row_space_xl_lengths, 2, row_space_xl_slots);
    /* cell: Auto Layout row, spacing 0 */
    const ink_ui_length_t cell_6_lengths[1] = {
        {.fixed = 32, .flex = 0}
    };
    ink_ui_rect_t cell_6_slots[1];
    ink_ui_rect_split(row_space_xl_slots[0], INK_UI_AXIS_ROW, 0, cell_6_lengths, 1, cell_6_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 144, .y = 119, .width = 32, .height = 8}, 0, INK_UI_PAINT_BLACK);
    /* xl 32: static text, tiny5_8 */
    ink_ui_text_draw(canvas, row_space_xl_slots[1], "xl 32", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* 03 radius: Auto Layout column, spacing 0 */
    const ink_ui_length_t n_03_radius_lengths[2] = {
        {.fixed = 14, .flex = 0},
        {.fixed = 40, .flex = 0}
    };
    ink_ui_rect_t n_03_radius_slots[2];
    ink_ui_rect_split(col_2_slots[1], INK_UI_AXIS_COLUMN, 0, n_03_radius_lengths, 2, n_03_radius_slots);
    /* 03 RADIUS: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(n_03_radius_slots[0], (ink_ui_size_t){.width = 38, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "03 RADIUS", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* radius row: Auto Layout row, spacing 8 */
    const ink_ui_length_t radius_row_lengths[4] = {
        {.fixed = 24, .flex = 0},
        {.fixed = 24, .flex = 0},
        {.fixed = 24, .flex = 0},
        {.fixed = 24, .flex = 0}
    };
    ink_ui_rect_t radius_row_slots[4];
    ink_ui_rect_split(
        ink_ui_rect_align(n_03_radius_slots[1], (ink_ui_size_t){.width = 120, .height = 40}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_ROW, 8, radius_row_lengths, 4, radius_row_slots);
    /* item radius/none: Auto Layout column, spacing 2 */
    const ink_ui_length_t item_radius_none_lengths[2] = {
        {.fixed = 24, .flex = 0},
        {.fixed = 14, .flex = 0}
    };
    ink_ui_rect_t item_radius_none_slots[2];
    ink_ui_rect_split(radius_row_slots[0], INK_UI_AXIS_COLUMN, 2, item_radius_none_lengths, 2, item_radius_none_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 144, .y = 148, .width = 24, .height = 24}, 0, INK_UI_PAINT_WHITE);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 144, .y = 148, .width = 24, .height = 24}, 1, 0, INK_UI_PAINT_BLACK);
    /* none: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(item_radius_none_slots[1], (ink_ui_size_t){.width = 17, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "none", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* item radius/sm: Auto Layout column, spacing 2 */
    const ink_ui_length_t item_radius_sm_lengths[2] = {
        {.fixed = 24, .flex = 0},
        {.fixed = 14, .flex = 0}
    };
    ink_ui_rect_t item_radius_sm_slots[2];
    ink_ui_rect_split(radius_row_slots[1], INK_UI_AXIS_COLUMN, 2, item_radius_sm_lengths, 2, item_radius_sm_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 176, .y = 148, .width = 24, .height = 24}, 2, INK_UI_PAINT_WHITE);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 176, .y = 148, .width = 24, .height = 24}, 1, 2, INK_UI_PAINT_BLACK);
    /* sm 2: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(item_radius_sm_slots[1], (ink_ui_size_t){.width = 17, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "sm 2", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* item radius/md: Auto Layout column, spacing 2 */
    const ink_ui_length_t item_radius_md_lengths[2] = {
        {.fixed = 24, .flex = 0},
        {.fixed = 14, .flex = 0}
    };
    ink_ui_rect_t item_radius_md_slots[2];
    ink_ui_rect_split(radius_row_slots[2], INK_UI_AXIS_COLUMN, 2, item_radius_md_lengths, 2, item_radius_md_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 208, .y = 148, .width = 24, .height = 24}, 4, INK_UI_PAINT_WHITE);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 208, .y = 148, .width = 24, .height = 24}, 1, 4, INK_UI_PAINT_BLACK);
    /* md 4: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(item_radius_md_slots[1], (ink_ui_size_t){.width = 17, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "md 4", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* item radius/full: Auto Layout column, spacing 2 */
    const ink_ui_length_t item_radius_full_lengths[2] = {
        {.fixed = 24, .flex = 0},
        {.fixed = 14, .flex = 0}
    };
    ink_ui_rect_t item_radius_full_slots[2];
    ink_ui_rect_split(radius_row_slots[3], INK_UI_AXIS_COLUMN, 2, item_radius_full_lengths, 2, item_radius_full_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 240, .y = 148, .width = 24, .height = 24}, 255, INK_UI_PAINT_WHITE);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 240, .y = 148, .width = 24, .height = 24}, 1, 255, INK_UI_PAINT_BLACK);
    /* full: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas,
        ink_ui_rect_align(item_radius_full_slots[1], (ink_ui_size_t){.width = 11, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "full", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* col 3: Auto Layout column, spacing 4 */
    const ink_ui_length_t col_3_lengths[2] = {
        {.fixed = 124, .flex = 0},
        {.fixed = 56,  .flex = 0}
    };
    ink_ui_rect_t col_3_slots[2];
    ink_ui_rect_split(
        ink_ui_rect_align(body_slots[2], (ink_ui_size_t){.width = 128, .height = 184}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_COLUMN, 4, col_3_lengths, 2, col_3_slots);
    /* 05 type: Auto Layout column, spacing 0 */
    const ink_ui_length_t n_05_type_lengths[6] = {
        {.fixed = 14, .flex = 0},
        {.fixed = 42, .flex = 0},
        {.fixed = 28, .flex = 0},
        {.fixed = 16, .flex = 0},
        {.fixed = 10, .flex = 0},
        {.fixed = 14, .flex = 0}
    };
    ink_ui_rect_t n_05_type_slots[6];
    ink_ui_rect_split(col_3_slots[0], INK_UI_AXIS_COLUMN, 0, n_05_type_lengths, 6, n_05_type_slots);
    /* 05 TYPE: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(n_05_type_slots[0], (ink_ui_size_t){.width = 31, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "05 TYPE", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* sample tiny5_24: static text, tiny5_24 */
    ink_ui_text_draw(canvas, n_05_type_slots[1], "tiny5_24", &s_tiny5_24_start, INK_UI_PAINT_BLACK);
    /* sample tiny5_16: static text, tiny5_16 */
    ink_ui_text_draw(canvas, n_05_type_slots[2], "tiny5_16", &s_tiny5_16_start, INK_UI_PAINT_BLACK);
    /* sample unifont_vi_16: static text, unifont_vi_16 */
    ink_ui_text_draw(canvas, n_05_type_slots[3], "unifont_vi_16", &s_unifont_vi_16_start, INK_UI_PAINT_BLACK);
    /* sample fixed_10: static text, fixed_10 */
    ink_ui_text_draw(canvas, n_05_type_slots[4], "fixed_10", &s_fixed_10_start, INK_UI_PAINT_BLACK);
    /* sample tiny5_8: static text, tiny5_8 */
    ink_ui_text_draw(canvas, n_05_type_slots[5], "tiny5_8", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* 04 stroke: Auto Layout column, spacing 0 */
    const ink_ui_length_t n_04_stroke_lengths[2] = {
        {.fixed = 14, .flex = 0},
        {.fixed = 42, .flex = 0}
    };
    ink_ui_rect_t n_04_stroke_slots[2];
    ink_ui_rect_split(col_3_slots[1], INK_UI_AXIS_COLUMN, 0, n_04_stroke_lengths, 2, n_04_stroke_slots);
    /* 04 STROKE: static text, tiny5_8 */
    ink_ui_text_draw(
        canvas, ink_ui_rect_align(n_04_stroke_slots[0], (ink_ui_size_t){.width = 40, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        "04 STROKE", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* stroke list: Auto Layout column, spacing 0 */
    const ink_ui_length_t stroke_list_lengths[3] = {
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0},
        {.fixed = 14, .flex = 0}
    };
    ink_ui_rect_t stroke_list_slots[3];
    ink_ui_rect_split(
        ink_ui_rect_align(n_04_stroke_slots[1], (ink_ui_size_t){.width = 52, .height = 42}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_COLUMN, 0, stroke_list_lengths, 3, stroke_list_slots);
    /* pair stroke/none: Auto Layout row, spacing 4 */
    const ink_ui_length_t pair_stroke_none_lengths[2] = {
        {.fixed = 24, .flex = 0},
        {.fixed = 23, .flex = 0}
    };
    ink_ui_rect_t pair_stroke_none_slots[2];
    ink_ui_rect_split(
        ink_ui_rect_align(stroke_list_slots[0], (ink_ui_size_t){.width = 51, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_ROW, 4, pair_stroke_none_lengths, 2, pair_stroke_none_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 280, .y = 175, .width = 24, .height = 12}, 0, INK_UI_PAINT_WHITE);
    /* none 0: static text, tiny5_8 */
    ink_ui_text_draw(canvas, pair_stroke_none_slots[1], "none 0", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* pair stroke/thin: Auto Layout row, spacing 4 */
    const ink_ui_length_t pair_stroke_thin_lengths[2] = {
        {.fixed = 24, .flex = 0},
        {.fixed = 20, .flex = 0}
    };
    ink_ui_rect_t pair_stroke_thin_slots[2];
    ink_ui_rect_split(
        ink_ui_rect_align(stroke_list_slots[1], (ink_ui_size_t){.width = 48, .height = 14}, INK_UI_ALIGN_START, INK_UI_ALIGN_START),
        INK_UI_AXIS_ROW, 4, pair_stroke_thin_lengths, 2, pair_stroke_thin_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 280, .y = 189, .width = 24, .height = 12}, 0, INK_UI_PAINT_WHITE);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 280, .y = 189, .width = 24, .height = 12}, 1, 0, INK_UI_PAINT_BLACK);
    /* thin 1: static text, tiny5_8 */
    ink_ui_text_draw(canvas, pair_stroke_thin_slots[1], "thin 1", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
    /* pair stroke/thick: Auto Layout row, spacing 4 */
    const ink_ui_length_t pair_stroke_thick_lengths[2] = {
        {.fixed = 24, .flex = 0},
        {.fixed = 24, .flex = 0}
    };
    ink_ui_rect_t pair_stroke_thick_slots[2];
    ink_ui_rect_split(stroke_list_slots[2], INK_UI_AXIS_ROW, 4, pair_stroke_thick_lengths, 2, pair_stroke_thick_slots);
    ink_ui_fill(canvas, (ink_ui_rect_t){.x = 280, .y = 203, .width = 24, .height = 12}, 0, INK_UI_PAINT_WHITE);
    ink_ui_stroke(canvas, (ink_ui_rect_t){.x = 280, .y = 203, .width = 24, .height = 12}, 2, 0, INK_UI_PAINT_BLACK);
    /* thick 2: static text, tiny5_8 */
    ink_ui_text_draw(canvas, pair_stroke_thick_slots[1], "thick 2", &s_tiny5_8_start, INK_UI_PAINT_BLACK);
}
