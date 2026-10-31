#include <zephyr/kernel.h>
#include <drivers/behavior.h>
#include <stdio.h>
#include <string.h>

#include "layer.h"
#include "../assets/custom_fonts.h"
#include <zmk/physical_layouts.h>
#include <zmk/keymap.h>
#include <zmk/matrix.h>

/* Full French name shown under the short layer name (display-name in the keymap) */
static const struct {
    const char *short_name;
    const char *full_name;
} layer_titles[] = {
    {"BASE", "QWERTY"},       {"CURS", "CURSEUR"}, {"NUM", "NOMBRES"},
    {"FUNC", "FONCTIONS"},    {"SYM", "SYMBOLES"}, {"MOUSE", "SOURIS"},
    {"RACC", "RACCOURCIS"},   {"SYS", "SYSTEME"},  {"WORLD", "ACCENTS"},
};

static const char *full_name_for(const char *short_name) {
    for (size_t i = 0; i < ARRAY_SIZE(layer_titles); i++) {
        if (strcmp(short_name, layer_titles[i].short_name) == 0) {
            return layer_titles[i].full_name;
        }
    }
    return "";
}

void draw_layer_status(lv_obj_t *canvas, const struct status_state *state) {
    char fallback_layer_name[16];

    const char *layer_name = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(state->layer_index));

    if (layer_name == NULL || layer_name[0] == '\0') {
        sprintf(fallback_layer_name, "L#%" PRIu8, state->layer_index);
        layer_name = fallback_layer_name;
    }

    // 29 px per character: a 1 px wider box keeps 5-letter names on one line
    draw_text(canvas, -1, 36, SCREEN_WIDTH + 2, &quinquefive_24, LV_TEXT_ALIGN_CENTER,
              LVGL_FOREGROUND, layer_name);
    draw_text(canvas, 0, 63, SCREEN_WIDTH, &quinquefive_8, LV_TEXT_ALIGN_CENTER, LVGL_FOREGROUND,
              full_name_for(layer_name));
}
