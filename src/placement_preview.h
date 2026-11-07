#ifndef SIM_PLACEMENT_PREVIEW_H
#define SIM_PLACEMENT_PREVIEW_H
#include <stdint.h>
#include "dos/city_view.h"
typedef struct {int left,top,right,bottom;} SimPlacementPreview;
/* Geometry uses the same cursor-cell anchor as the original placement path. */
int sim_placement_preview_bounds(const sim_city_view *view,int x,int y,unsigned tool,uint16_t tile,SimPlacementPreview *rect);
int sim_placement_preview_render(uint8_t *fb,const sim_city_view *view,int x,int y,unsigned tool,uint16_t tile);
#endif
