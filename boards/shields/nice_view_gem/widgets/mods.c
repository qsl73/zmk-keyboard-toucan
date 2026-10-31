#include <zephyr/kernel.h>
#include <dt-bindings/zmk/modifiers.h>
#include "mods.h"

/*
 * Modifiers held right now, in the order of the left home row
 * (A = Ctrl, S = Option, D = Cmd, F = Maj), then the two capital modes.
 */
void draw_mods_status(lv_obj_t *canvas, const struct status_state *state) {
    uint8_t m = state->mods;

    draw_tag(canvas, 2, 76, 33, "CTL", m & (MOD_LCTL | MOD_RCTL));
    draw_tag(canvas, 37, 76, 33, "OPT", m & (MOD_LALT | MOD_RALT));
    draw_tag(canvas, 72, 76, 33, "CMD", m & (MOD_LGUI | MOD_RGUI));
    draw_tag(canvas, 107, 76, 33, "MAJ", m & (MOD_LSFT | MOD_RSFT));

    draw_tag(canvas, 2, 94, 72, "MOT MAJ", state->caps_word);
    draw_tag(canvas, 76, 94, 64, "VERROU", state->caps_lock);
}
