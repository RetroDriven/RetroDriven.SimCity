#include "new_city.h"
#include "ui_draw.h"
#include "mcga_draw.h"
#include <string.h>

void sim_new_city_init(sim_new_city *city)
{
    if (!city) return;
    memset(city, 0, sizeof(*city));
    memcpy(city->name, "HERESVILLE", 11);
}

int sim_new_city_step(sim_new_city *city, int direction)
{
    if (!city) return -1;
    if (direction > 0) city->focus = (city->focus + 1) % 5;
    else if (direction < 0) city->focus = city->focus == 0 ? 4 : city->focus - 1;
    return city->focus;
}

sim_new_city_action sim_new_city_event(sim_new_city *city, unsigned event)
{
    if (!city) return SIM_NEW_CITY_NONE;
    if (event >= 0x800 && event <= 0x802) city->difficulty = (int)(event - 0x800);
    else if (event == 0x803) {
        city->accepted = 1;
        return SIM_NEW_CITY_ACCEPT;
    } else if (event == 0x804) return SIM_NEW_CITY_EDIT_NAME;
    return SIM_NEW_CITY_NONE;
}

sim_new_city_action sim_new_city_key(sim_new_city *city, unsigned ascii)
{
    if (!city) return SIM_NEW_CITY_NONE;
    if (ascii >= 'a' && ascii <= 'z') ascii -= 'a' - 'A';
    switch (ascii) {
    case 'E': return sim_new_city_event(city, 0x800);
    case 'M': return sim_new_city_event(city, 0x801);
    case 'H': return sim_new_city_event(city, 0x802);
    case '+': sim_new_city_step(city, 1); break;
    case '-': sim_new_city_step(city, -1); break;
    case ' ': return SIM_NEW_CITY_EDIT_NAME;
    case 'O': case '\r': case '\n': return sim_new_city_event(city, 0x803);
    default: break;
    }
    return SIM_NEW_CITY_NONE;
}

sim_new_city_action sim_new_city_activate(sim_new_city *city)
{
    if (!city || city->focus < 0 || city->focus > 4) return SIM_NEW_CITY_NONE;
    return sim_new_city_event(city, 0x800u + (unsigned)city->focus);
}

void sim_new_city_set_name(sim_new_city *city, const char *name)
{
    unsigned i = 0;
    if (!city || !name) return;
    while (i < 18 && name[i]) { city->name[i] = name[i]; ++i; }
    city->name[i] = 0;
}

uint32_t sim_new_city_funds(const sim_new_city *city)
{
    static const uint32_t funds[3] = {20000, 10000, 5000};
    return city && city->difficulty >= 0 && city->difficulty < 3 ? funds[city->difficulty] : 0;
}

int sim_new_city_cursor(int focus, int *x, int *y)
{
    static const int positions[5][2] = {{124,92}, {124,108}, {124,124}, {156,140}, {100,60}};
    if (!x || !y || focus < 0 || focus > 4) return 0;
    *x = positions[focus][0]; *y = positions[focus][1];
    return 1;
}

const char *sim_new_city_level_name(int index)
{
    static const char *levels[3] = {"Easy", "Medium", "Hard"};
    return index >= 0 && index < 3 ? levels[index] : "";
}

int sim_new_city_hit_test(int x, int y)
{
    if (x >= 120 && x <= 127) {
        if (y >= 88 && y <= 95) return 0x800;
        if (y >= 104 && y <= 111) return 0x801;
        if (y >= 120 && y <= 127) return 0x802;
    }
    if (x >= 144 && x <= 159 && y >= 136 && y <= 143) return 0x803;
    if (x >= 88 && x <= 231 && y >= 56 && y <= 63) return 0x804;
    return 0;
}

int sim_new_city_render(uint8_t *fb, const uint8_t *font, const sim_new_city *city)
{
    char name[19];
    unsigned i;
    int good;
    if (!fb || !font || !city) return 0;
    for (i = 0; i < 18 && city->name[i]; ++i) name[i] = city->name[i];
    for (; i < 18; ++i) name[i] = ' ';
    name[18] = 0;
    /* The original dialog's call sequence, with the exact centered MCGA
     * rectangle and font positions. Caller draws the underlying city canvas. */
    good = sim_mcga_fill(fb, 80, 40, 239, 159, 15, SIM_MCGA_COPY) &&
           sim_ui_frame(fb, 80, 40, 239, 159, 4, 9) &&
           sim_ui_text(fb, font, 104, 72, "Game Play Level", 9, 15);
    for (i = 0; i < 3; ++i) {
        good = good && sim_ui_text(fb, font, 136, 88+(int)i*16,
                                  sim_new_city_level_name((int)i), 9, 15) &&
                       sim_ui_button(fb, font, 15, (uint16_t)(11+i*2), " ", 9, 15);
    }
    good = good && sim_ui_text(fb, font, 88, sim_ui_y(0x805), "SIMCITY city name:", 9, 15) &&
                   sim_ui_button(fb, font, 11, 7, name, 11, 15) &&
                   sim_ui_button(fb, font, 18, 17, "OK", 9, 11);
    for (i = 0; i < 3; ++i)
        good = good && sim_ui_text(fb, font, 120, 88+(int)i*16,
                                  city->difficulty == (int)i ? "*" : " ", 9, 11);
    return good;
}
