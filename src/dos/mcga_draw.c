#include "mcga_draw.h"

static int valid_op(sim_mcga_op op)
{
    return op >= SIM_MCGA_COPY && op <= SIM_MCGA_OR;
}

static uint8_t combine(uint8_t source, uint8_t destination, sim_mcga_op op)
{
    switch (op) {
    case SIM_MCGA_AND: return (uint8_t)(source & destination);
    case SIM_MCGA_XOR: return (uint8_t)(source ^ destination);
    case SIM_MCGA_OR: return (uint8_t)(source | destination);
    default: return source;
    }
}

int sim_mcga_fill(uint8_t *fb, int x0, int y0, int x1, int y1,
                  uint8_t color, sim_mcga_op op)
{
    int x, y, swap;
    if (fb == NULL || !valid_op(op)) return 0;
    if (x0 > x1) { swap = x0; x0 = x1; x1 = swap; }
    if (y0 > y1) { swap = y0; y0 = y1; y1 = swap; }
    if (x0 < 0 || y0 < 0 || x1 >= 320 || y1 >= 200) return 0;
    for (y = y0; y <= y1; ++y)
        for (x = x0; x <= x1; ++x) {
            size_t offset = (size_t)y * 320 + (unsigned)x;
            fb[offset] = combine(color, fb[offset], op);
        }
    return 1;
}

static int geometry(uint8_t *fb, int x, int y, const uint8_t *source,
                     size_t size, unsigned width, unsigned height,
                     unsigned left, unsigned right)
{
    if (fb == NULL || source == NULL || width == 0 || height == 0) return 0;
    if (height > size / width || left >= width || right >= width - left) return 0;
    if (x < 0 || y < 0 || x >= 320 || y >= 200) return 0;
    if (height > 200u - (unsigned)y || width - right > 320u - (unsigned)x) return 0;
    return 1;
}

static int draw(uint8_t *fb, int x, int y, const uint8_t *source,
                 size_t size, unsigned width, unsigned height,
                 unsigned left, unsigned right, uint8_t foreground,
                 uint8_t background, sim_mcga_op op, int mask, int nonzero)
{
    unsigned row, column;
    if (!valid_op(op) || !geometry(fb,x,y,source,size,width,height,left,right)) return 0;
    for (row = 0; row < height; ++row)
        for (column = left; column < width - right; ++column) {
            uint8_t value = source[(size_t)row * width + column];
            size_t offset = ((size_t)y + row) * 320 + (unsigned)x + column;
            if (nonzero && value == 0) continue;
            if (mask) value = (uint8_t)((value & foreground) | ((uint8_t)~value & background));
            fb[offset] = combine(value,fb[offset],op);
        }
    return 1;
}

int sim_mcga_blit(uint8_t *fb, int x, int y, const uint8_t *source,
                  size_t size, unsigned width, unsigned height,
                  unsigned left, unsigned right, sim_mcga_op op)
{
    return draw(fb,x,y,source,size,width,height,left,right,0,0,op,0,0);
}

int sim_mcga_mask(uint8_t *fb, int x, int y, const uint8_t *source,
                  size_t size, unsigned width, unsigned height,
                  unsigned left, unsigned right, uint8_t foreground,
                  uint8_t background, sim_mcga_op op)
{
    return draw(fb,x,y,source,size,width,height,left,right,foreground,background,op,1,0);
}

int sim_mcga_blit_nonzero(uint8_t *fb, int x, int y, const uint8_t *source,
                          size_t size, unsigned width, unsigned height,
                          unsigned left, unsigned right)
{
    return draw(fb,x,y,source,size,width,height,left,right,0,0,SIM_MCGA_COPY,0,1);
}
