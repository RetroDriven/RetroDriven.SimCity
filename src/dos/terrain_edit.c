/* Original free terrain editing31cd2..3246c, with portable input boundaries. */
#include "terrain_edit.h"
#include "terrain.h"
#include "mcga_draw.h"
#include "ui_draw.h"
#include <string.h>

#define DG 0x4d2c0u
#define BASELINE 0x38a30u
#define HISTORY (0x43440u+0x5dc0u)
static const uint16_t brushes[4]={0,0x3025,2,4};
static unsigned word(const uint8_t *p,unsigned at)
{ return p[at]|((unsigned)p[at+1]<<8); }
static void put_word(uint8_t *p,unsigned at,unsigned value)
{ p[at]=(uint8_t)value;p[at+1]=(uint8_t)(value>>8); }

const char *sim_terrain_edit_label(unsigned button)
{
    static const char *const labels[7]={"Dirt","Trees","Water","Channel","Fill","Undo","Smooth"};
    return button<7?labels[button]:NULL;
}

typedef struct span { int16_t y,left,right,dy; } span;
static void push_span(span *stack,unsigned *count,int y,int left,int right,int dy)
{
    if (*count<400 && y+dy>=0 && y+dy<100)
        stack[(*count)++]=(span){(int16_t)y,(int16_t)left,(int16_t)right,(int16_t)dy};
}
static int in_range(const uint16_t *map,int x,int y,unsigned low,unsigned high)
{
    unsigned tile=map[x*100+y]&1023;return tile>=low && tile<=high;
}

int sim_terrain_edit_flood(uint16_t *map,int x,int y,uint16_t replacement)
{
    span stack[400];unsigned count=0,low,high,id;int changed=0;
    if (!map || x<0 || x>=120 || y<0 || y>=100) return -1;
    id=map[x*100+y]&1023;
    if (!id) { if (!replacement) return 0;low=high=0; }
    else if (id<21) {
        if (replacement==2 || replacement==4) return 0;
        low=1;high=20;
    } else if (id<=37) {
        if (replacement==0x3025) return 0;
        low=21;high=37;
    } else {
        if (map[x*100+y]!=replacement) changed=1;
        map[x*100+y]=replacement;return changed;
    }
    /* The original only calls this with brushes outside the replaced class.
     * Reject an arbitrary same-class replacement that would not terminate. */
    if ((replacement&1023)>=low && (replacement&1023)<=high) return -1;
    push_span(stack,&count,y,x,x,1);push_span(stack,&count,y+1,x,x,-1);
    while (count) {
        span s=stack[--count];int start,left,right=s.right,dy=s.dy;
        y=s.y+dy;start=s.left;x=start;
        while (x>=0 && in_range(map,x,y,low,high)) { map[x*100+y]=replacement;++changed;--x; }
        if (x>=start) goto skip;
        left=x+1;
        if (left<start) push_span(stack,&count,y,left,start-1,-dy);
        x=start+1;
        do {
            while (x<120 && in_range(map,x,y,low,high)) { map[x*100+y]=replacement;++changed;++x; }
            push_span(stack,&count,y,left,x-1,dy);
            if (x>right+1) push_span(stack,&count,y,right+1,x-1,-dy);
skip:
            ++x;
            while (x<=right && !in_range(map,x,y,low,high)) ++x;
            left=x;
        } while (x<=right);
    }
    return changed;
}

static int smooth(uint8_t *memory,unsigned target)
{
    sim_terrain t;unsigned i;int changed=0;
    memset(&t,0,sizeof(t));
    for (i=0;i<12000;++i) t.tiles[i]=(uint16_t)word(memory,target+i*2);
    for (i=0;i<5;++i) t.random[i]=(uint16_t)word(memory,DG+0x3b48+i*2);
    sim_terrain_smooth(&t);
    for (i=0;i<12000;++i) {
        if (word(memory,target+i*2)!=t.tiles[i]) changed=1;
        put_word(memory,target+i*2,t.tiles[i]);
    }
    for (i=0;i<5;++i) put_word(memory,DG+0x3b48+i*2,t.random[i]);
    return changed;
}

static int flood(uint8_t *memory,unsigned target,int x,int y,unsigned replacement)
{
    uint16_t map[12000];unsigned i;int result;
    /* Original31f0c's non-terrain fallback writes the live DGROUP map even
     * when replay was passed the baseline far pointer. Preserve that quirk. */
    if (target!=DG+0x6b78 && (word(memory,target+(unsigned)(x*100+y)*2)&1023)>37) {
        unsigned at=DG+0x6b78+(unsigned)(x*100+y)*2;
        result=word(memory,at)!=replacement;put_word(memory,at,replacement);return result;
    }
    for (i=0;i<12000;++i) map[i]=(uint16_t)word(memory,target+i*2);
    result=sim_terrain_edit_flood(map,x,y,(uint16_t)replacement);
    if (result>=0) for (i=0;i<12000;++i) put_word(memory,target+i*2,map[i]);
    return result;
}

static void replay(uint8_t *memory,unsigned target,int x,int y,unsigned command)
{
    if (command&0x800) (void)smooth(memory,target);
    else if (command&0x400) (void)flood(memory,target,x,y,command^0x400);
    else put_word(memory,target+(unsigned)(x*100+y)*2,command);
}

static void remember(uint8_t *memory,int x,int y,unsigned command)
{
    uint8_t *d=memory+DG;unsigned count=d[0x32f4];
    if (count==100) {
        replay(memory,BASELINE,(int16_t)word(memory,HISTORY),(int16_t)word(memory,HISTORY+2),word(memory,HISTORY+4));
        memmove(memory+HISTORY,memory+HISTORY+6,99*6);--count;
    }
    put_word(memory,HISTORY+count*6,(unsigned)x);put_word(memory,HISTORY+count*6+2,(unsigned)y);
    put_word(memory,HISTORY+count*6+4,command);d[0x32f4]=(uint8_t)(count+1);
}

static int valid(uint8_t *memory,size_t size)
{ return memory && size>=0x100000 && memory[DG+0x32f4]<=100; }

int sim_terrain_edit_enter(uint8_t *memory,size_t size)
{
    uint8_t *d;
    if (!memory || size<0x100000) return 0;
    d=memory+DG;
    if (!d[0x32f2]) { d[0x4d20]=d[0x3a6e];put_word(d,0x3a6e,0); }
    d[0x32f2]=1;put_word(d,0xca90,0);put_word(d,0xca92,0);d[0x32f4]=0;
    memcpy(memory+BASELINE,d+0x6b78,24000);return SIM_EDIT_ACCEPTED;
}

int sim_terrain_edit_exit(uint8_t *memory,size_t size)
{
    uint8_t *d;
    if (!memory || size<0x100000) return 0;
    d=memory+DG;
    if (d[0x32f2]) { d[0x32f2]=0;put_word(d,0x3a6e,(unsigned)(int)(int8_t)d[0x4d20]); }
    return SIM_EDIT_ACCEPTED;
}

int sim_terrain_edit_select(uint8_t *memory,size_t size,unsigned button)
{
    uint8_t *d;unsigned i,count;int changed=0;
    if (!valid(memory,size) || button>6) return 0;
    d=memory+DG;
    if (button<4) put_word(d,0x32f0,button);
    else if (button==4) d[0x32f3]=(uint8_t)!d[0x32f3];
    else if (button==6) { changed=smooth(memory,DG+0x6b78);remember(memory,0,0,0x800); }
    else if ((count=d[0x32f4])!=0) {
        memcpy(d+0x6b78,memory+BASELINE,24000);d[0x32f4]=(uint8_t)--count;
        for (i=0;i<count;++i) replay(memory,DG+0x6b78,(int16_t)word(memory,HISTORY+i*6),
            (int16_t)word(memory,HISTORY+i*6+2),word(memory,HISTORY+i*6+4));
        changed=1;
    }
    return SIM_EDIT_ACCEPTED|(changed?SIM_EDIT_MAP_CHANGED:0);
}

int sim_terrain_edit_apply(uint8_t *memory,size_t size,int x,int y)
{
    uint8_t *d;unsigned tool,replacement,at;int changed=0,sound=0;
    if (!valid(memory,size) || x<0 || x>=120 || y<0 || y>=100) return 0;
    d=memory+DG;tool=word(d,0x32f0);
    if (tool>3) return 0;
    replacement=brushes[tool];at=0x6b78+(unsigned)(x*100+y)*2;
    if (d[0x32f3]) {
        d[0x32f3]=0;sound=1;changed=flood(memory,DG+0x6b78,x,y,replacement)>0;
        remember(memory,x,y,replacement|0x400);
    } else if (word(d,at)!=replacement) {
        put_word(d,at,replacement);changed=sound=1;remember(memory,x,y,replacement);
    }
    return SIM_EDIT_ACCEPTED|(sound?SIM_EDIT_SOUND_BUILD:0)|(changed?SIM_EDIT_MAP_CHANGED:0);
}

int sim_terrain_edit_hit(const uint8_t *d,size_t size,int x,int y)
{
    unsigned i;int left,top;
    if (!d || size<65536 || d[0x2950]!=6) return 0;
    left=(int)word(d,0x299a)+3;
    for (i=0;i<7;++i) {
        top=(int)word(d,0x2998)+3+(int)i*16;
        if (x>=left && x<=left+49 && y>=top && y<=top+15) return 0x110+(int)i;
    }
    return 0;
}

static int fill(uint8_t *fb,const uint8_t *d,unsigned at,uint8_t color)
{ return sim_mcga_fill(fb,(int)word(d,at+2),(int)word(d,at),(int)word(d,at+6),(int)word(d,at+4),color,SIM_MCGA_COPY); }
static int frame(uint8_t *fb,const uint8_t *d,unsigned at,int thickness,uint8_t color)
{ return sim_ui_frame(fb,(int)word(d,at+2),(int)word(d,at),(int)word(d,at+6),(int)word(d,at+4),thickness,color); }
static int toolbar(uint8_t *fb,const uint8_t *font,const uint8_t *art,const uint8_t *d)
{
    unsigned selected=word(d,0x32f0),i;int left=(int)word(d,0x299a)+3,top=(int)word(d,0x2998)+3;
    if (selected>3) return 0;
    if (!fill(fb,d,0x29a8,17) || !sim_ui_text(fb,font,(int)word(d,0x2992),(int)word(d,0x29a8)+1,
        sim_terrain_edit_label(selected),1,15) ||
        !sim_mcga_blit(fb,(int)word(d,0x299a),(int)word(d,0x2998),art+12,55*118,55,118,0,0,SIM_MCGA_COPY)) return 0;
    if (!d[0x32f4]) {
        int x,y;
        for (y=top+5*16;y<=top+5*16+15;++y) for (x=left;x<=left+49;++x)
            if ((x+y)&1) fb[y*320+x]=255;
    }
    for (i=0;i<7;++i) if (i==selected || (i==4 && d[0x32f3]))
        if (!sim_ui_frame(fb,left,top+(int)i*16,left+49,top+(int)i*16+15,2,14)) return 0;
    return 1;
}

int sim_terrain_edit_shell_render(uint8_t *fb,const sim_graphics_pack *pack,
                                   const uint8_t *art,size_t art_size,
                                   const uint8_t *d,size_t size,const char *name)
{
    sim_graphics_span font;char text[64];size_t length,spaces;int title_chars,title_y;
    if (!fb || !pack || !art || !d || size<65536 || art_size!=6578 || d[0x2950]!=6 ||
        art[2]!=8 || word(art,4)!=6502 || word(art,8)!=55 || word(art,10)!=118 ||
        art[6504]!=8 || word(art,6506)!=76 || word(art,6510)!=8 || word(art,6512)!=8 ||
        sim_graphics_pack_section(pack,SIM_GRAPHICS_FONT_MASKS,&font)!=SIM_GRAPHICS_OK) return 0;
    if (!fill(fb,d,0x2978,7) || !fill(fb,d,0x29b8,8) ||
        !sim_mcga_fill(fb,(int)word(d,0x299a),(int)word(d,0x2998),(int)word(d,0x299e),(int)word(d,0x29a4),16,SIM_MCGA_COPY) ||
        !fill(fb,d,0x29b0,15) ||
        !sim_ui_text(fb,font.data,(int)word(d,0x29b2),(int)word(d,0x29b0)+1,"+",1,15) ||
        !frame(fb,d,0x2970,3,0) || !frame(fb,d,0x2970,1,0) ||
        !frame(fb,d,0x2990,-1,15) || !toolbar(fb,font.data,art,d)) return 0;
    if (name) {
        length=strlen(name);if (length>40) return 0;
        title_chars=((int)word(d,0x297e)-(int)word(d,0x297a))/8-12;
        spaces=title_chars>(int)length?((size_t)title_chars-length)/2:0;
        title_y=((int)word(d,0x297c)-(int)word(d,0x2978)-7)/2+(int)word(d,0x2978);
        memset(text,' ',spaces);memcpy(text+spaces,name,length+1);
        if (!sim_ui_text(fb,font.data,(int)word(d,0x297a)+8,title_y,text,8,7)) return 0;
    }
    return 1;
}
