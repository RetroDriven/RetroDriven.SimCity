#include "message_popup.h"
#include "ui_draw.h"
#include "mcga_draw.h"
#include <string.h>
#define DG 0x4d2c0u
static unsigned u16(const uint8_t*p){return p[0]|((unsigned)p[1]<<8);}
static int dimensions(const char*s,int*width,int*lines){int n=0,max=0,count=0;unsigned total=0;if(!s)return 0;while(*s){if(++total>8192)return 0;if(*s=='\n'){if(n>max)max=n;n=0;++count;}else{if(++n>=80)return 0;}++s;if(!*s&&n){if(n>max)max=n;++count;}}*width=max>38?38:max;*lines=count;return 1;}
int sim_message_popup_layout(const uint8_t*m,size_t size,const char*s,int timed,SimMessagePopupLayout*l){int width,lines,rows,left,top,cx;if(!m||size<1048576u||!l||!dimensions(s,&width,&lines))return 0;rows=lines+4+(m[DG+0x2955]!=255?1:0);left=(40-(width+2))/2;top=(25-rows)/2;l->x0=left*8;l->y0=top*8;l->x1=(left+width+2)*8-1;l->y1=(top+rows)*8-1;cx=(l->x0+(l->x1-l->x0-64)/2)/8;l->button_x0=cx*8;l->button_y0=(top+rows-2)*8;l->button_x1=l->button_x0+63;l->button_y1=l->button_y0+7;l->timeout_ticks=timed?540:0;return l->y0>=0&&l->y1<200;}
int sim_message_popup_key(unsigned k){return k==10||k==13||k=='c'||k=='C';}
int sim_message_popup_render(uint8_t*fb,const uint8_t*font,const uint8_t*m,size_t size,const char*s,int timed,const uint8_t*mask,size_t n){SimMessagePopupLayout l;int good,row=0,y,fg=timed?9:4,bg=timed?11:12;char line[80];unsigned k;const char*p;if(!fb||!font||!sim_message_popup_layout(m,size,s,timed,&l))return 0;if(m[DG+0x2955]!=255&&(!mask||n<4||u16(mask)*u16(mask+2)>n-4))return 0;good=sim_mcga_fill(fb,l.x0,l.y0,l.x1,l.y1,15,SIM_MCGA_COPY)&&sim_ui_frame(fb,l.x0,l.y0,l.x1,l.y1,4,(uint8_t)fg);p=s;y=l.y0;while(*p){k=0;while(*p&&*p!='\n')line[k++]=*p++;line[k]=0;if(*p=='\n')++p;y+=8;if((int8_t)m[DG+0x2955]==row){unsigned w=u16(mask),h=u16(mask+2);good=good&&sim_mcga_mask(fb,(320-(int)w)/2,y,mask+4,n-4,w,h,0,0,4,15,SIM_MCGA_COPY);y+=(int)h-8;}else{int x=l.x0+(l.x1-l.x0-(int)k*8)/2;good=good&&sim_ui_text(fb,font,x,y,line,4,15);}++row;}
 return good&&sim_ui_button(fb,font,(uint16_t)(l.button_x0/8),(uint16_t)(l.button_y0/8),"Continue",(uint8_t)fg,(uint8_t)bg);
}

