#ifndef SIMCITY_DOS_ABOUT_UI_H
#define SIMCITY_DOS_ABOUT_UI_H
#include <stddef.h>
#include <stdint.h>
/* Original About modal29bf0; original accepts any key or click. */
int sim_about_ui_render(uint8_t *fb,const uint8_t *font,const uint8_t *memory,size_t size);
#endif
