#ifndef SIMCITY_DOS_GRAPHS_UI_H
#define SIMCITY_DOS_GRAPHS_UI_H
#include <stdint.h>
#include <stddef.h>
/* Original graph image record3: u16 width56,height76 +4256 indexed pixels. */
int sim_graphs_ui_render(uint8_t *fb,const uint8_t *font,const uint8_t *memory,
 size_t size,const uint8_t *graph_frame,size_t frame_size);
int sim_graphs_ui_activate(uint8_t *memory,size_t size,unsigned button);
int sim_graphs_ui_hit(const uint8_t *memory,size_t size,int x,int y);
int sim_graphs_ui_cursor(const uint8_t *memory,size_t size,unsigned button,int *x,int *y);
#endif
