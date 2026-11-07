#include "city_stats.h"
#include "ui_draw.h"
#include "mcga_draw.h"
#include <stdio.h>
#include <string.h>

static unsigned word(const uint8_t *d,unsigned at)
{ return d[at]|((unsigned)d[at+1]<<8); }
static int s16(unsigned value)
{ value&=65535;return value>=32768?(int)value-65536:(int)value; }
static int32_t longword(const uint8_t *d,unsigned at)
{
    uint32_t bits=word(d,at)|((uint32_t)word(d,at+2)<<16);
    int32_t result;memcpy(&result,&bits,4);return result;
}
static int copy_result(char *output,size_t capacity,const char *value)
{
    size_t length;
    if (!output || !capacity) return 0;
    output[0]=0;length=strlen(value);
    if (length>=capacity) return 0;
    memcpy(output,value,length+1);return 1;
}

int sim_city_stats_money(char *output,size_t capacity,int32_t funds)
{
    char number[16],result[20];
    uint32_t magnitude=funds<0?0u-(uint32_t)funds:(uint32_t)funds;
    size_t n=0,i,digits;
    result[n++]=funds<0?'-':' ';result[n++]='$';
    /* Original negation wraps INT32_MIN, then %ld still formats it negative. */
    if (magnitude==0x80000000u) { result[n++]='-'; }
    (void)snprintf(number,sizeof(number),"%u",(unsigned)magnitude);
    digits=strlen(number);
    for (i=0;i<digits;++i) {
        result[n++]=number[i];
        if (i+1<digits && (digits-i-1)%3==0) result[n++]=',';
    }
    result[n]=0;return copy_result(output,capacity,result);
}

int sim_city_stats_funds_label(char *output,size_t capacity,int32_t funds)
{
    char money[20],result[32];const char *value=money;
    if (!sim_city_stats_money(money,sizeof(money),funds)) return 0;
    if (strlen(money)>10) ++value;
    (void)snprintf(result,sizeof(result),"Funds:%-9s",value);
    return copy_result(output,capacity,result);
}

int sim_city_stats_date(char *output,size_t capacity,int32_t tick,
                         int16_t base,const char *const months[12])
{
    static const char *const english[12]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    char result[48];int year;unsigned month;
    if (output && capacity) output[0]=0;
    if (tick<0) return 0;
    month=(unsigned)(tick%48)/4;year=s16((unsigned)(tick/48)+(unsigned)(int)base);
    if (!months) months=english;
    if (!months[month] || strlen(months[month])>24) return 0;
    (void)snprintf(result,sizeof(result),"%3s %4d",months[month],year);
    return copy_result(output,capacity,result);
}

typedef struct demand_rectangle { int x0,y0,x1,y1;uint8_t color; } demand_rectangle;
int sim_city_stats_demand_render(uint8_t *fb,const uint8_t *d,size_t size)
{
    static const unsigned variables[3]={0xd2ba,0xd256,0xd2d0};
    static const int offsets[3]={8,21,34},denominators[3]={1500,2000,1500};
    static const uint8_t colors[3]={9,10,14};
    demand_rectangle rectangles[12];unsigned count=0,i,j;
    int baseline,upper,width;
    if (!fb || !d || size<65536) return 0;
    if (d[0x32f2]) return 1;
    if (d[0x2950]!=6) return 0;
    baseline=s16(word(d,0x29a0))+20;upper=baseline-10;width=s16(word(d,0x3266));
    if (width<0) return 0;
    for (i=0;i<3;++i) {
        int value=s16(word(d,variables[i]));
        /* Original IMUL16 is followed by CWD, discarding the high product. */
        int scaled=s16((unsigned)(value*9))/denominators[i];
        int x=s16(word(d,0x29a2))+2+offsets[i];
        int lower_start=value<0?baseline-scaled:baseline-1;
        int upper_start=value>0?upper-scaled:upper+1;
        if (value<0) rectangles[count++]=(demand_rectangle){x,lower_start,x+width,baseline,colors[i]};
        rectangles[count++]=(demand_rectangle){x,lower_start+1,x+width,baseline+8,60};
        if (value>0) rectangles[count++]=(demand_rectangle){x,upper_start,x+width,upper,colors[i]};
        rectangles[count++]=(demand_rectangle){x,upper_start-1,x+width,upper-9,60};
    }
    for (j=0;j<count;++j) {
        demand_rectangle *r=rectangles+j;
        if (r->x0<0 || r->x1>=320 || r->y0<0 || r->y0>=200 || r->y1<0 || r->y1>=200) return 0;
    }
    for (j=0;j<count;++j) {
        const demand_rectangle *r=rectangles+j;
        (void)sim_mcga_fill(fb,r->x0,r->y0,r->x1,r->y1,r->color,SIM_MCGA_COPY);
    }
    return 1;
}

int sim_city_stats_funds_render(uint8_t *fb,const uint8_t *font,const uint8_t *d,size_t size)
{
    char label[32];
    if (!fb || !font || !d || size<65536) return 0;
    if (d[0x32f2] || !d[0x2969] || (!d[0x2959] && d[0x5326]!=1)) return 1;
    if (!sim_city_stats_funds_label(label,sizeof(label),longword(d,0x2960))) return 0;
    return sim_ui_text(fb,font,s16(word(d,0x29ba)),s16(word(d,0x29b8))+1,label,7,8);
}

int sim_city_stats_date_render(uint8_t *fb,const uint8_t *font,const uint8_t *d,size_t size)
{
    char label[48];
    if (!fb || !font || !d || size<65536) return 0;
    if (!sim_city_stats_date(label,sizeof(label),longword(d,0x5f70),(int16_t)s16(word(d,0x294c)),NULL)) return 0;
    return sim_ui_text(fb,font,s16(word(d,0x297e))-96,s16(word(d,0x2978)),label,8,7);
}
