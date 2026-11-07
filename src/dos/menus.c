#include "menus.h"
#include "ui_draw.h"
#include <string.h>

static unsigned le16(const uint8_t *p) { return p[0] | ((unsigned)p[1]<<8); }
static int menu_valid(sim_menu_kind menu) { return menu>=0 && menu<=SIM_MENU_SPEED; }

int sim_menus_open(sim_menus *menus,const uint8_t *data,size_t size)
{
    sim_menus result;
    size_t position=0;
    unsigned record;
    if (!menus) return 0;
    memset(menus,0,sizeof(*menus));memset(&result,0,sizeof(result));
    if (!data) return 0;
    for (record=0;record<22;++record) {
        unsigned length,count,index;
        size_t cursor,end;
        const char *items[16]={0};
        if (position>size || size-position<4) return 0;
        length=le16(data+position);count=le16(data+position+2);position+=4;
        if (length>size-position) return 0;
        end=position+length;
        if (record>=16) {
            if (!count || count>16) return 0;
            cursor=position;
            for (index=0;index<count;++index) {
                if (cursor>=end) return 0;
                if (data[cursor]==255) ++cursor;
                else {
                    const uint8_t *nul=memchr(data+cursor,0,end-cursor);
                    if (!nul || (size_t)(nul-data-cursor)>40) return 0;
                    items[index]=(const char *)(data+cursor);cursor=(size_t)(nul-data)+1;
                }
            }
            if (cursor!=end || items[count-1]) return 0;
            if (record==16) {
                if (count!=5) return 0;
                for (index=0;index<4;++index) { if(!items[index]) return 0;result.title[index]=items[index]; }
            } else {
                static const sim_menu_kind kinds[5]={SIM_MENU_SYSTEM,SIM_MENU_OPTIONS,SIM_MENU_SPEED,SIM_MENU_DISASTERS,SIM_MENU_WINDOWS};
                sim_menu_kind kind=kinds[record-17];
                result.count[kind]=count-1;
                for (index=0;index<count-1;++index) { if(!items[index])return 0;result.item[kind][index]=items[index]; }
            }
        }
        position=end;
    }
    if (position!=size || result.count[SIM_MENU_OPTIONS]!=7 || result.count[SIM_MENU_WINDOWS]!=9) return 0;
    /* Original loader359f0 inserts these executable-resident strings after
     * reading the older MESSAGE.PTF arrays, shifting the following pointers. */
    for (record=7;record>4;--record) result.item[SIM_MENU_OPTIONS][record]=result.item[SIM_MENU_OPTIONS][record-1];
    result.item[SIM_MENU_OPTIONS][4]=" Music Off";result.count[SIM_MENU_OPTIONS]=8;
    for (record=9;record>5;--record) result.item[SIM_MENU_WINDOWS][record]=result.item[SIM_MENU_WINDOWS][record-1];
    result.item[SIM_MENU_WINDOWS][5]=" Terrain Edit  Ctrl-T";result.count[SIM_MENU_WINDOWS]=10;
    *menus=result;
    return 1;
}

int sim_menus_bar_x(unsigned menu)
{
    static const int x[4]={8,64,128,208};return menu<4 ? x[menu] : -1;
}

int sim_menus_bar_hit(int x,int y)
{
    static const int lengths[4]={6,7,9,7};unsigned i;
    if (y<0 || y>11) return -1;
    for(i=0;i<4;++i) if(x>=sim_menus_bar_x(i) && x<=sim_menus_bar_x(i)+lengths[i]*8)return (int)i;
    return -1;
}

int sim_menus_layout(const sim_menus *menus,sim_menu_kind menu,sim_menu_layout *layout)
{
    unsigned i,width=0;
    int left,top,right,bottom;
    if (!menus || !layout || !menu_valid(menu) || !menus->count[menu])return 0;
    for (i=0;i<menus->count[menu];++i) {
        size_t length=strlen(menus->item[menu][i]);if(length>width)width=(unsigned)length;
    }
    if(menu==SIM_MENU_SPEED) { ++width;left=80; }
    else {
        int delta=(int)strlen(menus->title[menu])-(int)width;
        left=(delta & ~1)*4+sim_menus_bar_x((unsigned)menu);
        if(left<0)left=0;
    }
    top=13;right=(int)width*8+left+15;
    if(right>319) {left-=right-319;right=319;}
    bottom=(int)menus->count[menu]*8+top+5;
    if(bottom>194) {top-=bottom-194;bottom=194;}
    if(left<0 || top<0 || width>40)return 0;
    *layout=(sim_menu_layout){left,top,right,bottom,left+8,top+3,(int)width,(int)menus->count[menu]};
    return 1;
}

static int selectable(const sim_menus *menus,sim_menu_kind menu,int selected)
{
    return menus && menu_valid(menu) && selected>=0 && (unsigned)selected<menus->count[menu] &&
           menus->item[menu][selected] && menus->item[menu][selected][1]!='-';
}

int sim_menus_next(const sim_menus *menus,sim_menu_kind menu,int selected,int direction)
{
    unsigned tries;
    if(!menus || !menu_valid(menu) || !menus->count[menu] || !direction)return selected;
    for(tries=0;tries<menus->count[menu];++tries) {
        if(direction>0)selected=(selected+1)%(int)menus->count[menu];
        else selected=selected>0?selected-1:(int)menus->count[menu]-1;
        if(selectable(menus,menu,selected))return selected;
    }
    return -1;
}

int sim_menus_hit(const sim_menus *menus,sim_menu_kind menu,int x,int y)
{
    sim_menu_layout layout;int row;
    if(!sim_menus_layout(menus,menu,&layout) || x<layout.text_x ||
       x>=layout.text_x+layout.width_chars*8 || y<layout.text_y ||
       y>=layout.text_y+layout.rows*8)return -1;
    row=(y-layout.text_y)/8;return selectable(menus,menu,row)?row:-1;
}

static void format_row(char *text,int width,const char *source,sim_menu_kind menu,
                       unsigned row,const sim_menu_flags *flags)
{
    size_t length=strlen(source);int i;
    memset(text,' ',(size_t)width);text[width]=0;
    if(source[1]=='-') { for(i=1;i<width-1;++i)text[i]='-';return; }
    if(menu==SIM_MENU_SPEED) {
        if(flags && (int)row==4-flags->speed)text[0]=16;
        if(length>(size_t)width-1)length=(size_t)width-1;
        memcpy(text+1,source,length);return;
    }
    if(length>(size_t)width)length=(size_t)width;
    memcpy(text,source,length);
    if(!flags)return;
    if(menu==SIM_MENU_OPTIONS) {
        int checked=0;
        if(row==0)checked=flags->auto_bulldoze;
        if(row==1)checked=flags->auto_budget;
        if(row==2)checked=flags->auto_goto;
        if(row==6)checked=flags->animate_all;
        if(row==7)checked=flags->frequent_animation;
        if(row==0 || row==1 || row==2 || row==6 || row==7)text[0]=(char)(checked?16:32);
        if(row==3)memcpy(text+7,flags->sound?"On ":"Off",3);
        if(row==4)memcpy(text+7,flags->music?"On ":"Off",3);
    }
    if(menu==SIM_MENU_DISASTERS && row==7)text[0]=(char)(flags->disable_disasters?16:32);
}

int sim_menus_render(uint8_t *fb,const uint8_t *font,const sim_menus *menus,
                     sim_menu_kind menu,int selected,const sim_menu_flags *flags)
{
    sim_menu_layout layout;char text[41];int i,good;
    if(!fb || !font || !sim_menus_layout(menus,menu,&layout))return 0;
    good=sim_ui_frame(fb,layout.text_x,layout.text_y,
        layout.text_x+layout.width_chars*8-1,layout.text_y+layout.rows*8-1,-3,1) &&
         sim_ui_frame(fb,layout.text_x,layout.text_y,
        layout.text_x+layout.width_chars*8-1,layout.text_y+layout.rows*8-1,-1,11);
    for(i=0;i<layout.rows;++i) {
        int highlight=i==selected && selectable(menus,menu,i);
        format_row(text,layout.width_chars,menus->item[menu][i],menu,(unsigned)i,flags);
        good=good && sim_ui_text(fb,font,layout.text_x,layout.text_y+i*8,text,
                                 (uint8_t)(highlight?11:1),(uint8_t)(highlight?1:11));
    }
    if(menu<SIM_MENU_SPEED)good=good && sim_ui_text(fb,font,sim_menus_bar_x((unsigned)menu),2,
                                                   menus->title[menu],11,1);
    return good;
}

int sim_menus_event(sim_menu_kind menu,int selected)
{
    if(!menu_valid(menu) || selected<0 || selected>=16)return -1;
    if(menu==SIM_MENU_SPEED)return selected<5?4-selected:-1;
    return (int)menu*16+selected;
}

int sim_menus_flags_dgroup(sim_menu_flags *flags,const uint8_t *d,size_t size)
{
    if(!flags || !d || size<65536)return 0;
    flags->auto_bulldoze=d[0x295b]!=0;flags->auto_budget=d[0x295c]!=0;
    flags->auto_goto=d[0x295d]!=0;flags->sound=d[0x295e]!=0;
    flags->music=le16(d+0xc9ea)!=0;
    flags->animate_all=d[0x2959]!=0;flags->frequent_animation=d[0x2958]!=0;
    flags->disable_disasters=d[0x3a64]==0;flags->speed=(int16_t)le16(d+0x3a6e);
    return 1;
}
