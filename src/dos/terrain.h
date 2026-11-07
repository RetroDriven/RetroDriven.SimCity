#ifndef SIMCITY_DOS_TERRAIN_H
#define SIMCITY_DOS_TERRAIN_H
#include <stdint.h>

#define SIM_TERRAIN_WIDTH 120
#define SIM_TERRAIN_HEIGHT 100
#define SIM_TERRAIN_CELLS 12000

/* Original map ordering is x-major: tiles[x*100+y]. This component creates
 * terrain only, not a complete initialized city/simulation state. */
typedef struct sim_terrain {
    uint16_t tiles[SIM_TERRAIN_CELLS];
    uint16_t random[5];
    int16_t x, y, start_x, start_y, direction, initial_direction;
    int island;
} sim_terrain;

void sim_terrain_init(sim_terrain *terrain);
/* DOS 0x12824 replaces random[0] with the low timer word, retaining history.
 * Reusing a context therefore matches repeated maps in one DOS process. */
void sim_terrain_generate(sim_terrain *terrain, uint16_t timer_low);
/* Original editor Smooth entry12f46: two shore passes then two woods passes.
 * Retains/advances the same five-word RNG history; no new timer seed. */
void sim_terrain_smooth(sim_terrain *terrain);
/* Inclusive upper bound. Original terrain only uses bounds1..150. */
uint16_t sim_terrain_random(sim_terrain *terrain, unsigned maximum);
int sim_terrain_inside(int x, int y);
#endif
