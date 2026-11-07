#ifndef SIMCITY_DOS_MESSAGE_POPUP_H
#define SIMCITY_DOS_MESSAGE_POPUP_H
#include <stddef.h>
#include <stdint.h>
typedef struct {int x0,y0,x1,y1,button_x0,button_y0,button_x1,button_y1;unsigned timeout_ticks;} SimMessagePopupLayout;
/* Text has original preformatted newlines; no wrapping is applied. Timed
 * notifications use original 540tick timeout. Optional mask is graphicsrecord7
 * including fourbyte width/height header when original DG2955 selects a line. */
int sim_message_popup_layout(const uint8_t *memory,size_t size,const char *text,int timed,SimMessagePopupLayout *layout);
int sim_message_popup_render(uint8_t *fb,const uint8_t *font,const uint8_t *memory,size_t size,const char *text,int timed,const uint8_t *mask,size_t mask_size);
int sim_message_popup_key(unsigned original_key);
#endif
