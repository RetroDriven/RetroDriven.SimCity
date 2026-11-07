#ifndef SIMCITY_DOS_SPRITES_H
#define SIMCITY_DOS_SPRITES_H
#include <stddef.h>
#include <stdint.h>
#include "graphics_pack.h"

enum sim_sprite_type {
    SIM_SPRITE_TRAIN=1, SIM_SPRITE_HELICOPTER=2, SIM_SPRITE_AIRPLANE=3,
    SIM_SPRITE_SHIP=4, SIM_SPRITE_MONSTER=5, SIM_SPRITE_TORNADO=6,
    SIM_SPRITE_EXPLOSION=7
};
typedef struct sim_sprite_rect { int top,left,bottom,right; } sim_sprite_rect;
typedef struct sim_sprite_description {
    unsigned type,record,frame,width,height;
    int state,visible,map_pixel_x,map_pixel_y,screen_x,screen_y;
} sim_sprite_description;

/* Read original frame/position arrays and standard MCGA view geometry.
 * Train state5 is hidden; states6 upward skip that missing image.
 * Coordinates are original map pixels, not map-cell coordinates. */
int sim_sprite_describe(const sim_graphics_pack *pack,const uint8_t *dgroup,
                         size_t dgroup_size,unsigned type,
                         sim_sprite_description *sprite);
/* Original renderer 2a33c draw order: train, helicopter, ship, monster,
 * tornado, explosion, airplane. Zero pixels are transparent. Full tile
 * redraw makes the original dirty-tile cache invalidation unnecessary.
 * dgroup is read-only: simulation updates own the original sprite state.
 */
int sim_sprites_render(uint8_t *fb,const sim_graphics_pack *pack,
                        const uint8_t *dgroup,size_t dgroup_size);
/* Optional visible rectangles reproduce original window-region clipping.
 * Rectangles intersect the original viewport and must be disjoint to avoid
 * repeated work. NULL/zero selects the whole viewport. All screen coordinates
 * and rectangle bounds are inclusive. Invalid inputs return0 before drawing. */
int sim_sprites_render_clipped(uint8_t *fb,const sim_graphics_pack *pack,
                                const uint8_t *dgroup,size_t dgroup_size,
                                const sim_sprite_rect *clips,size_t clip_count);
#endif
