#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/position_state_changed.h>
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators.h>
#endif
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/usb.h>
#include <zmk/split/central.h>

#include "battery.h"
#include "battery_peripheral.h"
#include "layer.h"
#include "mods.h"
#include "output.h"
#include "profile.h"
#include "screen.h"
#include "sleep.h"
#include "stats.h"

struct connection_status_state {
    bool connected;
};

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

/**
 * Draw buffers
 **/

static void draw_top(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);
    fill_background(canvas);

    if (is_sleep_screen_active()) {
        draw_sleep_screen(canvas);
        return;
    }

    // Draw widgets, top to bottom
    draw_battery_status(canvas, state);
    draw_battery_peripheral_status(canvas, state);
    draw_hline(canvas, 0, 31, SCREEN_WIDTH);
    draw_layer_status(canvas, state);
    draw_mods_status(canvas, state);
    draw_stats_status(canvas, state);
    draw_output_status(canvas, state);
    draw_profile_status(canvas, state);
}

static void redraw_all(void) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        draw_top(widget->obj, widget->cbuf, &widget->state);
    }
}

/**
 * Battery status
 **/

// L
static void set_battery_status(struct zmk_widget_screen *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

    widget->state.battery = state.level;

    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev =
        (eh != NULL) ? as_zmk_battery_state_changed(eh) : NULL;

    return (struct battery_status_state){
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

// R
static void set_battery_peripheral_status(struct zmk_widget_screen *widget,
                                          struct battery_peripheral_status_state state) {
    widget->state.battery_p = state.level;
    if (state.level == 0) {
        // the right half disconnected: its power state is unknown until it reconnects
        widget->state.charging_p = false;
    }

    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void battery_peripheral_status_update_cb(struct battery_peripheral_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_peripheral_status(widget, state); }
}

static struct battery_peripheral_status_state battery_peripheral_status_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        (eh != NULL) ? as_zmk_peripheral_battery_state_changed(eh) : NULL;
    uint8_t level = 0;

    if (ev != NULL) {
        level = ev->state_of_charge;
    } else {
        zmk_split_central_get_peripheral_battery_level(0, &level);
    }

    return (struct battery_peripheral_status_state){
        .level = level,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_peripheral_status, struct battery_peripheral_status_state,
                            battery_peripheral_status_update_cb, battery_peripheral_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_peripheral_status, zmk_peripheral_battery_state_changed);

/**
 * Right half USB power, reported by the right half through the trackpad's
 * input-split channel (boards/shields/toucan/usb_power_report.c)
 **/

#if DT_NODE_EXISTS(DT_NODELABEL(glidepoint_split))

static atomic_t right_usb_powered = ATOMIC_INIT(0);

static void right_power_work_cb(struct k_work *work) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->state.charging_p = atomic_get(&right_usb_powered) != 0;
        draw_top(widget->obj, widget->cbuf, &widget->state);
    }
}

K_WORK_DEFINE(right_power_work, right_power_work_cb);

static void right_power_input_cb(struct input_event *evt) {
    if (evt->type != INPUT_EV_MSC || evt->code != TOUCAN_INPUT_MSC_USB_POWER) {
        return;
    }
    atomic_set(&right_usb_powered, evt->value ? 1 : 0);
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &right_power_work);
    }
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(glidepoint_split)), right_power_input_cb);

#endif

/**
 * Layer status
 **/

static void set_layer_status(struct zmk_widget_screen *widget, struct layer_status_state state) {
    widget->state.layer_index = zmk_keymap_highest_layer_active();

    draw_top(widget->obj, widget->cbuf3, &widget->state);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state) {
        .index = index
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

/**
 * Modifiers, "mot en majuscules" and key count
 *
 * The display work queue only runs once ZMK has finished handling the key
 * (the system work queue is cooperative), so reading the HID and caps-word
 * state from the update callback sees the result of that key.
 **/

static atomic_t key_count = ATOMIC_INIT(0);

static int key_count_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev != NULL && ev->state) {
        atomic_inc(&key_count);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(nice_view_gem_key_count, key_count_listener);
ZMK_SUBSCRIPTION(nice_view_gem_key_count, zmk_position_state_changed);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(caps_word), okay)
/* Mirrors `struct behavior_caps_word_data` of ZMK v0.3 (behavior_caps_word.c) */
struct caps_word_data_mirror {
    bool active;
};

static bool caps_word_is_active(void) {
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(caps_word));
    return ((const struct caps_word_data_mirror *)dev->data)->active;
}
#else
static bool caps_word_is_active(void) { return false; }
#endif

struct mods_status_state {
    bool unused;
};

static void mods_status_update_cb(struct mods_status_state state) {
    uint8_t mods = zmk_hid_get_explicit_mods();
    bool caps_word = caps_word_is_active();

    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (widget->state.mods == mods && widget->state.caps_word == caps_word) {
            continue; // nothing visible changed: don't redraw on every key
        }
        widget->state.mods = mods;
        widget->state.caps_word = caps_word;
        widget->state.key_count = (uint32_t)atomic_get(&key_count);
        draw_top(widget->obj, widget->cbuf, &widget->state);
    }
}

static struct mods_status_state mods_status_get_state(const zmk_event_t *eh) {
    return (struct mods_status_state){.unused = false};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_mods_status, struct mods_status_state, mods_status_update_cb,
                            mods_status_get_state)

ZMK_SUBSCRIPTION(widget_mods_status, zmk_keycode_state_changed);
ZMK_SUBSCRIPTION(widget_mods_status, zmk_position_state_changed);

/**
 * Caps Lock indicator from the computer
 **/

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)

#define HID_INDICATOR_CAPS_LOCK BIT(1) // HID LED page: Num Lock = bit 0, Caps Lock = bit 1

struct caps_lock_status_state {
    bool active;
};

static void caps_lock_status_update_cb(struct caps_lock_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->state.caps_lock = state.active;
        draw_top(widget->obj, widget->cbuf, &widget->state);
    }
}

static struct caps_lock_status_state caps_lock_status_get_state(const zmk_event_t *eh) {
    const struct zmk_hid_indicators_changed *ev =
        (eh != NULL) ? as_zmk_hid_indicators_changed(eh) : NULL;
    zmk_hid_indicators_t indicators =
        (ev != NULL) ? ev->indicators : zmk_hid_indicators_get_current_profile();

    return (struct caps_lock_status_state){
        .active = (indicators & HID_INDICATOR_CAPS_LOCK) != 0,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_caps_lock_status, struct caps_lock_status_state,
                            caps_lock_status_update_cb, caps_lock_status_get_state)

ZMK_SUBSCRIPTION(widget_caps_lock_status, zmk_hid_indicators_changed);

#endif

/**
 * Typing speed (WPM) and statistics
 **/

struct wpm_status_state {
    uint8_t wpm;
};

static void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->state.wpm = state.wpm;
        if (state.wpm > widget->state.wpm_max) {
            widget->state.wpm_max = state.wpm;
        }
        widget->state.key_count = (uint32_t)atomic_get(&key_count);
        draw_top(widget->obj, widget->cbuf, &widget->state);
    }
}

static struct wpm_status_state wpm_status_get_state(const zmk_event_t *eh) {
    const struct zmk_wpm_state_changed *ev = (eh != NULL) ? as_zmk_wpm_state_changed(eh) : NULL;
    int wpm = (ev != NULL) ? ev->state : 0;

    return (struct wpm_status_state){
        .wpm = (uint8_t)CLAMP(wpm, 0, 255),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state, wpm_status_update_cb,
                            wpm_status_get_state)

ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

/**
 * Output status
 **/

static void set_output_status(struct zmk_widget_screen *widget,
                              const struct output_status_state *state) {
    widget->state.selected_endpoint = state->selected_endpoint;
    widget->state.active_profile_index = state->active_profile_index;
    widget->state.active_profile_connected = state->active_profile_connected;
    widget->state.active_profile_bonded = state->active_profile_bonded;

    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}

static struct output_status_state output_status_get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

/**
 * Activity state handling for sleep screen
 **/

static int display_activity_event_handler(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return -ENOTSUP;
    }

    switch (ev->state) {
    case ZMK_ACTIVITY_ACTIVE:
        set_sleep_screen_active(false);
        // No need to force a redraw, it will happen automatically if really coming back from sleep (ACTIVE also comes after IDLE)
        break;
    case ZMK_ACTIVITY_SLEEP:
        set_sleep_screen_active(true);
        redraw_all();
        // Force LVGL to process pending updates and flush to display hardware
        // before the CPU enters deep sleep
        lv_task_handler();
        lv_refr_now(NULL);
        break;
    default:
        break; // ignore other states (like IDLE)
    }

    return 0;
}

ZMK_LISTENER(nice_view_gem_display, display_activity_event_handler);
ZMK_SUBSCRIPTION(nice_view_gem_display, zmk_activity_state_changed);

/**
 * Initialization
 **/

int zmk_widget_screen_init(struct zmk_widget_screen *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, SCREEN_WIDTH, SCREEN_HEIGHT);

    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, SCREEN_WIDTH, SCREEN_HEIGHT, LV_IMG_CF_TRUE_COLOR);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_battery_peripheral_status_init();
    widget_layer_status_init();
    widget_mods_status_init();
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
    widget_caps_lock_status_init();
#endif
    widget_wpm_status_init();
    widget_output_status_init();

    return 0;
}

lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget) { return widget->obj; }
