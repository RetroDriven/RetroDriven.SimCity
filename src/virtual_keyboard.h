#ifndef SIM_VIRTUAL_KEYBOARD_H
#define SIM_VIRTUAL_KEYBOARD_H
#include <stdint.h>
#include "dos/name_edit.h"
typedef struct {int row,column,lowercase;} SimVirtualKeyboard;
enum {SIM_VK_CLEAR=256,SIM_VK_CANCEL,SIM_VK_DONE};
void sim_virtual_keyboard_init(SimVirtualKeyboard *keyboard);
void sim_virtual_keyboard_step(SimVirtualKeyboard *keyboard,int dx,int dy);
unsigned sim_virtual_keyboard_selected(const SimVirtualKeyboard *keyboard);
int sim_virtual_keyboard_hit(SimVirtualKeyboard *keyboard,int x,int y);
void sim_virtual_keyboard_render(uint8_t *fb,const uint8_t *font,const SimVirtualKeyboard *keyboard,const sim_name_edit *edit,int blink);
#endif
