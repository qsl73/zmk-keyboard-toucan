#include <zephyr/kernel.h>
#include <stdio.h>
#include "output.h"
#include "../assets/custom_fonts.h"

/*
 * Bottom-left: where the keystrokes go.
 *   "USB"        sent over the cable
 *   "BT2 OK"     Bluetooth profile 2, connected
 *   "BT2 ..."    profile 2 paired, but the computer is not connected
 *   "BT2 LIBRE"  profile 2 has no paired computer (ready to pair)
 */
void draw_output_status(lv_obj_t *canvas, const struct status_state *state) {
    char text[24];

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    switch (state->selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        snprintf(text, sizeof(text), "USB");
        break;
    case ZMK_TRANSPORT_BLE:
        if (!state->active_profile_bonded) {
            snprintf(text, sizeof(text), "BT%d LIBRE", state->active_profile_index + 1);
        } else if (state->active_profile_connected) {
            snprintf(text, sizeof(text), "BT%d OK", state->active_profile_index + 1);
        } else {
            snprintf(text, sizeof(text), "BT%d ...", state->active_profile_index + 1);
        }
        break;
    default:
        snprintf(text, sizeof(text), "AUCUNE");
        break;
    }
#else
    snprintf(text, sizeof(text), state->connected ? "OK" : "...");
#endif

    draw_text(canvas, 2, 150, 90, &quinquefive_8, LV_TEXT_ALIGN_LEFT, LVGL_FOREGROUND, text);
}
