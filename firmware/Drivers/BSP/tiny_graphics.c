#include "tiny_graphics.h"

static int tg_clip_span(int origin, int size, uint16_t limit, uint16_t *start, uint16_t *len)
{
    int clipped_start = origin;
    int clipped_end = origin + size;

    if ((size <= 0) || (origin >= (int)limit) || (clipped_end <= 0))
    {
        return 0;
    }

    if (clipped_start < 0)
    {
        clipped_start = 0;
    }
    if (clipped_end > (int)limit)
    {
        clipped_end = (int)limit;
    }

    *start = (uint16_t)clipped_start;
    *len = (uint16_t)(clipped_end - clipped_start);
    return (*len > 0U) ? 1 : 0;
}

void tg_fill_rect(const tg_canvas_t *canvas,
                  int x,
                  int y,
                  int w,
                  int h,
                  tg_color_t color)
{
    uint16_t clipped_x;
    uint16_t clipped_y;
    uint16_t clipped_w;
    uint16_t clipped_h;

    if ((canvas == 0) || (canvas->fill_rect == 0))
    {
        return;
    }

    if (!tg_clip_span(x, w, canvas->width, &clipped_x, &clipped_w) ||
        !tg_clip_span(y, h, canvas->height, &clipped_y, &clipped_h))
    {
        return;
    }

    canvas->fill_rect(canvas->ctx, clipped_x, clipped_y, clipped_w, clipped_h, color);
}

void tg_draw_pixel(const tg_canvas_t *canvas, int x, int y, tg_color_t color)
{
    tg_fill_rect(canvas, x, y, 1, 1, color);
}

void tg_draw_line(const tg_canvas_t *canvas,
                  int x0,
                  int y0,
                  int x1,
                  int y1,
                  tg_color_t color)
{
    int dx = (x1 >= x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = (y1 >= y0) ? (y0 - y1) : (y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (1)
    {
        tg_draw_pixel(canvas, x0, y0, color);
        if ((x0 == x1) && (y0 == y1))
        {
            break;
        }

        if ((err * 2) >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if ((err * 2) <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void tg_draw_rect(const tg_canvas_t *canvas,
                  int x,
                  int y,
                  int w,
                  int h,
                  tg_color_t color)
{
    if ((w <= 0) || (h <= 0))
    {
        return;
    }

    tg_fill_rect(canvas, x, y, w, 1, color);
    tg_fill_rect(canvas, x, y + h - 1, w, 1, color);
    tg_fill_rect(canvas, x, y, 1, h, color);
    tg_fill_rect(canvas, x + w - 1, y, 1, h, color);
}

void tg_fill_round_rect(const tg_canvas_t *canvas,
                        int x,
                        int y,
                        int w,
                        int h,
                        int radius,
                        tg_color_t color)
{
    int row;

    if ((w <= 0) || (h <= 0))
    {
        return;
    }

    if (radius <= 0)
    {
        tg_fill_rect(canvas, x, y, w, h, color);
        return;
    }

    if ((radius * 2) > w)
    {
        radius = w / 2;
    }
    if ((radius * 2) > h)
    {
        radius = h / 2;
    }

    tg_fill_rect(canvas, x + radius, y, w - (radius * 2), h, color);
    for (row = 0; row < radius; ++row)
    {
        int dy = (radius - 1) - row;
        int inset = radius;

        while ((inset > 0) && (((inset * inset) + (dy * dy)) > (radius * radius)))
        {
            --inset;
        }

        inset = radius - inset;
        tg_fill_rect(canvas, x + inset, y + row, w - (inset * 2), 1, color);
        tg_fill_rect(canvas, x + inset, y + h - 1 - row, w - (inset * 2), 1, color);
    }
}

void tg_draw_circle(const tg_canvas_t *canvas, int cx, int cy, int radius, tg_color_t color)
{
    int x = radius;
    int y = 0;
    int err = 1 - radius;

    if (radius < 0)
    {
        return;
    }

    while (x >= y)
    {
        tg_draw_pixel(canvas, cx + x, cy + y, color);
        tg_draw_pixel(canvas, cx + y, cy + x, color);
        tg_draw_pixel(canvas, cx - y, cy + x, color);
        tg_draw_pixel(canvas, cx - x, cy + y, color);
        tg_draw_pixel(canvas, cx - x, cy - y, color);
        tg_draw_pixel(canvas, cx - y, cy - x, color);
        tg_draw_pixel(canvas, cx + y, cy - x, color);
        tg_draw_pixel(canvas, cx + x, cy - y, color);

        ++y;
        if (err < 0)
        {
            err += (2 * y) + 1;
        }
        else
        {
            --x;
            err += (2 * (y - x)) + 1;
        }
    }
}

void tg_draw_text(const tg_canvas_t *canvas,
                  int x,
                  int y,
                  const char *text,
                  uint8_t scale,
                  tg_color_t color,
                  tg_glyph_fn glyph_cb)
{
    int cursor_x = x;

    if ((canvas == 0) || (text == 0) || (glyph_cb == 0))
    {
        return;
    }

    if (scale == 0U)
    {
        scale = 1U;
    }

    while (*text != '\0')
    {
        uint8_t glyph[8];
        uint8_t glyph_w = 0U;
        uint8_t glyph_h = 0U;
        uint8_t col;

        glyph_cb(*text, glyph, &glyph_w, &glyph_h);
        for (col = 0U; col < glyph_w; ++col)
        {
            uint8_t bits = glyph[col];
            uint8_t row = 0U;

            while (row < glyph_h)
            {
                if ((bits & (1U << row)) != 0U)
                {
                    uint8_t run = 1U;

                    while (((uint8_t)(row + run) < glyph_h) && ((bits & (1U << (row + run))) != 0U))
                    {
                        ++run;
                    }

                    tg_fill_rect(canvas,
                                 cursor_x + ((int)col * (int)scale),
                                 y + ((int)row * (int)scale),
                                 scale,
                                 (int)run * (int)scale,
                                 color);
                    row = (uint8_t)(row + run);
                }
                else
                {
                    ++row;
                }
            }
        }

        cursor_x += ((int)glyph_w * (int)scale) + 1;
        ++text;
    }
}