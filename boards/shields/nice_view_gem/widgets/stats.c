#include <zephyr/kernel.h>
#include <stdio.h>
#include "stats.h"
#include "../assets/custom_fonts.h"

/*
 * Typing statistics since the keyboard was switched on:
 *   "42 MPM"   current speed (words per minute)   "MAX 87"   best speed
 *   "TOUCHES"  number of key presses              "12345"
 */
void draw_stats_status(lv_obj_t *canvas, const struct status_state *state) {
    char text[16];

    draw_hline(canvas, 0, 114, SCREEN_WIDTH);

    snprintf(text, sizeof(text), "%u MPM", state->wpm);
    draw_text(canvas, 2, 119, 70, &quinquefive_8, LV_TEXT_ALIGN_LEFT, LVGL_FOREGROUND, text);
    snprintf(text, sizeof(text), "MAX %u", state->wpm_max);
    draw_text(canvas, 72, 119, 70, &quinquefive_8, LV_TEXT_ALIGN_RIGHT, LVGL_FOREGROUND, text);

    draw_text(canvas, 2, 131, 70, &quinquefive_8, LV_TEXT_ALIGN_LEFT, LVGL_FOREGROUND, "TOUCHES");
    snprintf(text, sizeof(text), "%u", (unsigned int)state->key_count);
    draw_text(canvas, 72, 131, 70, &quinquefive_8, LV_TEXT_ALIGN_RIGHT, LVGL_FOREGROUND, text);

    draw_hline(canvas, 0, 145, SCREEN_WIDTH);
}
