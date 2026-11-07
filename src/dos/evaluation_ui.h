#ifndef SIMCITY_DOS_EVALUATION_UI_H
#define SIMCITY_DOS_EVALUATION_UI_H
#include <stdint.h>
#include <stddef.h>
/* Original 320x200 MCGA Evaluation panel, after engine evaluation has run.
 * Memory must contain original EXE strings and installed MESSAGE.PTF tables.
 * Enter/Escape or a click acknowledge this read-only modal. */
int sim_evaluation_ui_render(uint8_t *framebuffer,const uint8_t *font,
                              const uint8_t *memory,size_t memory_size);
#endif
