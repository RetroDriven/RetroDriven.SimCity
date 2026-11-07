#include "city_tools.h"

static const uint16_t road_shapes[16]={66,67,66,68,67,67,69,73,66,71,66,72,70,75,74,76};
static const uint16_t rail_shapes[16]={226,227,226,228,227,227,229,233,226,231,226,232,230,235,234,236};
static const uint16_t wire_shapes[16]={210,211,210,212,211,211,213,217,210,215,210,216,214,219,218,220};

int sim_city_tool_rectangle(unsigned tool,int xyxy[4])
{
    unsigned index;
    if (!xyxy) return 0;
    xyxy[0]=xyxy[1]=xyxy[2]=xyxy[3]=0;
    if (tool==SIM_CITY_COAL) tool=SIM_CITY_NUCLEAR;
    if (tool<1 || tool>14) return 0;
    index=tool-1;
    xyxy[0]=10+26*(int)(index&1);
    xyxy[1]=35+17*(int)(index/2);
    xyxy[2]=xyxy[0]+23;
    xyxy[3]=xyxy[1]+14;
    return 1;
}

static unsigned normalized(unsigned value)
{
    value&=1023;
    if (value>=64 && value<=207) value=(value&15)+64;
    return value;
}

/* Original bodies read linear neighbors before some bounds checks, including
 * a y=0 road-connectivity quirk. Keep in-array linear wrap; outside the caller's
 * map use zero instead of exposing unrelated DOS DGROUP memory. */
static unsigned at(const uint16_t *map,int index)
{
    return index>=0 && index<12000?map[index]:0;
}

static int road_vertical(unsigned n)
{
    return (n==237 || (n>=64 && n<=78)) && n!=77 && n!=238 && n!=64;
}

static int road_horizontal(unsigned n)
{
    return (n==238 || (n>=64 && n<=78)) && n!=78 && n!=237 && n!=65;
}

static int rail_vertical(unsigned n)
{
    return n>=221 && n<=238 && n!=221 && n!=237 && n!=224;
}

static int rail_horizontal(unsigned n)
{
    return n>=221 && n<=238 && n!=222 && n!=238 && n!=225;
}

static int wire_vertical(unsigned raw)
{
    unsigned n=normalized(raw);
    return (raw&0x4000) && n!=209 && n!=78 && n!=222;
}

static int wire_horizontal(unsigned raw)
{
    unsigned n=normalized(raw);
    return (raw&0x4000) && n!=208 && n!=77 && n!=221;
}

static void connect_one(uint16_t *map,int x,int y)
{
    int index=x*100+y;
    unsigned n=normalized(map[index]),mask=0;
    if (n>=66 && n<=76) {
        /* Assembly3381c compares neighborBX, not coordinateDX. */
        if (road_vertical(normalized(at(map,index-1)))) mask|=1;
        if (x<119 && road_horizontal(normalized(at(map,index+100)))) mask|=2;
        if (y<99 && road_vertical(normalized(at(map,index+1)))) mask|=4;
        if (x>0 && road_horizontal(normalized(at(map,index-100)))) mask|=8;
        map[index]=(uint16_t)(road_shapes[mask]|0x3000);
    } else if (n>=226 && n<=236) {
        if (y>0 && rail_vertical(normalized(at(map,index-1)))) mask|=1;
        if (x<119 && rail_horizontal(normalized(at(map,index+100)))) mask|=2;
        if (y<99 && rail_vertical(normalized(at(map,index+1)))) mask|=4;
        if (x>0 && rail_horizontal(normalized(at(map,index-100)))) mask|=8;
        map[index]=(uint16_t)(rail_shapes[mask]|0x3000);
    } else if (n>=210 && n<=220) {
        if (y>0 && wire_vertical(at(map,index-1))) mask|=1;
        if (x<119 && wire_horizontal(at(map,index+100))) mask|=2;
        if (y<99 && wire_vertical(at(map,index+1))) mask|=4;
        if (x>0 && wire_horizontal(at(map,index-100))) mask|=8;
        map[index]=(uint16_t)(wire_shapes[mask]|0x7000);
    }
}

static void connect_neighbors(uint16_t *map,int x,int y)
{
    connect_one(map,x,y);
    if (y>0) connect_one(map,x,y-1);
    if (x<119) connect_one(map,x+1,y);
    if (y<99) connect_one(map,x,y+1);
    if (x>0) connect_one(map,x-1,y);
}

static uint16_t bulldoze(uint16_t *map,uint32_t *funds,int index)
{
    unsigned n;
    if (*funds<1) return 0x21;
    if (!(map[index]&0x1000)) return 0x2e;
    --*funds;
    n=normalized(map[index]);
    map[index]=(uint16_t)((n==64||n==65||n==208||n==209||n==224||n==225)?2:0);
    return 0;
}

static uint16_t road(uint16_t *map,uint32_t *funds,int index)
{
    unsigned n=map[index]&1023, target=0;
    unsigned cost=10;
    if (*funds<cost) return 0x21;
    if (!n) target=0x3042;
    else if (n>=2 && n<=4) {
        unsigned east=normalized(at(map,index+100));
        unsigned west=normalized(at(map,index-100));
        unsigned south=normalized(at(map,index+1));
        unsigned north=normalized(at(map,index-1));
        cost=50;
        if (*funds<cost) return 0x21;
        if (east==238 || (east>=64&&east<=77&&east!=65)) target=0x1040;
        else if (west==238 || (west>=64&&west<=77&&west!=65)) target=0x1040;
        else if (south==237 || (south>=65&&south<=78&&south!=77)) target=0x1041;
        else if (north==237 || (north>=65&&north<=78&&north!=77)) target=0x1041;
        if (!target) return 0x2d;
    } else if (n==210) target=0x704e;
    else if (n==211) target=0x704d;
    else if (n==226) target=0x30ed;
    else if (n==227) target=0x30ee;
    if (target) { map[index]=(uint16_t)target;*funds-=cost; }
    return 0;
}

static uint16_t rail(uint16_t *map,uint32_t *funds,int index)
{
    unsigned n=normalized(map[index]),target=0,cost=20;
    if (*funds<cost) return 0x21;
    if (!n) target=0x30e2;
    else if (n>=2 && n<=4) {
        unsigned east=normalized(at(map,index+100)),west=normalized(at(map,index-100));
        unsigned south=normalized(at(map,index+1)),north=normalized(at(map,index-1));
        cost=100;if (*funds<cost) return 0x21;
        if (east==221 || (east>=224&&east<=237&&east!=225)) target=0x10e0;
        else if (west==221 || (west>=224&&west<=237&&west!=225)) target=0x10e0;
        else if (south==222 || (south>=225&&south<=238&&south!=237)) target=0x10e1;
        else if (north==222 || (north>=225&&north<=238&&north!=237)) target=0x10e1;
        if (!target) return 0x2d;
    } else if (n==210) target=0x70de;
    else if (n==211) target=0x70dd;
    else if (n==66) target=0x30ee;
    else if (n==67) target=0x30ed;
    if (target) { map[index]=(uint16_t)target;*funds-=cost; }
    return 0;
}

static uint16_t wire(uint16_t *map,uint32_t *funds,int index)
{
    unsigned n=normalized(map[index]),target=0,cost=5;
    if (*funds<cost) return 0x21;
    if (!n) target=0x70d2;
    else if (n>=2 && n<=4) {
        cost=25;if (*funds<cost) return 0x21;
        if (wire_horizontal(at(map,index+100))) target=0x50d1;
        else if (wire_horizontal(at(map,index-100))) target=0x50d1;
        else if (wire_vertical(at(map,index-1))) target=0x50d0;
        else if (wire_vertical(at(map,index+1))) target=0x50d0;
        if (!target) return 0x2d;
    } else if (n==66) target=0x704d;
    else if (n==67) {
        /* Original33761 returns directly: this particular crossing is free. */
        map[index]=0x704e;return 0;
    } else if (n==226) target=0x70dd;
    else if (n==227) target=0x70de;
    if (target) { map[index]=(uint16_t)target;*funds-=cost; }
    return 0;
}

uint16_t sim_city_tool_apply_lowlevel(uint16_t *map,uint32_t *funds,int x,int y,
                                     unsigned tool,int auto_bulldoze)
{
    int index;
    uint16_t result;
    if (!map||!funds||x<0||x>=120||y<0||y>=100 ||
        tool>SIM_CITY_RAIL) return SIM_CITY_TOOL_UNSUPPORTED;
    index=x*100+y;
    if (tool==SIM_CITY_CONNECT) { connect_neighbors(map,x,y);return 0; }
    if (tool==SIM_CITY_BULLDOZE) result=bulldoze(map,funds,index);
    else {
        /* Original caller2a7f4 leaves BX=index*2. OriginBX0 suppresses auto
         * clearing in3338f; retain this caller-specific behavior explicitly. */
        if (auto_bulldoze && index && (map[index]&0x1000)) {
            unsigned n=normalized(map[index]);
            if (n<64 || n==867) {
                if (*funds<1) return 0x21;
                --*funds;map[index]=0;
            }
        }
        if (tool==SIM_CITY_ROAD) result=road(map,funds,index);
        else if (tool==SIM_CITY_WIRE) result=wire(map,funds,index);
        else result=rail(map,funds,index);
    }
    /* Original dispatcher always refreshes the5-cellcross after these tools,
     * even for insufficient funds or a refused/no-op placement. */
    connect_neighbors(map,x,y);
    return result;
}

uint16_t sim_city_tool_apply(uint16_t *map,uint32_t *funds,int x,int y,
                            unsigned tool,int auto_bulldoze)
{
    /* Preserve the0.3limited API guard. Complete placement uses the literal
     * low-level entry above only after its outer demolition/footprint logic. */
    if (!map||!funds||x<0||x>=120||y<0||y>=100 || tool>4)
        return SIM_CITY_TOOL_UNSUPPORTED;
    if (tool && (map[x*100+y]&0x400)) return SIM_CITY_TOOL_UNSUPPORTED;
    return sim_city_tool_apply_lowlevel(map,funds,x,y,tool,auto_bulldoze);
}
