#ifndef SIMCITY_DOS_MENUS_H
#define SIMCITY_DOS_MENUS_H
#include <stddef.h>
#include <stdint.h>

typedef enum sim_menu_kind {
    SIM_MENU_SYSTEM=0, SIM_MENU_OPTIONS=1, SIM_MENU_DISASTERS=2,
    SIM_MENU_WINDOWS=3, SIM_MENU_SPEED=4
} sim_menu_kind;

typedef struct sim_menus {
    const char *title[4];
    const char *item[5][16];
    unsigned count[5];
} sim_menus;

typedef struct sim_menu_flags {
    int auto_bulldoze,auto_budget,auto_goto,sound,music;
    int animate_all,frequent_animation,disable_disasters,speed;
} sim_menu_flags;

typedef struct sim_menu_layout {
    int x0,y0,x1,y1; /* Original capture rectangle, inclusive */
    int text_x,text_y,width_chars,rows;
} sim_menu_layout;

/* Caller retains the decoded original MESSAGE.PTF buffer. No allocation. */
int sim_menus_open(sim_menus *menus,const uint8_t *messages,size_t size);
int sim_menus_bar_x(unsigned menu);
int sim_menus_bar_hit(int x,int y);
int sim_menus_layout(const sim_menus *menus,sim_menu_kind menu,sim_menu_layout *layout);
int sim_menus_next(const sim_menus *menus,sim_menu_kind menu,int selected,int direction);
int sim_menus_hit(const sim_menus *menus,sim_menu_kind menu,int x,int y);
int sim_menus_render(uint8_t *fb,const uint8_t *font,const sim_menus *menus,
                     sim_menu_kind menu,int selected,const sim_menu_flags *flags);
/* Original event IDs: menu*0x10 + row. Speed popup returns speed0..4. */
int sim_menus_event(sim_menu_kind menu,int selected);
int sim_menus_flags_dgroup(sim_menu_flags *flags,const uint8_t *dgroup,size_t size);
#endif
