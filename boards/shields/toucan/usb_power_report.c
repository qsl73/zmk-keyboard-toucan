/*
 * Toucan, right half: report USB power to the left half.
 *
 * ZMK does not send a peripheral's USB/charging state to the central half, so
 * the right half reads the nRF52840 VBUS detector every few seconds and, when
 * it changes (and each time the halves reconnect), sends it as an input event
 * through the trackpad's existing input-split channel. The left half's screen
 * picks it up (nice_view_gem/widgets/screen.c). The event is sent with
 * sync = false, so the trackpad listener on the left ignores it.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <hal/nrf_power.h>

#include <zmk/event_manager.h>
#include <zmk/events/split_peripheral_status_changed.h>

/* Must match nice_view_gem/widgets/util.h */
#define TOUCAN_INPUT_MSC_USB_POWER 0x0100

#define POLL_PERIOD K_SECONDS(5)
#define RESEND_DELAY K_SECONDS(2)

static const struct device *const trackpad = DEVICE_DT_GET(DT_NODELABEL(glidepoint));
static int last_state = -1;

static void send_state(void) {
    if (last_state < 0) {
        return;
    }
    input_report(trackpad, INPUT_EV_MSC, TOUCAN_INPUT_MSC_USB_POWER, last_state, false,
                 K_NO_WAIT);
}

static void resend_handler(struct k_work *work) { send_state(); }
static K_WORK_DELAYABLE_DEFINE(resend_work, resend_handler);

static void poll_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(poll_work, poll_handler);

static void poll_handler(struct k_work *work) {
    int now = nrf_power_usbregstatus_vbusdet_get(NRF_POWER) ? 1 : 0;

    if (now != last_state) {
        last_state = now;
        send_state();
    }
    k_work_schedule(&poll_work, POLL_PERIOD);
}

static int split_status_listener(const zmk_event_t *eh) {
    const struct zmk_split_peripheral_status_changed *ev =
        as_zmk_split_peripheral_status_changed(eh);

    if (ev != NULL && ev->connected) {
        // give the left half time to subscribe to the trackpad channel
        k_work_reschedule(&resend_work, RESEND_DELAY);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(toucan_usb_power, split_status_listener);
ZMK_SUBSCRIPTION(toucan_usb_power, zmk_split_peripheral_status_changed);

static int usb_power_report_init(void) {
    k_work_schedule(&poll_work, K_SECONDS(1));
    return 0;
}

SYS_INIT(usb_power_report_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
