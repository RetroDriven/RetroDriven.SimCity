#ifndef SIMCITY_DOS_MAPS_SUBMENU_H
#define SIMCITY_DOS_MAPS_SUBMENU_H
#include <stddef.h>
#include <stdint.h>
typedef struct {int layer,current,selected,x0,y0,x1,y1,text_x,text_y,width;} SimMapsSubmenu;
int sim_maps_submenu_open(const uint8_t *memory,size_t size,unsigned layer,int pointer_x,int pointer_y,SimMapsSubmenu *popup);
int sim_maps_submenu_render(uint8_t *fb,const uint8_t *font,const SimMapsSubmenu *popup);
int sim_maps_submenu_hit(const SimMapsSubmenu *popup,int x,int y);
/* Returns0 to continue,1 to close,2 to close and replay the original extended
 * key through the outer input adapter. Escape discards highlight. Enter/LF/
 * Space accept highlighted row, or retain subtype with no highlighted row. */
int sim_maps_submenu_key(SimMapsSubmenu *popup,unsigned key);
/* Original3175a selects the requested main layer even after popup Escape.
 * The subtype changes only when a row remains highlighted at close. */
int sim_maps_submenu_commit(uint8_t *memory,size_t size,const SimMapsSubmenu *popup);
#endif
