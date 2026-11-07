#include "ui_draw.h"
#include "mcga_draw.h"
#include <string.h>

int sim_ui_x(uint16_t value)
{
    return (value & 255) * 8 + (int8_t)(value >> 8);
}

int sim_ui_y(uint16_t value)
{
    return (value & 255) * 8 + ((int8_t)(value >> 8) * 8) / 14;
}

int sim_ui_text(uint8_t *fb, const uint8_t *font, int x, int y,
                const char *text, uint8_t fg, uint8_t bg)
{
    if (!fb || !font || !text) return 0;
    for (; *text; ++text) {
        unsigned glyph = (unsigned char)*text;
        if (glyph >= 128 || !sim_mcga_mask(fb, x, y, font+glyph*64, 64,
                                          8, 8, 0, 0, fg, bg, SIM_MCGA_COPY)) return 0;
        x += 8;
        if (x >= 320) { x = 0; y += 8; }
    }
    return 1;
}

int sim_ui_frame(uint8_t *fb, int x0, int y0, int x1, int y1,
                 int thickness, uint8_t color)
{
    int a, b;
    if (!fb) return 0;
    if (!thickness) return 1;
    if (thickness < 0) {
        --x0; --y0; ++x1; ++y1; ++thickness;
    } else --thickness;
    a = y1 - thickness; b = y0 + thickness;
    return sim_mcga_fill(fb, x0, b, x0+thickness, a, color, SIM_MCGA_COPY) &&
           sim_mcga_fill(fb, x1, b, x1-thickness, a, color, SIM_MCGA_COPY) &&
           sim_mcga_fill(fb, x0, b, x1, y0, color, SIM_MCGA_COPY) &&
           sim_mcga_fill(fb, x0, a, x1, y1, color, SIM_MCGA_COPY);
}

int sim_ui_button(uint8_t *fb, const uint8_t *font, uint16_t column,
                  uint16_t row, const char *text, uint8_t fg, uint8_t bg)
{
    int x, y, end;
    size_t length;
    if (!text) return 0;
    length = strlen(text);
    if (!length || length > 40) return 0;
    x = sim_ui_x(column); y = sim_ui_y(row); end = x+(int)length*8-1;
    return sim_ui_frame(fb, x, y, end, y+7, -3, fg) &&
           sim_ui_frame(fb, x, y, end, y+7, -2, bg) &&
           sim_ui_text(fb, font, x, y, text, fg, bg);
}
