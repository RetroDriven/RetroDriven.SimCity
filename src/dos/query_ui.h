#ifndef SIMCITY_DOS_QUERY_UI_H
#define SIMCITY_DOS_QUERY_UI_H
#include <stddef.h>
#include <stdint.h>
typedef struct {int x,y;unsigned tile,category,attribute[5];} SimQueryInfo;
int sim_query_ui_read(const uint8_t *memory,size_t size,int tile_x,int tile_y,SimQueryInfo *info);
int sim_query_ui_render(uint8_t *fb,const uint8_t *font,const uint8_t *memory,size_t size,const SimQueryInfo *info,int panel_row);
/* Original info panel flips between rows3 and15 to stay clear of cursor. */
int sim_query_ui_panel_row(int previous_row,int cursor_x,int cursor_y);
#endif
