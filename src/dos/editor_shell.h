#ifndef SIMCITY_DOS_EDITOR_SHELL_H
#define SIMCITY_DOS_EDITOR_SHELL_H
#include <stddef.h>
#include <stdint.h>
#include "graphics_pack.h"
#include "windows.h"
#include "message_dispatch.h"
/* Original dynamic MCGA city frame2fcca and current toolbar/status data.
 * Normal city mode only; terrain mode uses terrain_edit.c.
 * Draw the world/sprites after this, then overlay the other visible windows.
 * Does not mutate simulation/message state during ordinary host repaints. */
int sim_editor_shell_render(uint8_t *fb,const sim_graphics_pack *pack,const uint8_t *memory,size_t size);
int sim_editor_tool_rectangle(const uint8_t *memory,size_t size,unsigned tool,SimWindowRect *rect);
int sim_editor_tool_hit(const uint8_t *memory,size_t size,int x,int y);
/* Original full city repaint recalculates message character capacity. Call
 * once after geometry changes, not on every host framebuffer repaint. */
/* One original2fcca repaint message-bookkeeping transaction. This resets
 * last/icon then dispatches pending320c with notify0; it does not fabricate
 * a new scroll x when pending is0. Also preserves original DGROUP money/color/toolbar scratch writes;
 * allocated screen-capture caches remain frontend-owned. */
int sim_editor_shell_repaint(uint8_t *memory,size_t size,uint32_t clock,SimMessageEffects *effects);
int sim_editor_shell_metrics(uint8_t *memory,size_t size);
#endif
