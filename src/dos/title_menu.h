#ifndef SIMCITY_DOS_TITLE_MENU_H
#define SIMCITY_DOS_TITLE_MENU_H

/* Portable title selection state recovered from DOS routine 0x22c84.
 * Rectangles are the MCGA parameter table copied by 0x2d996, not image guesses.
 * Coordinates are DOS 320x200 pixels, with inclusive right/bottom bounds.
 * Controller-to-key/mouse mapping belongs to the platform adapter.
 */
typedef struct sim_title_menu { int selected; } sim_title_menu;
typedef struct sim_title_rect { int x0, y0, x1, y1; } sim_title_rect;

void sim_title_menu_init(sim_title_menu *menu);
int sim_title_menu_step(sim_title_menu *menu, int direction);
void sim_title_menu_select(sim_title_menu *menu, int index);
int sim_title_menu_accept(const sim_title_menu *menu);
int sim_title_rect_get(int index, sim_title_rect *rect);
int sim_title_hit_test(int x, int y);

/* MCGA scenario selector 0x22e62. Hit result is DOS scenario1..8, or0.
 * Center takes a zero-based index and returns success/failure.
 */
int sim_scenario_hit_test(int x, int y);
int sim_scenario_center(int index, int *x, int *y);

#endif
