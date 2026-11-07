#include "query_ui.h"
#include "ui_draw.h"
#include "mcga_draw.h"
#include <stdio.h>
#include <string.h>
#define DG 0x4d2c0u
static unsigned u16(const uint8_t*p){return p[0]|((unsigned)p[1]<<8);}
static unsigned field(const uint8_t*m,unsigned alias,unsigned offset){return 16u*u16(m+DG+alias)+offset;}
static const char *ptr(const uint8_t*m,unsigned o){unsigned a=u16(m+DG+o)+16u*u16(m+DG+o+2),n=0;if(!a)return "";while(a+n<1048576u&&n<255u){if(!m[a+n])return (const char*)m+a;++n;}return "";}
int sim_query_ui_panel_row(int old,int x,int y){int cx=x/8,cy=y/8;if(old!=3&&old!=15)old=3;if(cx-1<=22&&cx+1>=1&&cy<=old+8&&cy+1>=old)return old==3?15:3;return old;}
int sim_query_ui_read(const uint8_t*m,size_t size,int x,int y,SimQueryInfo*q){unsigned i,v,cell,a;int growth;if(!m||size<1048576u||!q||x<0||x>=120||y<0||y>=100)return 0;q->x=x;q->y=y;q->tile=u16(m+DG+0x6b78+2u*(unsigned)(x*100+y))&1023u;for(i=1;i<37&&u16(m+DG+0x1f1c+i*2)<=q->tile;++i){}if(i>=37)return 0;if(i==9&&(q->tile&15)==15)i=2;q->category=i;cell=(unsigned)((x&~1)*25+y/2);
 a=field(m,0x49b0,0x654a+cell);q->attribute[0]=m[a]>>6;
 a=field(m,0x49b2,0x9b46+cell);v=m[a];q->attribute[1]=v<50?4:v<150?5:v<250?6:7;
 a=field(m,0x49b4,0x75d0+cell);q->attribute[2]=(m[a]>>6)+8;
 a=field(m,0x49b6,0x6018+cell);v=m[a];q->attribute[3]=v>0&&v<64?13:(v>>6)+12;
 a=field(m,0x49b8,0x99c0+(unsigned)((x/8)*26+((y&~4)/4)));growth=(int16_t)u16(m+a);q->attribute[4]=growth<0?16:growth==0?17:growth<=100?18:19;return 1;}
int sim_query_ui_render(uint8_t*fb,const uint8_t*font,const uint8_t*m,size_t size,const SimQueryInfo*q,int row){int good,i,pad;const char*name;char title[96],value[128];size_t length,total;if(!fb||!font||!m||size<1048576u||!q||q->category>36||(row!=3&&row!=15))return 0;good=sim_mcga_fill(fb,8,row*8,175,(row+8)*8-1,11,SIM_MCGA_COPY)&&sim_ui_frame(fb,8,row*8,175,(row+8)*8-1,2,15);name=ptr(m,0xd322+q->category*4);length=strlen(name);if(length>60u)return 0;pad=19-(int)length;pad=pad>0?pad/2:0;memset(title,32,(size_t)pad);memcpy(title+pad,name,length);total=(size_t)pad+length;while(total<19u)title[total++]=32;title[total]=0;good=good&&sim_ui_text(fb,font,16,(row+1)*8,title,9,11);for(i=0;i<5;++i){if(q->attribute[i]>19)return 0;good=good&&sim_ui_text(fb,font,16,(row+i+2)*8,ptr(m,0xd2bc+(unsigned)i*4),9,11);snprintf(value,sizeof(value),"%-8s",ptr(m,0x1fe2+q->attribute[i]*4));good=good&&sim_ui_text(fb,font,96,(row+i+2)*8,value,9,11);}return good;}

