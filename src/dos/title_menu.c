#include "title_menu.h"

static const sim_title_rect rectangles[3] = {
    {112, 114, 211, 128},
    {220, 114, 296, 128},
    {150, 136, 258, 150}
};

void sim_title_menu_init(sim_title_menu *menu)
{
    if (menu) menu->selected = -1;
}

int sim_title_menu_step(sim_title_menu *menu, int direction)
{
    if (!menu) return -1;
    if (direction > 0) {
        ++menu->selected;
        if (menu->selected > 2) menu->selected = 0;
    } else if (direction < 0) {
        --menu->selected;
        if (menu->selected < 0) menu->selected = 2;
    }
    return menu->selected;
}

void sim_title_menu_select(sim_title_menu *menu, int index)
{
    if (menu) menu->selected = index >= 0 && index < 3 ? index : -1;
}

int sim_title_menu_accept(const sim_title_menu *menu)
{
    return menu && menu->selected >= 0 && menu->selected < 3 ? menu->selected + 1 : 0;
}

int sim_title_rect_get(int index, sim_title_rect *rect)
{
    if (!rect) return 0;
    rect->x0 = rect->y0 = rect->x1 = rect->y1 = 0;
    if (index < 0 || index >= 3) return 0;
    *rect = rectangles[index];
    return 1;
}

int sim_title_hit_test(int x, int y)
{
    int i;
    for (i = 0; i < 3; ++i) {
        const sim_title_rect *r = &rectangles[i];
        if (x >= r->x0 && x <= r->x1 && y >= r->y0 && y <= r->y1) return i;
    }
    return -1;
}

int sim_scenario_hit_test(int x, int y)
{
    if (x <= 0 || x >= 320 || y <= 24 || y >= 200) return 0;
    return 1 + x / 80 + (y > 117 ? 4 : 0);
}

int sim_scenario_center(int index, int *x, int *y)
{
    if (!x || !y) return 0;
    *x = *y = 0;
    if (index < 0 || index >= 8) return 0;
    *x = 40 + (index & 3) * 80;
    *y = 67 + (index / 4) * 84;
    return 1;
}
