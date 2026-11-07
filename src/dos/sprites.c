/* Original standard MCGA sprite composition, renderer 2a33c and clipped
 * transparent-zero primitive 253d2 -> 1f444. */
#include "sprites.h"
#include <string.h>

static unsigned word(const uint8_t *d,unsigned at)
{ return d[at]|((unsigned)d[at+1]<<8); }
static int signed_word(const uint8_t *d,unsigned at)
{ unsigned value=word(d,at);return value>=32768?(int)value-65536:(int)value; }
static int valid(const uint8_t *d,size_t size)
{ return d && size>=65536; }

int sim_sprite_describe(const sim_graphics_pack *pack,const uint8_t *d,
                         size_t size,unsigned type,sim_sprite_description *s)
{
    sim_graphics_frame first,frame;
    int state,tile_width,tile_height;
    if (!s) return 0;
    memset(s,0,sizeof(*s));
    if (!valid(d,size) || !pack || type<1 || type>7) return 0;
    s->type=type;s->record=type+8;
    state=signed_word(d,0xd19e + type*2);s->state=state;
    s->map_pixel_x=signed_word(d,0x527c+type*2);
    s->map_pixel_y=signed_word(d,0x52a8+type*2);
    /* Original2a33c always omits train state5 and closes that index gap.
     * D1CE controls orientation generation in updater36416, not this mapping. */
    if (!state || (type==1 && state==5)) return 1;
    if (state<0) return 0;
    if (type==1 && state>5) --state;
    s->frame=(unsigned)(state-1);
    if (sim_graphics_pack_frame(pack,s->record,0,&first)!=SIM_GRAPHICS_OK ||
        sim_graphics_pack_frame(pack,s->record,s->frame,&frame)!=SIM_GRAPHICS_OK ||
        first.width!=frame.width || first.height!=frame.height ||
        frame.kind!=8 || frame.width>=256 || frame.height>=256) return 0;
    tile_width=signed_word(d,0x2b1c);tile_height=signed_word(d,0x2b1e);
    if (tile_width<=0 || tile_height<=0) return 0;
    s->width=frame.width;s->height=frame.height;
    s->screen_x=(tile_width-(int)frame.width)/2+s->map_pixel_x-
        signed_word(d,0x5f10)*tile_width+signed_word(d,0x2992);
    s->screen_y=(tile_height-(int)frame.height)/2+s->map_pixel_y-
        signed_word(d,0x5f0e)*tile_height+signed_word(d,0x2990);
    s->visible=1;return 1;
}

int sim_sprites_render_clipped(uint8_t *fb,const sim_graphics_pack *pack,
                                const uint8_t *d,size_t size,
                                const sim_sprite_rect *clips,size_t count)
{
    static const unsigned order[7]={1,2,4,5,6,7,3};
    sim_sprite_description sprites[7];
    sim_sprite_rect viewport;
    size_t i,j;
    if (!fb || !pack || !valid(d,size) || (count && !clips)) return 0;
    if (!d[0x2969]) return 1;
    if (d[0x2950]!=6) return 0;
    viewport.top=signed_word(d,0x2990);viewport.left=signed_word(d,0x2992);
    viewport.bottom=signed_word(d,0x2994);viewport.right=signed_word(d,0x2996);
    if (viewport.top<0 || viewport.left<0 || viewport.bottom>=200 ||
        viewport.right>=320 || viewport.top>viewport.bottom ||
        viewport.left>viewport.right) return 0;
    for (j=0;j<count;++j) if (clips[j].top>clips[j].bottom ||
        clips[j].left>clips[j].right) return 0;
    for (i=0;i<7;++i)
        if (!sim_sprite_describe(pack,d,size,order[i],sprites+i)) return 0;
    if (!count) { clips=&viewport;count=1; }
    for (i=0;i<7;++i) if (sprites[i].visible) {
        sim_graphics_frame frame;
        const sim_sprite_description *s=sprites+i;
        (void)sim_graphics_pack_frame(pack,s->record,s->frame,&frame);
        for (j=0;j<count;++j) {
            int top=s->screen_y,left=s->screen_x;
            int bottom=top+(int)s->height-1,right=left+(int)s->width-1,x,y;
            if (top<viewport.top) top=viewport.top;
            if (left<viewport.left) left=viewport.left;
            if (bottom>viewport.bottom) bottom=viewport.bottom;
            if (right>viewport.right) right=viewport.right;
            if (top<clips[j].top) top=clips[j].top;
            if (left<clips[j].left) left=clips[j].left;
            if (bottom>clips[j].bottom) bottom=clips[j].bottom;
            if (right>clips[j].right) right=clips[j].right;
            for (y=top;y<=bottom;++y) for (x=left;x<=right;++x) {
                uint8_t pixel=frame.pixels.data[(size_t)(y-s->screen_y)*s->width+(unsigned)(x-s->screen_x)];
                if (pixel) fb[y*320+x]=pixel;
            }
        }
    }
    return 1;
}

int sim_sprites_render(uint8_t *fb,const sim_graphics_pack *pack,
                        const uint8_t *d,size_t size)
{ return sim_sprites_render_clipped(fb,pack,d,size,NULL,0); }
