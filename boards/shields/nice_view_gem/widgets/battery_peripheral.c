#include <zephyr/kernel.h>
#include "battery.h"
#include "battery_peripheral.h"

/*
 * Right half. ZMK reports a level of 0 when the right half disconnects, so 0
 * is shown as "D --". Its USB power state arrives from the right half itself
 * (see TOUCAN_INPUT_MSC_USB_POWER in util.h).
 */
void draw_battery_peripheral_status(lv_obj_t *canvas, const struct status_state *state) {
    bool known = state->battery_p > 0;
    draw_battery_gauge(canvas, 76, 'D', state->battery_p, known, state->charging_p);
}
