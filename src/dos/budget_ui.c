#include "budget_ui.h"
#include "ui_draw.h"
#include "mcga_draw.h"
#include <stdio.h>
#include <string.h>

#define DG 0x4d2c0u
static const unsigned rows[4]={0x36b6,0x36c0,0x36ac,0x36ca};
static const unsigned allocated[3]={0x532a,0x52be,0xd168};
static uint16_t r16(const uint8_t *p){return p[0]|((uint16_t)p[1]<<8);}
static void w16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static uint32_t r32(const uint8_t *p){return r16(p)|((uint32_t)r16(p+2)<<16);}
static void w32(uint8_t *p,uint32_t v){w16(p,(uint16_t)v);w16(p+2,(uint16_t)(v>>16));}
static int32_t signed32(uint32_t bits){int32_t value;memcpy(&value,&bits,4);return value;}
static int32_t proportion(int32_t a,int32_t b,int32_t divisor)
{
    uint32_t bits=(uint32_t)((uint64_t)(uint32_t)a*(uint32_t)b);
    return signed32(bits)/divisor;
}

int sim_budget_ui_adjust(uint8_t *memory,size_t size,unsigned event,unsigned step)
{
    uint8_t *d,*row;
    unsigned index;
    int value;
    if(!memory || size<1048576 || event<0x800 || event>0x807 || (step!=1 && step!=10))return 0;
    d=memory+DG;index=(event-0x800)/2;row=d+rows[index];
    value=(int16_t)r16(row+8)+(event&1?-(int)step:(int)step);
    if(value<0)value=0;
    if(value>(index==3?20:100))value=index==3?20:100;
    w16(row+8,(uint16_t)value);
    if(index!=3){
        int32_t request=signed32(r32(row+4));
        uint32_t total,available;
        w32(row,(uint32_t)proportion(value,request,100));
        w32(d+allocated[index],r32(row));
        total=r32(d+0xd168)+r32(d+0x52be)+r32(d+0x532a);
        available=r32(d+0x5f7a)+r32(d+0x2960);
        if(signed32(available)<signed32(total)){
            uint32_t amount=r32(row)+available-total;
            if(!request)return 0; /* Original would divide by zero on invalid state. */
            w32(row,amount);w32(d+allocated[index],amount);
            w16(row+8,(uint16_t)proportion(signed32(amount),100,request));
            total=r32(d+0xd168)+r32(d+0x52be)+r32(d+0x532a);
        }
        w32(d+0x4dcc,total);
    }
    w16(d+0xd192,r16(d+0x36d2));
    return 1;
}

int sim_budget_ui_hit(int x,int y)
{
    int row;
    for(row=0;row<3;++row)if(y>=96+row*16 && y<=103+row*16){
        if(x>=208 && x<=215)return 0x800+row*2;
        if(x>=248 && x<=255)return 0x801+row*2;
    }
    if(y>=40 && y<=47){if(x>=176 && x<=183)return 0x806;if(x>=216 && x<=223)return 0x807;}
    if(x>=72 && x<=231 && y>=176 && y<=183)return 0x808;
    return 0;
}

int sim_budget_ui_cursor(unsigned focus,int *x,int *y)
{
    if(!x || !y || focus>8)return 0;
    if(focus<6){*x=focus&1?251:211;*y=99+(int)(focus/2)*16;}
    else if(focus<8){*x=focus&1?219:179;*y=43;}
    else {*x=151;*y=179;}
    return 1;
}

int sim_budget_ui_step(uint8_t *memory,size_t size,int direction)
{
    uint8_t *d;int focus;
    if(!memory || size<1048576)return -1;
    d=memory+DG;focus=(int16_t)r16(d+0x36f0);
    if(direction>0)focus=(focus+1)%9;
    else if(direction<0)focus=focus==0?8:focus-1;
    w16(d+0x36f0,(uint16_t)focus);return focus;
}

static void money(char *out,size_t size,int32_t value)
{
    char digits[16],commas[24];unsigned n=0,i,length;
    uint32_t magnitude=value<0?0u-(uint32_t)value:(uint32_t)value;
    snprintf(digits,sizeof(digits),"%u",(unsigned)magnitude);length=(unsigned)strlen(digits);
    for(i=0;i<length;++i){commas[n++]=digits[i];if(i+1<length && (length-i-1)%3==0)commas[n++]=',';}
    commas[n]=0;snprintf(out,size,"%s%s",value<0?"-$":" $",commas);
}

int sim_budget_ui_render(uint8_t *fb,const uint8_t *font,const uint8_t *memory,size_t size)
{
    const uint8_t *d;char text[64],formatted[32];int good,i;
    static const char *labels[3]={" Trans","Police","  Fire"};
    if(!fb || !font || !memory || size<1048576)return 0;
    d=memory+DG;
    good=sim_mcga_fill(fb,8,16,295,191,15,SIM_MCGA_COPY) &&
         sim_mcga_fill(fb,16,64,287,143,9,SIM_MCGA_COPY);
    for(i=4;i>=2;--i){
        uint8_t color=(uint8_t)(i==3?9:11);
        good=good && sim_ui_frame(fb,16,64,287,143,i,color) &&
                     sim_ui_frame(fb,8,16,295,191,i,color);
    }
    snprintf(text,sizeof(text),"%d Fiscal Budget",(int16_t)(signed32(r32(d+0x5f70))/48+(int16_t)r16(d+0x294c)));
    good=good && sim_ui_text(fb,font,88,24,text,9,15);
    for(i=0;i<3;++i)good=good && sim_mcga_fill(fb,24+i*2,32+i*2,280-i*2,32+i*2,9,SIM_MCGA_COPY);
    good=good && sim_ui_text(fb,font,64,69," Amount    Amount   Funding",15,9) &&
                 sim_ui_text(fb,font,64,77,"Requested Allocated  Level ",15,9) &&
                 sim_mcga_fill(fb,64,88,256,89,15,SIM_MCGA_COPY);
    for(i=0;i<3;++i){
        const uint8_t *row=d+rows[i];int y=96+i*16;
        good=good && sim_ui_text(fb,font,24,y,labels[i],15,9);
        snprintf(text,sizeof(text),"$%-6d",(int16_t)r16(row+4));
        good=good && sim_ui_text(fb,font,80,y,text,15,9);
        snprintf(text,sizeof(text),"$%-6d",(int16_t)r16(row));
        good=good && sim_ui_text(fb,font,144,y,text,15,9) &&
                     sim_ui_text(fb,font,208,y,"\x1e",15,11) &&
                     sim_ui_text(fb,font,248,y,"\x1f",15,11);
        snprintf(text,sizeof(text),"%3d%%",(int16_t)r16(row+8));
        good=good && sim_ui_text(fb,font,216,y,text,15,9);
    }
    good=good && sim_ui_text(fb,font,96,40,"Tax Rate",9,15);
    snprintf(text,sizeof(text)," Taxes collected   $%-6d",(int16_t)r16(d+0x5f7a));
    good=good && sim_ui_text(fb,font,48,sim_ui_y(0xfd07),text,9,15) &&
                 sim_ui_text(fb,font,176,40,"\x1e",15,11) &&
                 sim_ui_text(fb,font,216,40,"\x1f",15,11);
    snprintf(text,sizeof(text),"%3d%%",(int16_t)r16(d+0x36d2));
    good=good && sim_ui_text(fb,font,184,40,text,15,9);
    snprintf(text,sizeof(text),"%-14s"," Cash Flow");
    good=good && sim_ui_text(fb,font,64,sim_ui_y(0x412),text,9,15);
    snprintf(text,sizeof(text),"%-14s"," Previous Funds");
    good=good && sim_ui_text(fb,font,64,sim_ui_y(0x413),text,9,15);
    money(formatted,sizeof(formatted),signed32(r32(d+0x2960)));snprintf(text,sizeof(text),"%-10s",formatted);
    good=good && sim_ui_text(fb,font,176,sim_ui_y(0x413),text,9,15) &&
                 sim_mcga_fill(fb,64,166,232,166,9,SIM_MCGA_COPY);
    snprintf(text,sizeof(text),"%-14s"," Current Funds");
    good=good && sim_ui_text(fb,font,64,sim_ui_y(0x814),text,9,15);
    money(formatted,sizeof(formatted),signed32(r32(d+0x5f7a)-r32(d+0x4dcc)));snprintf(text,sizeof(text),"%-10s",formatted);
    good=good && sim_ui_text(fb,font,176,sim_ui_y(0x412),text,9,15);
    money(formatted,sizeof(formatted),signed32(r32(d+0x5f7a)-r32(d+0x4dcc)+r32(d+0x2960)));snprintf(text,sizeof(text),"%-10s",formatted);
    return good && sim_ui_text(fb,font,176,sim_ui_y(0x814),text,9,15) &&
                   sim_ui_button(fb,font,9,22,"Go with these figures",9,11);
}
