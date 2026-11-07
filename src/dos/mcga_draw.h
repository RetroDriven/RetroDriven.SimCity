#ifndef SIMCITY_DOS_MCGA_DRAW_H
#define SIMCITY_DOS_MCGA_DRAW_H
#include <stdint.h>
#include <stddef.h>

/* Reconstructed standard MCGA primitives. Indexed framebuffer is 320x200.
 * Calls reject out-of-screen geometry instead of reproducing DOS corruption.
 * Source and framebuffer must be distinct valid buffers of the given size.
 * No clipping is implicit: left/right are the original source-column skips.
 */
typedef enum sim_mcga_op {
    SIM_MCGA_COPY = 0, SIM_MCGA_AND, SIM_MCGA_XOR, SIM_MCGA_OR
} sim_mcga_op;

int sim_mcga_fill(uint8_t *fb, int x0, int y0, int x1, int y1,
                  uint8_t color, sim_mcga_op op);
int sim_mcga_blit(uint8_t *fb, int x, int y, const uint8_t *source,
                  size_t source_size, unsigned width, unsigned height,
                  unsigned left, unsigned right, sim_mcga_op op);
int sim_mcga_mask(uint8_t *fb, int x, int y, const uint8_t *source,
                  size_t source_size, unsigned width, unsigned height,
                  unsigned left, unsigned right, uint8_t foreground,
                  uint8_t background, sim_mcga_op op);
/* Separate original transparent-zero entrypoint at 0x1f444. */
int sim_mcga_blit_nonzero(uint8_t *fb, int x, int y, const uint8_t *source,
                          size_t source_size, unsigned width, unsigned height,
                          unsigned left, unsigned right);
#endif
