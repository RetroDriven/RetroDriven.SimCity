#ifndef SIM_DOS_UI_DRAW_H
#define SIM_DOS_UI_DRAW_H
#include <stdint.h>
/* Standard MCGA 8x8 font is 8192 caller-owned byte masks. */
int sim_ui_x(uint16_t units);
int sim_ui_y(uint16_t units);
int sim_ui_text(uint8_t *fb, const uint8_t *font, int x, int y,
                const char *text, uint8_t foreground, uint8_t background);
int sim_ui_frame(uint8_t *fb, int x0, int y0, int x1, int y1,
                 int thickness, uint8_t color);
int sim_ui_button(uint8_t *fb, const uint8_t *font, uint16_t column,
                  uint16_t row, const char *text, uint8_t foreground,
                  uint8_t background);
#endif
