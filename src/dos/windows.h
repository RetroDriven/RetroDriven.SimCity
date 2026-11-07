#ifndef SIMCITY_DOS_WINDOWS_H
#define SIMCITY_DOS_WINDOWS_H
#include <stddef.h>
#include <stdint.h>
enum {SIM_WINDOW_EDITOR=1,SIM_WINDOW_MAPS=2,SIM_WINDOW_GRAPHS=3};
enum {SIM_WINDOW_POSITION=0,SIM_WINDOW_RESIZE=1};
typedef struct {int top,left,bottom,right;} SimWindowRect;
typedef struct {int mode,window,start_x,start_y,last_x,last_y,dx,dy;SimWindowRect original,preview;} SimWindowDrag;
/* Persistent2f066/2f16a/2efc0 state; frontend redraws/registers controls. */
int sim_windows_activate(uint8_t *memory,size_t size,unsigned window);
int sim_windows_close(uint8_t *memory,size_t size,unsigned window);
int sim_windows_close_current(uint8_t *memory,size_t size);
int sim_windows_hide(uint8_t *memory,size_t size);
/* Z-order2964 is frontmost; output rendering order is back to front. */
int sim_windows_draw_order(const uint8_t *memory,size_t size,unsigned out[3]);
int sim_windows_hit(const uint8_t *memory,size_t size,int x,int y);
/* part0 outer,part1 caption,part2 close button. Rectangles are inclusive. */
int sim_window_rectangle(const uint8_t *memory,size_t size,unsigned window,unsigned part,SimWindowRect *rect);
/* Begin Position for active window; Resize activates Editor first. Update
 * samples original screen cursor points, preserving DOS snapping/clamps.
 * preview only draws an outline; persistent rectangles change at commit.
 * Position's original horizontal algebra includes a16pixel bias. */
int sim_window_drag_begin(uint8_t *memory,size_t size,unsigned mode,int cursor_x,int cursor_y,SimWindowDrag *drag);
int sim_window_drag_update(const uint8_t *memory,size_t size,int cursor_x,int cursor_y,SimWindowDrag *drag);
/* Exact2pixel originalXOR outline; calling twice restores framebuffer. */
int sim_window_preview_render(uint8_t *fb,const SimWindowDrag *drag);
int sim_window_drag_commit(uint8_t *memory,size_t size,const SimWindowDrag *drag);
#endif
