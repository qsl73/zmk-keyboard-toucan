#include <zephyr/kernel.h>
#include <stdio.h>
#include "battery.h"
#include "../assets/custom_fonts.h"

/*
 * One half's battery: "G 87%" on top, a gauge underneath and a bolt when the
 * half is plugged in. The left column starts at x = 2, the right one at x = 76.
 */
void draw_battery_gauge(lv_obj_t *canvas, lv_coord_t x, char side, uint8_t level, bool known,
                        bool charging) {
    char text[12];

    if (known) {
        snprintf(text, sizeof(text), "%c %u%%", side, level);
    } else {
        snprintf(text, sizeof(text), "%c --", side);
    }
    draw_text(canvas, x, 2, 66, &quinquefive_8, LV_TEXT_ALIGN_LEFT, LVGL_FOREGROUND, text);

    // gauge: 52 x 12 body + 3 x 6 terminal
    draw_outline(canvas, x, 14, 52, 12);
    draw_filled(canvas, x + 52, 17, 3, 6);
    if (known) {
        uint8_t clamped = level > 100 ? 100 : level;
        draw_filled(canvas, x + 2, 16, (48 * clamped + 50) / 100, 8);
    }

    if (known && charging) {
        draw_bolt(canvas, x + 58, 13);
    }
}

void draw_battery_status(lv_obj_t *canvas, const struct status_state *state) {
    draw_battery_gauge(canvas, 2, 'G', state->battery, true, state->charging);
}
