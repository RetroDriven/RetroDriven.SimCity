/* Portable standard MCGA Maps repaint31490 and overlay paths2d34c..2d945.
 * Original dirty caches/window manager registration are replaced by repaint. */
#include "maps.h"
#include "mcga_draw.h"
#include "ui_draw.h"
#include <string.h>

#define DG 0x4d2c0u
static unsigned word(const uint8_t *d,unsigned at)
{ return d[at]|((unsigned)d[at+1]<<8); }
static int signed_word(const uint8_t *d,unsigned at)
{ unsigned n=word(d,at);return n>=32768?(int)n-65536:(int)n; }
static void put_word(uint8_t *d,unsigned at,int value)
{ d[at]=(uint8_t)value;d[at+1]=(uint8_t)((unsigned)value>>8); }
static uint32_t far_address(const uint8_t *d,unsigned at)
{ return word(d,at)+(uint32_t)word(d,at+2)*16; }
static int fill_rect(uint8_t *fb,const uint8_t *d,unsigned at,unsigned color)
{ return sim_mcga_fill(fb,signed_word(d,at+2),signed_word(d,at),signed_word(d,at+6),signed_word(d,at+4),(uint8_t)color,SIM_MCGA_COPY); }
static int frame_rect(uint8_t *fb,const uint8_t *d,unsigned at,int thickness,unsigned color)
{ return sim_ui_frame(fb,signed_word(d,at+2),signed_word(d,at),signed_word(d,at+6),signed_word(d,at+4),thickness,(uint8_t)color); }

int sim_maps_configure_mcga(uint8_t *d,size_t size)
{
    static const int rectangles[6][4]={
        {25,88,36,291},{25,84,36,91},{21,80,140,295},
        {37,140,136,259},{37,83,137,137},{37,261,137,292}
    };
    unsigned i,j;
    if (!d || size<65536) return 0;
    for (i=0;i<6;++i) for (j=0;j<4;++j) put_word(d,0x2a70+i*8+j*2,rectangles[i][j]);
    put_word(d,0x2b14,1);put_word(d,0x2b16,1);put_word(d,0x2b18,1);return 1;
}

const char *sim_maps_label(unsigned layer,unsigned population,unsigned protection)
{
    static const char *const labels[12]={NULL," City Form "," Power Grid "," Transportation ",
        " Population Density "," Traffic Density "," Pollution Map "," Crime Map "," Land Value ",
        " Police Protection "," Fire Protection "," Population Growth "};
    if (layer<1 || layer>9) return NULL;
    if (layer==4 && population==2) layer=11;
    if (layer==9 && protection==2) layer=10;
    return labels[layer];
}

int sim_maps_hit(const uint8_t *d,size_t size,int x,int y)
{
    int index,left,top;
    if (!d || size<65536 || d[0x2950]!=6) return 0;
    for (index=0;index<9;++index) {
        left=signed_word(d,0x2a92)+(index&1)*27;
        top=signed_word(d,0x2a90)+(index/2)*19;
        if (x>=left+3 && x<=left+27 && y>=top+3 && y<=top+19) return 0x411+index;
    }
    return 0;
}

int sim_maps_center(uint8_t *d,size_t size,int map_x,int map_y)
{
    int columns,rows,x,y;
    unsigned bits;
    if (!d || size<65536 || map_x<-32768 || map_x>32767 || map_y<-32768 || map_y>32767) return 0;
    columns=signed_word(d,0x296e);rows=signed_word(d,0x296c);
    if (columns<1 || columns>120 || rows<1 || rows>100) return 0;
    bits=(unsigned)(map_x-columns/2)&65535;x=bits>=32768?(int)bits-65536:(int)bits;
    bits=(unsigned)(map_y-rows/2)&65535;y=bits>=32768?(int)bits-65536:(int)bits;
    bits=(unsigned)(x+columns)&65535;
    if (x<0) x=0;else if ((bits>=32768?(int)bits-65536:(int)bits)>120) x=120-columns;
    bits=(unsigned)(y+rows)&65535;
    if (y<0) y=0;else if ((bits>=32768?(int)bits-65536:(int)bits)>100) y=100-rows;
    put_word(d,0x5f10,x);put_word(d,0x5f0e,y);return 1;
}

int sim_maps_viewport_render(uint8_t *fb,const uint8_t *d,size_t size)
{
    int x,y,right,bottom;
    if (!fb || !d || size<65536) return 0;
    if (!d[0x296a]) return 1;
    x=signed_word(d,0x5f10)*signed_word(d,0x2b16)+signed_word(d,0x2a8a);
    y=signed_word(d,0x5f0e)*signed_word(d,0x2b14)+signed_word(d,0x2a88);
    right=x+signed_word(d,0x296e)*signed_word(d,0x2b16)-1;
    bottom=y+signed_word(d,0x296c)*signed_word(d,0x2b14)-1;
    if (x<0 || y<0 || right>=320 || bottom>=200 || right<x || bottom<y) return 0;
    /* Original23f1e keeps corners out of the two side rectangles, so XOR
     * touches every standard border pixel once rather than canceling it. */
    return sim_mcga_fill(fb,x+1,y+2,x,bottom-2,255,SIM_MCGA_XOR) &&
        sim_mcga_fill(fb,right,y+2,right-1,bottom-2,255,SIM_MCGA_XOR) &&
        sim_mcga_fill(fb,x,y+1,right,y,255,SIM_MCGA_XOR) &&
        sim_mcga_fill(fb,x,bottom,right,bottom-1,255,SIM_MCGA_XOR);
}

static int overview(uint8_t *fb,const uint8_t *d,const uint8_t *colors,
                      const uint8_t *font)
{
    static const char glyphs[]=" RHASMT";
    unsigned x,y,type;
    int left=signed_word(d,0x2a8a),top=signed_word(d,0x2a88);
    int tile_width=signed_word(d,0x2b1c),tile_height=signed_word(d,0x2b1e);
    if (tile_width<=0 || tile_height<=0 || left<0 || top<0 || left+120>320 || top+100>200) return 0;
    for (x=0;x<120;++x) for (y=0;y<100;++y)
        if ((word(d,0x6b78+(x*100+y)*2)&1023)>=960) return 0;
    for (x=0;x<120;++x) for (y=0;y<100;++y)
        fb[(top+(int)y)*320+left+(int)x]=colors[word(d,0x6b78+(x*100+y)*2)&1023];
    for (type=1;type<7;++type) if (word(d,0xd19e + type*2)) {
        char text[2]={glyphs[type],0};
        int sx=signed_word(d,0x527c+type*2)/tile_width;
        int sy=signed_word(d,0x52a8+type*2)/tile_height;
        if (sx<0) sx=0;else if (sx>112) sx=112;
        if (sy<0) sy=0;else if (sy>92) sy=92;
        if (!sim_ui_text(fb,font,left+sx,top+sy,text,15,0)) return 0;
    }
    return 1;
}

static uint8_t overlay_color(const uint8_t *d,unsigned value,int signed_scale)
{
    unsigned index=d[0x28ec+value+(signed_scale?16:0)];
    return d[0x28ad+index*2]; /* high byte of original MCGA fill color word */
}

static int overlay(uint8_t *fb,const uint8_t *d,const uint8_t *memory,size_t size,
                     unsigned layer,unsigned population,unsigned protection)
{
    int left=signed_word(d,0x2a8a),top=signed_word(d,0x2a88);
    unsigned x,y;
    uint32_t address;
    if (layer==1) return 1;
    if (layer==2 || layer==3) {
        for (x=0;x<120;++x) for (y=0;y<100;++y) {
            unsigned tile=word(d,0x6b78+(x*100+y)*2),id=tile&1023;
            int color=-1;
            if (layer==3) {
                if (id>=64 && id<=239) {
                    if (id<=206) color=171;
                    else if (id>=221 && id<=236) color=170;
                    else if (id>=237) color=0;
                }
            } else if (tile&0xc400) {
                if (tile&0x400) {
                    if (!(tile&0x8000)) color=0;
                    else color=(id==0x330 || id==0x2ee)?160:39;
                } else color=33;
            }
            if (color>=0) fb[(top+(int)y)*320+left+(int)x]=(uint8_t)color;
        }
        return 1;
    }
    if ((layer==4 && population==2) || layer==9) {
        int signed_scale=layer==4;
        /* Original31b9e literal far pointers for coarse arrays. */
        address=layer==4?0x38a30u+0x99c0u:
            (protection==1?0x43440u+0x9b14u:0x38a30u+0xa884u);
        if (address+15*13*2>size) return 0;
        for (x=0;x<15;++x) for (y=0;y<13;++y) {
            int value=signed_word(memory,address+(x*13+y)*2);
            unsigned yy,xx,height=y==12?4:8;
            if (!value) continue;
            if (signed_scale) { if(value>127)value=127;if(value<-127)value=-127; }
            else if (value>255) value=255;
            for (xx=0;xx<8;++xx) for (yy=0;yy<height;++yy)
                fb[(top+(int)(y*8+yy))*320+left+(int)(x*8+xx)]=overlay_color(d,((unsigned)value&240)>>4,signed_scale);
        }
    } else {
        address=far_address(d,0x3284+layer*4);
        if (address+3000>size) return 0;
        for (x=0;x<60;++x) for (y=0;y<50;++y) {
            unsigned value=memory[address+x*50+y],xx,yy;
            if (!value) continue;
            for (xx=0;xx<2;++xx) for (yy=0;yy<2;++yy)
                fb[(top+(int)(y*2+yy))*320+left+(int)(x*2+xx)]=overlay_color(d,value>>4,0);
        }
    }
    return 1;
}

int sim_maps_render(uint8_t *fb,const sim_graphics_pack *pack,const uint8_t *memory,size_t size)
{
    const uint8_t *d;
    const char *title;
    sim_graphics_span colors,font;
    sim_graphics_frame buttons,legend;
    unsigned layer,population,protection,legend_record=5;
    int x,y,index,title_x;
    size_t length;
    if (!fb || !pack || !memory || size<0x100000) return 0;
    d=memory+DG;layer=word(d,0x3268);population=word(d,0x326c);protection=word(d,0x326a);
    if (d[0x2950]!=6 || word(d,0x2b14)!=1 || word(d,0x2b16)!=1 ||
        !(title=sim_maps_label(layer,population,protection)) ||
        sim_graphics_pack_section(pack,SIM_GRAPHICS_MAP_COLORS,&colors)!=SIM_GRAPHICS_OK ||
        sim_graphics_pack_section(pack,SIM_GRAPHICS_FONT_MASKS,&font)!=SIM_GRAPHICS_OK ||
        sim_graphics_pack_frame(pack,4,0,&buttons)!=SIM_GRAPHICS_OK || buttons.width!=56 || buttons.height!=97) return 0;
    /* Exact31490 shell order; original foreground words choose the high-byte
     * MCGA palette index even when their low-byte attributes differ. */
    if (!fill_rect(fb,d,0x2a90,31) || !frame_rect(fb,d,0x2a80,4,12) ||
        !frame_rect(fb,d,0x2a80,1,4) || !fill_rect(fb,d,0x2a98,31) ||
        !frame_rect(fb,d,0x2a88,-2,15) || !fill_rect(fb,d,0x2a70,16) ||
        !fill_rect(fb,d,0x2a78,0)) return 0;
    {
        char smiley[2]={15,0};
        if (!sim_ui_text(fb,font.data,signed_word(d,0x2a7a),signed_word(d,0x2a78)+1,smiley,0,15)) return 0;
    }
    length=strlen(title);
    title_x=(int)((unsigned)(signed_word(d,0x2a76)-signed_word(d,0x2a72)-(int)length*8)&65535)/2+signed_word(d,0x2a72);
    if (!sim_ui_text(fb,font.data,title_x,signed_word(d,0x2a70)+1,title,0,15)) return 0;
    x=signed_word(d,0x2a92)+1;y=signed_word(d,0x2a90);
    if (!sim_mcga_blit(fb,x,y,buttons.pixels.data,buttons.pixels.size,buttons.width,buttons.height,0,0,SIM_MCGA_COPY) ||
        !overview(fb,d,colors.data,font.data)) return 0;
    /* Original repeats the button image after painting the overview. */
    if (!sim_mcga_blit(fb,x,y,buttons.pixels.data,buttons.pixels.size,buttons.width,buttons.height,0,0,SIM_MCGA_COPY)) return 0;
    index=(int)layer-1;x=signed_word(d,0x2a92)+(index&1)*27;y=signed_word(d,0x2a90)+(index/2)*19;
    if (!sim_ui_frame(fb,x+3,y+3,x+27,y+19,2,14) || !overlay(fb,d,memory,size,layer,population,protection)) return 0;
    if ((layer>=4 && layer<=8) || layer==9) {
        if (layer==4 && population==2) legend_record=6;
        if (sim_graphics_pack_frame(pack,legend_record,0,&legend)!=SIM_GRAPHICS_OK) return 0;
        if (!sim_mcga_blit(fb,signed_word(d,0x2a9a)+4,signed_word(d,0x2a98)+2,
            legend.pixels.data,legend.pixels.size,legend.width,legend.height,0,0,SIM_MCGA_COPY)) return 0;
    }
    return 1;
}
