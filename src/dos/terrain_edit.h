#ifndef SIMCITY_DOS_TERRAIN_EDIT_H
#define SIMCITY_DOS_TERRAIN_EDIT_H
#include <stddef.h>
#include <stdint.h>
#include "graphics_pack.h"

enum sim_terrain_edit_button {
    SIM_EDIT_DIRT=0,SIM_EDIT_TREES=1,SIM_EDIT_WATER=2,SIM_EDIT_CHANNEL=3,
    SIM_EDIT_FILL=4,SIM_EDIT_UNDO=5,SIM_EDIT_SMOOTH=6
};
enum sim_terrain_edit_result {
    SIM_EDIT_ACCEPTED=1,SIM_EDIT_SOUND_BUILD=2,SIM_EDIT_MAP_CHANGED=4
};
/* Native helpers use original1MB simulation memory and original undo storage:
 * baseline38a3:0, history4344:5dc0, countDGROUP32f4. No allocation or context.
 * Enter/exit change the original editor flag/speed and undo baseline. Platform
 * window registration/repainting is caller-owned; select city window1 on entry.
 */
int sim_terrain_edit_enter(uint8_t *memory,size_t size);
int sim_terrain_edit_exit(uint8_t *memory,size_t size);
/* Buttons0..3 select the brush;4 toggles Fill;5 undoes;6 smooths and records it.
 * Return bit flags above, zero for invalid input. Tools cost no funds. */
int sim_terrain_edit_select(uint8_t *memory,size_t size,unsigned button);
/* One original brush action at a map cell; the platform owns drag sampling.
 * Fill is one-shot and always requests original effect6, even for a no-op.
 * Normal paint requests effect6 only when replacing a different map word. */
int sim_terrain_edit_apply(uint8_t *memory,size_t size,int map_x,int map_y);
/* Original bounded400-span fill31e78, standalone primitive for tests/tools.
 * map is x-major120x100. Return changed-cell count; -1 invalid input. */
int sim_terrain_edit_flood(uint16_t *map,int x,int y,uint16_t replacement);
const char *sim_terrain_edit_label(unsigned button);
/* Exact mode6 toolbar hit rectangles325e4. Returns event110..116, or0. */
int sim_terrain_edit_hit(const uint8_t *dgroup,size_t size,int x,int y);
/* Original3289e editor shell without viewport/cache operations. Caller draws
 * the existing city viewport with sim_city_view.editor=1. tbar is the decoded
 * original MCGATBAR.PGF (two old-format image records), caller-owned. */
int sim_terrain_edit_shell_render(uint8_t *fb,const sim_graphics_pack *pack,
                                   const uint8_t *tbar,size_t tbar_size,
                                   const uint8_t *dgroup,size_t size,
                                   const char *city_name);
#endif
