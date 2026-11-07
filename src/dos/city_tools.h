#ifndef SIMCITY_DOS_CITY_TOOLS_H
#define SIMCITY_DOS_CITY_TOOLS_H
#include <stdint.h>

enum sim_city_tool {
    SIM_CITY_CONNECT=0, SIM_CITY_BULLDOZE=1, SIM_CITY_ROAD=2,
    SIM_CITY_WIRE=3, SIM_CITY_RAIL=4, SIM_CITY_PARK=5,
    SIM_CITY_RESIDENTIAL=6, SIM_CITY_COMMERCIAL=7, SIM_CITY_INDUSTRIAL=8,
    SIM_CITY_POLICE=9, SIM_CITY_FIRE=10, SIM_CITY_STADIUM=11,
    SIM_CITY_NUCLEAR=12, SIM_CITY_SEAPORT=13, SIM_CITY_AIRPORT=14,
    SIM_CITY_COAL=15, SIM_CITY_QUERY=16
};
#define SIM_CITY_TOOL_UNSUPPORTED 0xfffeu
/* Reconstructed low-level DOS tool path0x3331d for bulldoze/road/wire/rail.
 * Map is120x100, x-major. Funds retain the original unsigned32-bit bit pattern.
 * Return is original status (0,0x21,0x2d,0x2e), not a success boolean:
 * status0 may mean unchanged occupied tile; compare map/funds if needed.
 * This legacy0.3entry keeps its zone-center guard (flag0x400), returning
 * UNSUPPORTED without mutation. Use sim_city_place in placement.h for the
 * recovered complete outer wrapper and multi-tile demolition.
 */
uint16_t sim_city_tool_apply(uint16_t *map, uint32_t *funds, int x, int y,
                            unsigned tool, int auto_bulldoze);
/* Literal original low-level dispatcher, including center-marker input.
 * Does not expand multi-tile demolition. Prefer sim_city_place from placement.h
 * for complete tools; this entry is used by that recovered outer wrapper. */
uint16_t sim_city_tool_apply_lowlevel(uint16_t *map,uint32_t *funds,int x,int y,
                                    unsigned tool,int auto_bulldoze);
/* Original standard MCGA toolbar helper0x2fa98, tool IDs1..15.
 * Coal15 shares power-plant toolbar cell12, as original30018 selects it.
 * Inclusive rectangle in x0,y0,x1,y1 order. */
int sim_city_tool_rectangle(unsigned tool, int xyxy[4]);
#endif
