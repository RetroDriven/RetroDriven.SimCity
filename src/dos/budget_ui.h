#ifndef SIMCITY_DOS_BUDGET_UI_H
#define SIMCITY_DOS_BUDGET_UI_H
#include <stddef.h>
#include <stdint.h>

/* Shared original flat1MB city image. Call simulation_budget prepare first. */
int sim_budget_ui_render(uint8_t *fb,const uint8_t *font,const uint8_t *memory,size_t size);
/* Original mouse held-left means step1; alternate/buttonless means step10. */
int sim_budget_ui_adjust(uint8_t *memory,size_t size,unsigned event,unsigned step);
int sim_budget_ui_hit(int x,int y); /* Original event0x800..808, or0 */
int sim_budget_ui_cursor(unsigned focus,int *x,int *y);
int sim_budget_ui_step(uint8_t *memory,size_t size,int direction);
#endif
