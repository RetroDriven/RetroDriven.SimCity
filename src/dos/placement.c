#include "placement.h"
#include <string.h>

/* Original records at file2a970, original segment2777, sixteen bytes each.
 * Tool12 is nuclear; tool15 is coal. Numeric IDs follow the DOS program. */
static const sim_city_tool_info information[16]={
    {0,0,0,0,""},
    {1,1,0,0,"Bulldoze: $1"},
    {1,10,66,0x1000,"Road: $10"},
    {1,5,210,0x5000,"Power lines: $5"},
    {1,20,226,0x1000,"Railroad: $20"},
    {1,10,40,0x1000,"Park: $10"},
    {3,100,240,0x5000,"Residential: $100"},
    {3,100,423,0x4000,"Commercial: $100"},
    {3,100,612,0x4000,"Industrial: $100"},
    {3,500,770,0x4000,"Police station: $500"},
    {3,500,761,0x4000,"Fire station: $500"},
    {4,3000,779,0x4800,"Stadium: $3000"},
    {4,5000,811,0xc800,"Nuclear reactor: $5000"},
    {4,5000,693,0x4800,"Seaport: $5000"},
    {6,10000,709,0x4800,"Airport: $10000"},
    {4,3000,745,0x4800,"Coal power plant: $3000"}
};

void sim_city_placement_init(sim_city_placement_state *state)
{
    static const uint16_t initial[7]={1018,4521,202,65117,3,39848,468};
    if (!state) return;
    memcpy(state->random,initial,sizeof(initial));
    state->special_4fd8=0;
}

int sim_city_tool_info_get(unsigned tool,sim_city_tool_info *info)
{
    if (!info) return 0;
    memset(info,0,sizeof(*info));
    if (tool<1 || tool>15) return 0;
    *info=information[tool];return 1;
}

static unsigned random_value(sim_city_placement_state *state,unsigned maximum)
{
    uint16_t sum=0;
    unsigned i,value;
    for (i=6;i>0;--i) {
        state->random[i]=state->random[i-1];
        sum=(uint16_t)(sum+state->random[i]);
    }
    state->random[0]=sum;
    value=(sum&32767)/(32767/(maximum+1));
    return value>maximum?maximum:value;
}

static int auto_clearable(unsigned tile)
{
    return (tile>=5 && tile<=47) || (tile>=210 && tile<=220) ||
           (tile>=861 && tile<=869);
}

int sim_city_demolition_size(unsigned tile,int *dx,int *dy)
{
    if ((tile>238 && tile<693) || (tile>760 && tile<779)) {
        *dx=*dy=-1;return 3;
    }
    if ((tile>=693&&tile<=708) || (tile>=745&&tile<=760) ||
        (tile>=779&&tile<=826)) { *dx=*dy=-1;return 4; }
    if (tile==698 || tile==750 || tile==784 || tile==816) {
        *dx=*dy=0;return 4;
    }
    if (tile==699 || tile==751 || tile==785 || tile==817 ||
        (tile>=924&&tile<=926)) { *dx=-1;*dy=0;return 4; }
    if (tile==702 || tile==754 || tile==788 || tile==820) {
        *dx=0;*dy=-1;return 4;
    }
    if (tile==703 || tile==755 || tile==789 || tile==821) {
        *dx=*dy=-1;return 4;
    }
    if (tile>708 && tile<745) {
        *dx=(709-(int)tile)%6;*dy=(709-(int)tile)/6;return 6;
    }
    *dx=*dy=0;return 0;
}

static uint16_t demolish(uint16_t *map,uint32_t *funds,
                         sim_city_placement_state *state,int x,int y)
{
    unsigned raw=map[x*100+y],tile=raw&1023;
    if (!(raw&0x400)) {
        if (tile>=2 && tile<=4) {
            uint16_t result=1;
            if (!(*funds&0x80000000u) && *funds>=6) {
                result=sim_city_tool_apply(map,funds,x,y,1,0);
                if (map[x*100+y]!=tile) *funds-=5;
            }
            return result;
        }
        return sim_city_tool_apply(map,funds,x,y,1,0);
    }
    if (!(*funds&0x80000000u) && *funds) {
        int dx,dy,size=sim_city_demolition_size(tile,&dx,&dy),xx,yy;
        /* Added guard for malformed/out-of-map zone-center states. Real
         * placed footprints satisfy these bounds before they can exist. */
        if (size && (x+dx<0 || y+dy<0 || x+dx+size>120 || y+dy+size>100))
            return SIM_CITY_TOOL_UNSUPPORTED;
        --*funds;
        for (xx=x+dx;xx<x+dx+size;++xx) for (yy=y+dy;yy<y+dy+size;++yy) {
            unsigned n=map[xx*100+yy]&1023;
            if (n!=52 && n!=0 && n!=56)
                map[xx*100+yy]=(uint16_t)(0x1b5c+random_value(state,2));
        }
        if (tile==418) state->special_4fd8=16;
    }
    return 1;
}

static void perimeter(uint16_t *map,uint32_t *funds,int x,int y,int size)
{
    int n,cx=x,cy=y;
    for (n=0;n<size;++n) { (void)sim_city_tool_apply(map,funds,cx,cy-1,0,0);++cx; }
    for (n=0;n<size;++n) { (void)sim_city_tool_apply(map,funds,cx,cy,0,0);++cy; }
    for (n=0;n<size;++n) { --cx;(void)sim_city_tool_apply(map,funds,cx,cy,0,0); }
    --cx;
    for (n=0;n<size;++n) { --cy;(void)sim_city_tool_apply(map,funds,cx,cy,0,0); }
}

uint16_t sim_city_place_view(uint16_t *map,uint32_t *funds,
    sim_city_placement_state *state,int x,int y,unsigned tool,int auto_bulldoze,
    int view_x,int view_y,int view_width,int view_height)
{
    sim_city_tool_info info;
    unsigned flags,tile,clear_cost=0,variant;
    int size,half,left,top,xx,yy;
    uint32_t remaining;
    if (!map || !funds || !state || !sim_city_tool_info_get(tool,&info))
        return SIM_CITY_TOOL_UNSUPPORTED;
    size=(int)info.width;half=(size-1)/2;left=x-half;top=y-half;
    if (left<view_x || top<view_y || left+size>view_x+view_width ||
        top+size>view_y+view_height || left<0 || top<0 || left+size>120 || top+size>100)
        return 0;
    if (tool==1) {
        uint16_t result=demolish(map,funds,state,left,top);
        return result==SIM_CITY_TOOL_UNSUPPORTED?result:1;
    }
    if (tool>=2 && tool<=4) {
        uint16_t result=sim_city_tool_apply_lowlevel(map,funds,left,top,tool,auto_bulldoze);
        return result==0?1:(result==1?0x21:result);
    }
    tile=info.base_tile;flags=info.flags&0xf7ff;
    if (tool==5) {
        unsigned current=map[left*100+top]&1023;
        if (current==840 || (current>=40 && current<43)) return 1;
        flags=0;variant=random_value(state,4);
        tile=variant==4?0x3b48:variant+0x3028;
    }
    for (yy=top;yy<top+size;++yy) for (xx=left;xx<left+size;++xx) {
        unsigned current=map[xx*100+yy]&1023;
        if (current>=2 && current<=4) return 0x2c;
        if (current) {
            if (!auto_bulldoze || !auto_clearable(current)) return 0x22;
            ++clear_cost;
        }
    }
    remaining=*funds-clear_cost-info.cost;
    if (remaining&0x80000000u) return 0x21;
    *funds=remaining;
    for (yy=0;yy<size;++yy) for (xx=0;xx<size;++xx) {
        unsigned value=(flags+tile)&65535;
        if (xx==1 && size!=1) {
            if (yy==1) value=((value+0x400)&65535)|0x1000;
            else if (yy==2 && (info.flags&0x800)) value=(value+0x800)&65535;
        }
        map[(left+xx)*100+top+yy]=(uint16_t)value;
        ++tile;
    }
    perimeter(map,funds,left,top,size);
    return 1;
}

uint16_t sim_city_place(uint16_t *map,uint32_t *funds,
    sim_city_placement_state *state,int x,int y,unsigned tool,int auto_bulldoze)
{
    return sim_city_place_view(map,funds,state,x,y,tool,auto_bulldoze,0,0,120,100);
}

static uint16_t word(const uint8_t *bytes,size_t offset)
{
    return (uint16_t)(bytes[offset]|((uint16_t)bytes[offset+1]<<8));
}

static void put_word(uint8_t *bytes,size_t offset,uint16_t value)
{
    bytes[offset]=(uint8_t)value;bytes[offset+1]=(uint8_t)(value>>8);
}

uint16_t sim_city_place_dgroup(uint8_t *dgroup,size_t size,int x,int y,unsigned tool)
{
    uint16_t map[12000],result;
    sim_city_placement_state state;
    uint32_t funds;
    unsigned i;
    if (!dgroup || size<65536 || tool<1 || tool>15) return SIM_CITY_TOOL_UNSUPPORTED;
    for (i=0;i<12000;++i) map[i]=word(dgroup,0x6b78+i*2);
    for (i=0;i<7;++i) state.random[i]=word(dgroup,0x2402+i*2);
    state.special_4fd8=word(dgroup,0x4fd8);
    funds=word(dgroup,0x2960)|((uint32_t)word(dgroup,0x2962)<<16);
    dgroup[0x2b1a]=(uint8_t)tool;
    result=sim_city_place_view(map,&funds,&state,x,y,tool,dgroup[0x295b],
        (int16_t)word(dgroup,0x5f10),(int16_t)word(dgroup,0x5f0e),
        (int16_t)word(dgroup,0x296e),(int16_t)word(dgroup,0x296c));
    for (i=0;i<12000;++i) put_word(dgroup,0x6b78+i*2,map[i]);
    for (i=0;i<7;++i) put_word(dgroup,0x2402+i*2,state.random[i]);
    put_word(dgroup,0x4fd8,state.special_4fd8);
    put_word(dgroup,0x2960,(uint16_t)funds);put_word(dgroup,0x2962,(uint16_t)(funds>>16));
    return result;
}
