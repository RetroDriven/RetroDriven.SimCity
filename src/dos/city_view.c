#include "city_view.h"
#include "mcga_draw.h"
#include "ui_draw.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Original DGROUP:15aa + tile*2, entries for tile 0340..0373. */
static const uint16_t animation[52] = {
    8,8,8,8,8,8,8,8,4,4,4,4,4,4,4,4,4,4,4,4,
    8,8,8,8,8,8,8,8,861,862,863,864,865,866,867,867,
    869,870,871,868,873,874,875,876,877,878,879,880,
    881,882,883,872
};

void sim_city_view_init(sim_city_view *v)
{
    if (v == NULL) return;
    v->map_x = v->map_y = 0;
    v->screen_x = 64; v->screen_y = 35;
    v->columns = 31; v->rows = 18;
    v->tick = 0; v->power_blink = 0; v->editor = 0;
}

static unsigned select_tile(uint16_t *word, const sim_city_view *v)
{
    unsigned flags = *word, tile = flags & 1023u, group;
    if (!v->editor && (tile == 3 || tile == 4)) tile = 2;
    if ((flags & 0x400u) && !(flags & 0x8000u) && v->power_blink) {
        if (v->tick & 2) tile = 0x33b;
        return tile;
    }
    if (!(flags & 0x800u)) return tile;
    group = tile & 0xff0u;
    if (group == 0x50 || group == 0x90) return tile + (v->tick & 3u) * 16;
    if (group == 0x30) {
        group = tile & 0x38u;
        return group + ((v->tick + tile) & (group == 0x38 ? 7u : 3u));
    }
    if (tile >= 0x374) {
        if (tile >= 0x3a4 && tile <= 0x3b3)
            return ((((tile + 4) ^ v->tick) & 7u) ^ (tile - 4)) + 4;
        return tile ^ ((v->tick ^ tile) & 3u);
    }
    if (tile >= 0x35c) {
        tile = animation[tile - 0x340];
        *word = (uint16_t)((flags & 0xfc00u) | tile);
        return tile;
    }
    if (tile >= 0x340) {
        if (tile >= 0x354) return 0x354 + ((v->tick + tile + 4) & 7u);
        group = animation[tile - 0x340] - 1;
        return ((v->tick + tile) & group) + (tile & ~group);
    }
    return tile;
}

int sim_city_view_render(uint8_t *fb, const uint8_t *tiles,
                          uint16_t *map, const sim_city_view *v)
{
    unsigned x, y;
    if (fb == NULL || tiles == NULL || map == NULL || v == NULL) return 0;
    if (v->map_x < 0 || v->map_y < 0 || v->map_x >= 120 || v->map_y >= 100 ||
        v->screen_x < 0 || v->screen_y < 0 || v->screen_x >= 320 || v->screen_y >= 200 ||
        v->columns == 0 || v->rows == 0 ||
        v->columns > 120u - (unsigned)v->map_x || v->rows > 100u - (unsigned)v->map_y ||
        v->columns > (320u - (unsigned)v->screen_x) / 8 ||
        v->rows > (200u - (unsigned)v->screen_y) / 8) return 0;
    for (x = 0; x < v->columns; ++x)
        for (y = 0; y < v->rows; ++y)
            if ((map[((unsigned)v->map_x + x) * 100 + (unsigned)v->map_y + y] & 1023u) >= 960)
                return 0;
    for (x = 0; x < v->columns; ++x)
        for (y = 0; y < v->rows; ++y) {
            size_t index = ((unsigned)v->map_x + x) * 100 + (unsigned)v->map_y + y;
            unsigned tile = select_tile(&map[index],v);
            if (tile >= 960) return 0;
            if (!sim_mcga_blit(fb,v->screen_x + (int)x * 8,v->screen_y + (int)y * 8,
                               tiles + tile * 64,64,8,8,0,0,SIM_MCGA_COPY)) return 0;
        }
    return 1;
}

int sim_city_tool_select(uint8_t *fb, int index)
{
    int x, y;
    if (fb == NULL || index < 0 || index >= 14) return 0;
    /* Original 0x2fa98, internal mode6 (MCGA), table0x2fb4f. */
    x = 10 + (index & 1) * 26;
    y = 35 + (index / 2) * 17;
    return sim_ui_frame(fb,x,y,x+23,y+14,2,14);
}

int sim_city_shell_render(uint8_t *fb, const sim_graphics_pack *pack,
                          const char *name, uint32_t funds, const char *date,
                          const char *tool_label)
{
    sim_graphics_frame toolbar, demand;
    sim_graphics_span font;
    char text[64];
    size_t length, spaces;
    if (fb == NULL || pack == NULL ||
        sim_graphics_pack_frame(pack,1,0,&toolbar) != SIM_GRAPHICS_OK ||
        sim_graphics_pack_frame(pack,2,0,&demand) != SIM_GRAPHICS_OK ||
        sim_graphics_pack_section(pack,SIM_GRAPHICS_FONT_MASKS,&font) != SIM_GRAPHICS_OK ||
        toolbar.width != 56 || toolbar.height != 119 || demand.width != 48 || demand.height != 32)
        return 0;
    /* Original MCGA parameter copier0x2d996; city frame0x2fcca. */
    if (!sim_mcga_fill(fb,16,16,311,23,7,SIM_MCGA_COPY) ||
        !sim_mcga_fill(fb,8,24,311,34,8,SIM_MCGA_COPY) ||
        !sim_mcga_fill(fb,7,35,62,189,16,SIM_MCGA_COPY) ||
        !sim_mcga_blit(fb,7,35,toolbar.pixels.data,toolbar.pixels.size,56,119,0,0,SIM_MCGA_COPY) ||
        !sim_mcga_blit(fb,12,158,demand.pixels.data,demand.pixels.size,48,32,0,0,SIM_MCGA_COPY) ||
        !sim_mcga_fill(fb,64,180,304,189,17,SIM_MCGA_COPY) ||
        !sim_mcga_fill(fb,304,180,311,189,15,SIM_MCGA_COPY) ||
        !sim_ui_text(fb,font.data,304,181,"+",1,15) ||
        !sim_ui_frame(fb,5,13,314,192,3,0) ||
        !sim_ui_frame(fb,5,13,314,192,1,0) ||
        !sim_ui_frame(fb,64,35,311,178,-1,15)) return 0;
    {
        /* Original0x300fa maps button11 to cost14; disabled-button body
         * 0x1f260 ORs255 into alternating pixels, leaving the others intact. */
        static const unsigned prices[15] = {
            1,10,5,20,10,100,100,100,500,500,3000,5000,5000,10000,3000
        };
        int index, x, y;
        for (index = 0; index < 14; ++index) {
            int cost_index = index == 11 ? 14 : index;
            int x0 = 10 + (index & 1) * 26, y0 = 35 + (index / 2) * 17;
            if (funds >= prices[cost_index]) continue;
            for (y = y0; y <= y0 + 14; ++y)
                for (x = x0; x <= x0 + 23; ++x)
                    if ((x + y) & 1) fb[y * 320 + x] = 255;
        }
    }
    if (tool_label && !sim_ui_text(fb,font.data,64,181,tool_label,1,15)) return 0;
    /* DOS Funds:%-9s, date at titlebar.right-96. Comma formatting is the
     * ordinary positive-funds format; no negative/overflow balance claimed. */
    if (funds >= 1000000)
        snprintf(text,sizeof(text),"$%u,%03u,%03u",(unsigned)(funds/1000000),
                 (unsigned)((funds/1000)%1000),(unsigned)(funds%1000));
    else if (funds >= 1000)
        snprintf(text,sizeof(text),"$%u,%03u",(unsigned)(funds/1000),(unsigned)(funds%1000));
    else snprintf(text,sizeof(text),"$%u",(unsigned)funds);
    {
        char label[32];
        snprintf(label,sizeof(label),"Funds: %-9.9s",text);
        if (!sim_ui_text(fb,font.data,8,25,label,7,8)) return 0;
    }
    if (date && !sim_ui_text(fb,font.data,215,16,date,8,7)) return 0;
    if (name) {
        length = strlen(name);
        if (length > 24) length = 24;
        spaces = (24 - length) / 2;
        memset(text,' ',spaces);
        memcpy(text+spaces,name,length);
        text[spaces+length] = '\0';
        if (!sim_ui_text(fb,font.data,24,16,text,8,7)) return 0;
    }
    return sim_mcga_mask(fb,8,16,font.data+15*64,64,8,8,0,0,8,7,SIM_MCGA_COPY);
}
