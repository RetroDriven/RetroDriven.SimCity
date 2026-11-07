/* Reconstructed DOS2.00 terrain routines 0x12824..0x12f45.
 * Original tables, instruction evidence and finite comparisons: terrain.md. */
#include "terrain.h"
#include <string.h>

static const int8_t adjacent_x[4] = {-1,0,1,0};
static const int8_t adjacent_y[4] = {0,1,0,-1};
static const uint16_t shore[16] = {
    4109,4109,4113,4111,4101,2,4115,4113,
    4105,4107,2,4109,4103,4105,4101,2
};
static const uint16_t woods[16] = {0,0,0,34,0,0,36,35,0,32,0,33,30,31,29,37};
static const int8_t move_x[8] = {0,1,1,1,0,-1,-1,-1};
static const int8_t move_y[8] = {-1,-1,0,1,1,1,0,-1};
static const uint8_t big_river[81] = {
    0,0,0,3,3,3,0,0,0,
    0,0,3,2,2,2,3,0,0,
    0,3,2,2,2,2,2,3,0,
    3,2,2,2,2,2,2,2,3,
    3,2,2,2,4,2,2,2,3,
    3,2,2,2,2,2,2,2,3,
    0,3,2,2,2,2,2,3,0,
    0,0,3,2,2,2,3,0,0,
    0,0,0,3,3,3,0,0,0
};
static const uint8_t small_river[36] = {
    0,0,3,3,0,0, 0,3,2,2,3,0, 3,2,2,2,2,3,
    3,2,2,2,2,3, 0,3,2,2,3,0, 0,0,3,3,0,0
};

void sim_terrain_init(sim_terrain *t)
{
    static const uint16_t initial[5] = {1018,4521,202,419,3};
    if (!t) return;
    memset(t,0,sizeof(*t));
    memcpy(t->random,initial,sizeof(initial));
}

uint16_t sim_terrain_random(sim_terrain *t, unsigned maximum)
{
    uint16_t sum=0;
    unsigned i, value, divisor;
    /* Reject bounds whose original16-bit divisor would be zero/invalid. */
    if (!t || maximum>32766) return 0;
    divisor=32767/(maximum+1);
    for (i=4;i>0;--i) {
        t->random[i]=t->random[i-1];
        sum=(uint16_t)(sum+t->random[i]);
    }
    t->random[0]=sum;
    value=(sum&32767)/divisor;
    return (uint16_t)(value>maximum?maximum:value);
}

int sim_terrain_inside(int x, int y)
{
    return x>=0 && x<120 && y>=0 && y<100;
}

static unsigned low_random(sim_terrain *t, unsigned maximum)
{
    unsigned a=sim_terrain_random(t,maximum);
    unsigned b=sim_terrain_random(t,maximum);
    return a<b?a:b;
}

static void move(sim_terrain *t, unsigned direction)
{
    t->x=(int16_t)(t->x+move_x[direction&7]);
    t->y=(int16_t)(t->y+move_y[direction&7]);
}

static void stamp(sim_terrain *t, unsigned size, const uint8_t *pattern)
{
    unsigned dx,dy;
    for (dx=0;dx<size;++dx) for (dy=0;dy<size;++dy) {
        unsigned value=pattern[dy*size+dx];
        int x=t->x+(int)dx, y=t->y+(int)dy;
        uint16_t current;
        if (!value || !sim_terrain_inside(x,y)) continue;
        current=t->tiles[x*100+y];
        if (current) {
            current&=1023;
            if ((current==2 && value!=4) || current==4) continue;
        }
        t->tiles[x*100+y]=(uint16_t)value;
    }
}

static void smooth_shore(sim_terrain *t)
{
    int x,y,i;
    for (x=0;x<120;++x) for (y=0;y<100;++y) {
        uint16_t tile=t->tiles[x*100+y]&1023;
        unsigned mask=0, value;
        if (!tile || tile>=4) continue;
        for (i=0;i<4;++i) {
            int nx=x+adjacent_x[i],ny=y+adjacent_y[i];
            mask*=2;
            if (sim_terrain_inside(nx,ny)) {
                tile=t->tiles[nx*100+ny]&1023;
                if (tile && tile<21) ++mask;
            }
        }
        value=shore[mask&15];
        if (value!=2 && sim_terrain_random(t,1)) ++value;
        t->tiles[x*100+y]=(uint16_t)value;
    }
}

static void smooth_woods(sim_terrain *t)
{
    int x,y,i;
    for (x=0;x<120;++x) for (y=0;y<100;++y) {
        uint16_t tile=t->tiles[x*100+y]&1023;
        unsigned mask=0,value;
        if (tile<=20 || tile>=38) continue;
        for (i=0;i<4;++i) {
            int nx=x+adjacent_x[i],ny=y+adjacent_y[i];
            mask*=2;
            if (sim_terrain_inside(nx,ny)) {
                tile=t->tiles[nx*100+ny]&1023;
                if (tile>20 && tile<38) ++mask;
            }
        }
        value=woods[mask&15];
        if (!value) t->tiles[x*100+y]=0;
        else {
            if (value!=37 && ((x+y)&1)) value-=8;
            t->tiles[x*100+y]=(uint16_t)(value+0x3000);
        }
    }
}

static void add_woods(sim_terrain *t)
{
    unsigned count=sim_terrain_random(t,100)+50, n;
    for (n=0;n<count;++n) {
        unsigned steps,i;
        int x=sim_terrain_random(t,119), y=sim_terrain_random(t,99);
        steps=sim_terrain_random(t,150)+50;
        t->x=(int16_t)x;t->y=(int16_t)y;
        for (i=0;i<steps;++i) {
            move(t,sim_terrain_random(t,7));
            if (!sim_terrain_inside(t->x,t->y)) break;
            if (!t->tiles[t->x*100+t->y]) t->tiles[t->x*100+t->y]=0x3025;
        }
    }
    smooth_woods(t);smooth_woods(t);
}

static void river_walk(sim_terrain *t, int small)
{
    int inset=small?3:4;
    while (sim_terrain_inside(t->x+inset,t->y+inset)) {
        stamp(t,small?6:9,small?small_river:big_river);
        if (sim_terrain_random(t,10)>(small?5:4)) ++t->direction;
        if (sim_terrain_random(t,10)>(small?5:4)) --t->direction;
        if (!sim_terrain_random(t,small?12:10)) t->direction=t->initial_direction;
        move(t,(unsigned)t->direction);
    }
}

static void rivers(sim_terrain *t)
{
    t->initial_direction=(int16_t)sim_terrain_random(t,3);
    t->direction=t->initial_direction;
    river_walk(t,0);
    t->x=t->start_x;t->y=t->start_y;
    t->initial_direction^=4;
    t->direction=t->initial_direction;
    river_walk(t,0);
    t->x=t->start_x;t->y=t->start_y;
    t->initial_direction=(int16_t)sim_terrain_random(t,3);
    river_walk(t,1);
}

static void lakes(sim_terrain *t)
{
    unsigned count=sim_terrain_random(t,10),n;
    for (n=0;n<count;++n) {
        int x=sim_terrain_random(t,99),y=sim_terrain_random(t,80);
        unsigned stamps=sim_terrain_random(t,12)+2,i;
        for (i=0;i<stamps;++i) {
            t->x=(int16_t)(sim_terrain_random(t,12)+x+4);
            t->y=(int16_t)(sim_terrain_random(t,12)+y+4);
            if (!sim_terrain_random(t,4)) stamp(t,9,big_river);
            else stamp(t,6,small_river);
        }
    }
}

static void island(sim_terrain *t)
{
    int x,y;
    for (x=0;x<120;++x) for (y=0;y<100;++y) t->tiles[x*100+y]=2;
    for (x=5;x<115;++x) for (y=5;y<95;++y) t->tiles[x*100+y]=0;
    for (x=0;x<115;x+=2) {
        t->x=(int16_t)x;t->y=(int16_t)low_random(t,18);stamp(t,9,big_river);
        t->y=(int16_t)(90-low_random(t,18));stamp(t,9,big_river);
        t->y=0;stamp(t,6,small_river);
        t->y=94;stamp(t,6,small_river);
    }
    for (y=0;y<95;y+=2) {
        t->y=(int16_t)y;t->x=(int16_t)low_random(t,18);stamp(t,9,big_river);
        t->x=(int16_t)(110-low_random(t,18));stamp(t,9,big_river);
        t->x=0;stamp(t,6,small_river);
        t->x=114;stamp(t,6,small_river);
    }
    smooth_shore(t);add_woods(t);
}

void sim_terrain_generate(sim_terrain *t, uint16_t timer_low)
{
    if (!t) return;
    t->random[0]=timer_low;
    t->island=!sim_terrain_random(t,10);
    if (t->island) { island(t);return; }
    memset(t->tiles,0,sizeof(t->tiles));
    t->start_x=(int16_t)(sim_terrain_random(t,40)+40);
    t->start_y=(int16_t)(sim_terrain_random(t,33)+33);
    t->x=t->start_x;t->y=t->start_y;
    rivers(t);lakes(t);smooth_shore(t);add_woods(t);
}

void sim_terrain_smooth(sim_terrain *t)
{
    if (!t) return;
    smooth_shore(t);smooth_shore(t);smooth_woods(t);smooth_woods(t);
}
