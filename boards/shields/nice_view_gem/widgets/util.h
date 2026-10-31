#pragma once

#include <lvgl.h>
#include <zmk/endpoints.h>

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

#define BUFFER_SIZE 168
#define BUFFER_OFFSET_MIDDLE 0
#define BUFFER_OFFSET_BOTTOM 0

#define LVGL_BACKGROUND lv_color_black()
#define LVGL_FOREGROUND lv_color_white()

/*
 * Code of the input event the right half sends (through the trackpad's
 * input-split channel) to report whether it is powered over USB.
 * Must match boards/shields/toucan/usb_power_report.c.
 */
#define TOUCAN_INPUT_MSC_USB_POWER 0x0100

struct status_state {
    uint8_t battery;
    uint8_t battery_p;
    bool charging;
    bool charging_p;
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    uint8_t layer_index;
    const char *layer_label;
    uint8_t mods;       // explicit modifiers currently held (MOD_* bits)
    bool caps_word;     // "mot en majuscules" active
    bool caps_lock;     // Caps Lock indicator from the host
    uint8_t wpm;        // current words per minute
    uint8_t wpm_max;    // best WPM since power-on
    uint32_t key_count; // key presses since power-on
#else
    bool connected;
#endif
};

void to_uppercase(char *str);
void fill_background(lv_obj_t *canvas);
void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color);
void init_line_dsc(lv_draw_line_dsc_t *line_dsc, lv_color_t color, uint8_t width);
void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align);

/* Small drawing helpers shared by the widgets */
void draw_text(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w, const lv_font_t *font,
               lv_text_align_t align, lv_color_t color, const char *text);
void draw_outline(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h);
void draw_filled(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h);
void draw_hline(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w);
/* Box with a centred 8 px label; filled (inverted) when active */
void draw_tag(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w, const char *label,
              bool active);
/* Lightning bolt meaning "plugged in", 8 x 14 px */
void draw_bolt(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y);
