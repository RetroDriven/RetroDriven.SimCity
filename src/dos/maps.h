#ifndef SIMCITY_DOS_MAPS_H
#define SIMCITY_DOS_MAPS_H
#include <stddef.h>
#include <stdint.h>
#include "graphics_pack.h"

enum sim_map_layer {
    SIM_MAP_CITY_FORM=1, SIM_MAP_POWER=2, SIM_MAP_TRANSPORT=3,
    SIM_MAP_POPULATION=4, SIM_MAP_TRAFFIC=5, SIM_MAP_POLLUTION=6,
    SIM_MAP_CRIME=7, SIM_MAP_LAND_VALUE=8, SIM_MAP_PROTECTION=9
};
/* Copy the six original mode6 Maps rectangles and one-pixel scale into a
 * newly initialized DGROUP. Call once during graphics setup, not each repaint
 * (which would undo a user-moved window). Preserves layer/subtype selection. */
int sim_maps_configure_mcga(uint8_t *dgroup,size_t size);
/* Full original Maps window repaint31490, standard MCGA one-pixel map.
 * Uses real simulation memory (at least1MB), including DGROUP4d2c0 and far
 * density/protection/growth arrays. Does not mutate simulation/cache state.
 * DGROUP3268=layer1..9,326c=population1:density/2:growth,
 * 326a=protection1:police/2:fire. Standard geometry is original mode6 preset.
 * Image records4/5/6 and original tile-color lookup/font come from pack.
 */
int sim_maps_render(uint8_t *fb,const sim_graphics_pack *pack,
                      const uint8_t *memory,size_t memory_size);
/* Original hit rectangles31228 for the nine map buttons. Coordinates inclusive.
 * event411..419 corresponds to layer1..9; returns0 outside the buttons. */
int sim_maps_hit(const uint8_t *dgroup,size_t size,int x,int y);
/* Original3194a viewport outline from the unhighlighted state. Draw once on
 * each fresh Maps repaint; original2-pixel border XORs each pixel with255.
 * Does not change original cached-outline fields or toggle326e. */
int sim_maps_viewport_render(uint8_t *fb,const uint8_t *dgroup,size_t size);
/* Original30cbc/311de recenter/clamp mutation only. The caller redraws the
 * editor/Maps views after this action; x/y are map-cell coordinates. */
int sim_maps_center(uint8_t *dgroup,size_t size,int map_x,int map_y);
/* Original menu labels from English MESSAGE.PTF record10. */
const char *sim_maps_label(unsigned layer,unsigned population,unsigned protection);
#endif
