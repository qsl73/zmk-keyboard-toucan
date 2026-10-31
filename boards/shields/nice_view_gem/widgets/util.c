#include <zephyr/kernel.h>
#include "util.h"
#include "../assets/custom_fonts.h"
#include <ctype.h>


void to_uppercase(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = toupper(str[i]);
    }
}

void fill_background(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t rect_black_dsc;
    rect_black_dsc.bg_color = lv_color_white();  // set background color to white
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);

    lv_canvas_draw_rect(canvas, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, &rect_black_dsc);
}

void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align) {
    lv_draw_label_dsc_init(label_dsc);
    label_dsc->color = color;
    label_dsc->font = font;
    label_dsc->align = align;
}

void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color) {
    lv_draw_rect_dsc_init(rect_dsc);
    rect_dsc->bg_color = bg_color;
}

void init_line_dsc(lv_draw_line_dsc_t *line_dsc, lv_color_t color, uint8_t width) {
    lv_draw_line_dsc_init(line_dsc);
    line_dsc->color = color;
    line_dsc->width = width;
}

void draw_text(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w, const lv_font_t *font,
               lv_text_align_t align, lv_color_t color, const char *text) {
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, color, font, align);
    lv_canvas_draw_text(canvas, x, y, w, &label_dsc, text);
}

void draw_outline(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h) {
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_TRANSP;
    dsc.border_color = LVGL_FOREGROUND;
    dsc.border_width = 1;
    dsc.border_opa = LV_OPA_COVER;
    dsc.radius = 0;
    lv_canvas_draw_rect(canvas, x, y, w, h, &dsc);
}

void draw_filled(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h) {
    if (w <= 0 || h <= 0) {
        return;
    }
    lv_draw_rect_dsc_t dsc;
    init_rect_dsc(&dsc, LVGL_FOREGROUND);
    dsc.radius = 0;
    lv_canvas_draw_rect(canvas, x, y, w, h, &dsc);
}

void draw_hline(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w) {
    draw_filled(canvas, x, y, w, 1);
}

void draw_tag(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w, const char *label,
              bool active) {
    if (active) {
        draw_filled(canvas, x, y, w, 14);
    } else {
        draw_outline(canvas, x, y, w, 14);
    }
    draw_text(canvas, x, y + 3, w, &quinquefive_8, LV_TEXT_ALIGN_CENTER,
              active ? LVGL_BACKGROUND : LVGL_FOREGROUND, label);
}

void draw_bolt(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y) {
    /* Two convex triangles make the bolt (LVGL only fills convex polygons) */
    lv_draw_rect_dsc_t dsc;
    init_rect_dsc(&dsc, LVGL_FOREGROUND);
    lv_point_t top[] = {{x + 6, y}, {x, y + 8}, {x + 5, y + 8}};
    lv_point_t bottom[] = {{x + 3, y + 6}, {x + 8, y + 6}, {x + 2, y + 14}};
    lv_canvas_draw_polygon(canvas, top, 3, &dsc);
    lv_canvas_draw_polygon(canvas, bottom, 3, &dsc);
}
