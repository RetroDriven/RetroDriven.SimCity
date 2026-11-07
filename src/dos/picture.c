#include "picture.h"
#include "resource_lzss.h"

int sim_picture_decode(const uint8_t *compressed, size_t size, uint8_t *pixels)
{
    sim_lzss state;
    uint8_t header[12];
    if (!compressed || !pixels || !size) return 0;
    sim_lzss_init(&state, compressed, size);
    if (sim_lzss_read(&state, header, 12) != 12) return 0;
    /* DOS reader 0x20bb6 consumes a 12-byte header. The observed standard
     * MCGA pictures declare 320x200 little-endian at offsets 8 and 10.
     * Other picture encodings and display sizes are outside this adapter. */
    if (header[2] != 8 || header[8] != 0x40 || header[9] != 1 ||
        header[10] != 200 || header[11] != 0) return 0;
    return sim_lzss_read(&state, pixels, 64000) == 64000 &&
           !sim_lzss_malformed(&state);
}

void sim_palette_rgb8(const uint8_t *rgb6, uint32_t *rgb8)
{
    unsigned i;
    for (i = 0; i < 256; ++i) {
        uint32_t r = rgb6[i*3], g = rgb6[i*3+1], b = rgb6[i*3+2];
        /* VGA DAC expansion used by the DOSBox reference. */
        r = (r << 2) | (r >> 4);
        g = (g << 2) | (g >> 4);
        b = (b << 2) | (b >> 4);
        rgb8[i] = (r << 16) | (g << 8) | b;
    }
}
