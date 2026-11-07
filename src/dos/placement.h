#ifndef SIMCITY_DOS_PLACEMENT_H
#define SIMCITY_DOS_PLACEMENT_H
#include <stddef.h>
#include <stdint.h>
#include "city_tools.h"

typedef struct sim_city_placement_state {
    uint16_t random[7]; /* Original DS2402..240e, separate from terrain RNG. */
    uint16_t special_4fd8;
} sim_city_placement_state;

typedef struct sim_city_tool_info {
    unsigned width;
    uint32_t cost;
    uint16_t base_tile, flags;
    const char *label;
} sim_city_tool_info;

/* Original demolition footprint for a center-marked tile (raw &0x400).
 * Offsets are relative to the selected cell. Zero means no expanded area. */
int sim_city_demolition_size(unsigned tile,int *dx,int *dy);
void sim_city_placement_init(sim_city_placement_state *state);
int sim_city_tool_info_get(unsigned tool,sim_city_tool_info *info);
/* Original outer placement2a6f8. x,y is the cursor's map cell; footprints use
 * top-left=(x,y)-(width-1)/2, including original asymmetric even-sized zones.
 * Returns original OUTER status:0 outside viewport/map,1 accepted/redraw,
 * 0x21 insufficient funds,0x22 must bulldoze,0x2c water refused,other direct
 * tool errors. Accepted/redraw can be a no-op. Query16 is not a mutation tool.
 */
uint16_t sim_city_place(uint16_t *map,uint32_t *funds,
    sim_city_placement_state *state,int x,int y,unsigned tool,int auto_bulldoze);
uint16_t sim_city_place_view(uint16_t *map,uint32_t *funds,
    sim_city_placement_state *state,int x,int y,unsigned tool,int auto_bulldoze,
    int view_x,int view_y,int view_width,int view_height);
/* Bridge for original-DGROUP simulation/persistence state. Copies its map to
 * aligned local storage; does not require caller alignment or alias uint8_t.
 * Uses viewportDS5f10/5f0e and296e/296c, writes map/funds/RNG/special4fd8 and
 * selected tool2b1a. Other simulation state is untouched. */
uint16_t sim_city_place_dgroup(uint8_t *dgroup,size_t size,int x,int y,unsigned tool);
#endif
