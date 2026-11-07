#ifndef SIMCITY_DOS_NEW_CITY_H
#define SIMCITY_DOS_NEW_CITY_H
#include <stdint.h>

/* Selection behavior from DOS dialog 0x26dac. No map generation occurs here. */
typedef struct sim_new_city {
    char name[19];
    int difficulty; /* Easy0, Medium1, Hard2 */
    int focus;      /* Easy0, Medium1, Hard2, OK3, name4 */
    int accepted;
} sim_new_city;

typedef enum sim_new_city_action {
    SIM_NEW_CITY_NONE = 0, SIM_NEW_CITY_ACCEPT, SIM_NEW_CITY_EDIT_NAME
} sim_new_city_action;

void sim_new_city_init(sim_new_city *city);
int sim_new_city_step(sim_new_city *city, int direction);
sim_new_city_action sim_new_city_event(sim_new_city *city, unsigned event);
sim_new_city_action sim_new_city_key(sim_new_city *city, unsigned ascii);
sim_new_city_action sim_new_city_activate(sim_new_city *city);
void sim_new_city_set_name(sim_new_city *city, const char *name);
uint32_t sim_new_city_funds(const sim_new_city *city);
int sim_new_city_cursor(int focus, int *x, int *y);
const char *sim_new_city_level_name(int index);
int sim_new_city_hit_test(int x, int y); /* returns event0x800..804, or0 */
int sim_new_city_render(uint8_t *fb, const uint8_t *font8192,
                        const sim_new_city *city);
#endif
