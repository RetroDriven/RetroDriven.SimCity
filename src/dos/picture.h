#ifndef SIM_DOS_PICTURE_H
#define SIM_DOS_PICTURE_H
#include <stddef.h>
#include <stdint.h>
/* Standard MCGA PPF only. Caller provides 64000 bytes. No allocation. */
int sim_picture_decode(const uint8_t *compressed, size_t size, uint8_t *pixels);
void sim_palette_rgb8(const uint8_t *rgb6, uint32_t *rgb8);
#endif
