#ifndef TINY_GRAPHICS_H
#define TINY_GRAPHICS_H

#include <stddef.h>
#include <stdint.h>

typedef uint16_t tg_color_t;

typedef void (*tg_fill_rect_fn)(void *ctx,
                                uint16_t x,
                                uint16_t y,
                                uint16_t w,
                                uint16_t h,
                                tg_color_t color);

typedef struct
{
    uint16_t width;
    uint16_t height;
    tg_fill_rect_fn fill_rect;
    void *ctx;
} tg_canvas_t;

typedef void (*tg_glyph_fn)(char c, uint8_t *columns, uint8_t *width, uint8_t *height);

void tg_fill_rect(const tg_canvas_t *canvas,
                  int x,
                  int y,
                  int w,
                  int h,
                  tg_color_t color);

void tg_draw_pixel(const tg_canvas_t *canvas, int x, int y, tg_color_t color);
void tg_draw_line(const tg_canvas_t *canvas,
                  int x0,
                  int y0,
                  int x1,
                  int y1,
                  tg_color_t color);
void tg_draw_rect(const tg_canvas_t *canvas,
                  int x,
                  int y,
                  int w,
                  int h,
                  tg_color_t color);
void tg_fill_round_rect(const tg_canvas_t *canvas,
                        int x,
                        int y,
                        int w,
                        int h,
                        int radius,
                        tg_color_t color);
void tg_draw_circle(const tg_canvas_t *canvas, int cx, int cy, int radius, tg_color_t color);
void tg_draw_text(const tg_canvas_t *canvas,
                  int x,
                  int y,
                  const char *text,
                  uint8_t scale,
                  tg_color_t color,
                  tg_glyph_fn glyph_cb);

#endif