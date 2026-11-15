/* Design: docs/design/ui.md */
#include "ink_u8g2.h"

#include <string.h>

#define MAX_TILES       255        /* u8x8_display_info_t keeps tile counts in uint8_t; one tile = 8 px */
#define MAX_FRAME_BYTES UINT16_MAX /* u8g2_ll_hvline_horizontal_right_lsb() computes the byte offset in uint16_t */

/* ---- Virtual screen ---- */

bool ink_u8g2_screen_init(ink_u8g2_screen_t *screen, uint16_t width, uint16_t height, uint8_t quarter_turns) {
    if (screen == nullptr || width == 0 || height == 0 || quarter_turns > 3) { return false; }
    const uint32_t tile_width  = ((uint32_t)width + 7) / 8;
    const uint32_t tile_height = ((uint32_t)height + 7) / 8;
    if (tile_width > MAX_TILES || tile_height > MAX_TILES || tile_width * height > MAX_FRAME_BYTES) { return false; }

    memset(screen, 0, sizeof(*screen));
    screen->info.tile_width   = (uint8_t)tile_width;
    screen->info.tile_height  = (uint8_t)tile_height;
    screen->info.pixel_width  = width;
    screen->info.pixel_height = height;
    /* NOTE: U8g2 normally gets display_info from a display driver callback; no driver exists here, so it is set
     * directly. Drawing calls only read display_info; the bus callbacks stay NULL and are never called. */
    u8g2_GetU8x8(&screen->u8g2)->display_info = &screen->info;

    static const u8g2_cb_t *const rotations[] = {U8G2_R0, U8G2_R1, U8G2_R2, U8G2_R3};
    u8g2_SetupBuffer(&screen->u8g2, nullptr, (uint8_t)tile_height, u8g2_ll_hvline_horizontal_right_lsb, rotations[quarter_turns]);
    return true;
}
