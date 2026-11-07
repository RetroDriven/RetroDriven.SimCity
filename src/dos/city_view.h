#ifndef SIMCITY_DOS_CITY_VIEW_H
#define SIMCITY_DOS_CITY_VIEW_H
#include <stdint.h>
#include "graphics_pack.h"

typedef struct sim_city_view {
    int map_x, map_y;
    int screen_x, screen_y;
    unsigned columns, rows;
    uint16_t tick;
    int power_blink;
    int editor;
} sim_city_view;

/* Standard MCGA viewport observed in the original reference city.
 * The caller supplies the scroll origin; map data uses tiles[x*100+y]. */
void sim_city_view_init(sim_city_view *view);
/* Draw the full viewport. This reproduces DOS tile selection/animation but
 * intentionally omits its dirty-tile cache. tiles61440 is the graphics pack's
 * fixed 960x64-byte section. Animation can advance some map tile words.
 * Invalid dimensions, scroll ranges, pointers or tile IDs return zero.
 */
int sim_city_view_render(uint8_t *fb, const uint8_t *tiles61440,
                          uint16_t *map12000, const sim_city_view *view);
/* Static standard MCGA city frame and authentic toolbar assets. The menu bar
 * at y0..11 is caller-owned. Text arguments may be NULL to omit those labels.
 * Demand meter is its original base image; no simulated demand is fabricated.
 */
int sim_city_shell_render(uint8_t *fb, const sim_graphics_pack *pack,
                          const char *name, uint32_t funds, const char *date,
                          const char *tool_label);
/* Original internal mode6 (MCGA) geometry. index0..13; yellow selection frame. */
int sim_city_tool_select(uint8_t *fb, int index);
#endif
