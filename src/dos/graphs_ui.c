/* Original2dd30/2deb4/2dfce/2e444 MCGA graph presentation. */
#include "graphs_ui.h"
#include "ui_draw.h"
#include "mcga_draw.h"
#include <stdio.h>
#include <string.h>
#define DG 0x4d2c0u
static unsigned u16(const uint8_t*p){return p[0]|((unsigned)p[1]<<8);}
static int r(const uint8_t*m,unsigned o){return (int16_t)u16(m+DG+o);}
static int32_t signed32(uint32_t u){int32_t s;memcpy(&s,&u,4);return s;}
static int32_t r32(const uint8_t*m,unsigned o){return signed32(u16(m+DG+o)|((uint32_t)u16(m+DG+o+2)<<16));}
static const char *ptr(const uint8_t*m,unsigned o){unsigned a=u16(m+DG+o)+16u*u16(m+DG+o+2),n=0;if(!a)return "";while(a+n<1048576u&&n<255u){if(!m[a+n])return (const char*)m+a;++n;}return "";}
static int rectfill(uint8_t*fb,const uint8_t*m,unsigned o,uint8_t color){return sim_mcga_fill(fb,r(m,o+2),r(m,o),r(m,o+6),r(m,o+4),color,SIM_MCGA_COPY);}
static int rectframe(uint8_t*fb,const uint8_t*m,unsigned o,int thick,uint8_t color){return sim_ui_frame(fb,r(m,o+2),r(m,o),r(m,o+6),r(m,o+4),thick,color);}
static void button(const uint8_t*m,unsigned i,int *x,int *y){*x=r(m,0x2a4e)+(int)(i&1)*27+1;*y=r(m,0x2a4c)+(int)(i/2)*18+2;}
/* Original1f5e8 tie policy: diagonal step on error>=0. */
static void line(uint8_t*fb,int x,int y,int ex,int ey,uint8_t color){int dx=ex-x,dy=ey-y,sx=1,sy=1,mx,my,major,minor,error,i;if(dx<0){dx=-dx;sx=-1;}if(dy<0){dy=-dy;sy=-1;}mx=sx;my=0;major=dx;minor=dy;if(dx<dy){major=dy;minor=dx;mx=0;my=sy;}error=2*minor-major;for(i=0;i<=major;++i){if(x>=0&&x<320&&y>=0&&y<200)fb[y*320+x]=color;if(error<0){x+=mx;y+=my;error+=2*minor;}else{x+=sx;y+=sy;error+=2*minor-2*major;}}}
static int clippedtext(uint8_t*fb,const uint8_t*font,int x,int y,const char*s,uint8_t fg,uint8_t bg,int x0,int y0,int x1,int y1){unsigned g,xx,yy;for(;*s;++s){g=(unsigned char)*s;if(g>=128)return 0;for(yy=0;yy<8;++yy)for(xx=0;xx<8;++xx){int px=x+(int)xx,py=y+(int)yy;if(px>=x0&&px<=x1&&py>=y0&&py<=y1&&px>=0&&px<320&&py>=0&&py<200)fb[py*320+px]=font[g*64+yy*8+xx]?fg:bg;}x+=8;if(x>=320){x=0;y+=8;}}return 1;}
int sim_graphs_ui_activate(uint8_t*m,size_t size,unsigned i){if(!m||size<1048576u||i>7)return 0;if(i<6){m[DG+0x2ed0+i]^=1;if(m[DG+0x2ed0+i])m[DG+0x2ec2]=(uint8_t)i;}else{m[DG+0x2ec1]=(uint8_t)(i-6);m[DG+0x2ec2]=(uint8_t)i;}return 1;}
int sim_graphs_ui_hit(const uint8_t*m,size_t size,int x,int y){unsigned i;int bx,by;if(!m||size<1048576u)return -1;for(i=0;i<8;++i){button(m,i,&bx,&by);if(x>=bx&&x<=bx+26&&y>=by&&y<=by+17)return (int)i;}return -1;}
int sim_graphs_ui_cursor(const uint8_t*m,size_t size,unsigned i,int*x,int*y){if(!m||size<1048576u||i>7||!x||!y)return 0;button(m,i,x,y);*x+=13;*y+=8;return 1;}
int sim_graphs_ui_render(uint8_t*fb,const uint8_t*font,const uint8_t*m,size_t size,const uint8_t*frame,size_t n){
 int top,bottom,left,right,good,i,k,bx,by,max=0,range,phase,oldx=0,oldy=0,baseyear,interval,factor,year,relative;int32_t step,q,scale,sum,amount,ticks;unsigned a,index;char label[32];const char *title;
 if(!fb||!font||!m||size<1048576u||!frame||n<4260||u16(frame)!=56||u16(frame+2)!=76)return 0;
 top=r(m,0x2a3c);left=r(m,0x2a3e);bottom=r(m,0x2a40);right=r(m,0x2a42);range=m[DG+0x2ec1]!=0;index=m[DG+0x2ec2];if(index>7)return 0;
 good=sim_mcga_fill(fb,r(m,0x2a4e),r(m,0x2a4c),r(m,0x2a52),bottom,31,SIM_MCGA_COPY)&&rectframe(fb,m,0x2a24,5,9)&&rectframe(fb,m,0x2a24,2,14)&&rectframe(fb,m,0x2a4c,-2,9);
 good=good&&sim_mcga_blit(fb,r(m,0x2a4e),r(m,0x2a4c),frame+4,n-4,56,76,0,0,SIM_MCGA_COPY);
 for(i=0;i<6;++i)if(m[DG+0x2ed0+(unsigned)i]){button(m,(unsigned)i,&bx,&by);good=good&&sim_ui_frame(fb,bx,by,bx+26,by+17,3,(uint8_t)(u16(m+DG+0x2ec4+(unsigned)i*2)>>8));}
 button(m,(unsigned)(range+6),&bx,&by);good=good&&sim_ui_frame(fb,bx,by,bx+26,by+17,3,14);
 good=good&&rectfill(fb,m,0x2a2c,11)&&rectfill(fb,m,0x2a34,11)&&sim_ui_text(fb,font,r(m,0x2a36),r(m,0x2a34),"\x0f",15,11);
 title=ptr(m,0x532e + index*4);good=good&&sim_ui_text(fb,font,r(m,0x2a2e)+(r(m,0x2a32)-r(m,0x2a2e)-(int)strlen(title)*8)/2,r(m,0x2a2c),title,15,11);
 good=good&&rectfill(fb,m,0x2a3c,11);if(right<=left||bottom<=top)return 0;step=(int32_t)(((int64_t)(right-left)*65536)/40);ticks=r32(m,0x5f70);phase=4-(int)(range?ticks/48:ticks>>4);q=left*65536;
 for(i=0;i<40;++i){int x=q>>16;if((phase&3)==0)line(fb,x,top,x,bottom,9);else{line(fb,x,top,x,top+4,9);line(fb,x,bottom,x,bottom-4,9);}++phase;q+=step;}
 {static const unsigned peaks[2][3]={{0xd31a,0xc938,0x5272},{0x6b40,0x5354,0x5f74}};for(i=0;i<3;++i)if(max<r(m,peaks[range][i]))max=r(m,peaks[range][i]);}
 for(i=0;i<6;++i)if(m[DG+0x2ed0+(unsigned)i]){scale=max>128?max*128:16384;if(i==3)scale=32768;a=u16(m+DG+0x2ed8+(unsigned)i*4)+16u*u16(m+DG+0x2eda+(unsigned)i*4)+(range?240u:0u);if(a+240>1048576u)return 0;q=right*65536-step;for(k=0;k<40;++k){int x=q>>16,y;sum=(int16_t)u16(m+a)+(int16_t)u16(m+a+2)+(int16_t)u16(m+a+4);a+=6;amount=signed32((uint32_t)(bottom-top-6)*(uint32_t)sum);amount=signed32((uint32_t)amount<<7);amount=amount/3/scale;y=(int16_t)(bottom-(int16_t)amount-1);if(y>=bottom)y=bottom-1;else if(y<=top)y=top+1;if(k){uint8_t c=(uint8_t)(u16(m+DG+0x2ec4+(unsigned)i*2)>>8);line(fb,oldx,oldy,x,y,c);line(fb,oldx,oldy-1,x,y-1,c);}oldx=x;oldy=y;q-=step;}}
 good=good&&rectfill(fb,m,0x2a44,11);baseyear=(int16_t)(ticks/48)-(range?120:10);interval=range?50:5;factor=range?1:12;year=(baseyear/interval)*interval;for(i=0;i<4;++i){int x,y=r(m,0x2a44)+1+(year%(interval*2)?4:0);relative=(int16_t)((year-baseyear)*factor);q=signed32((uint32_t)relative*(uint32_t)step)/3;x=(q>>16)+r(m,0x2a46)-16;snprintf(label,sizeof(label),"%d",(int16_t)(year+r(m,0x294c)));good=good&&clippedtext(fb,font,x,y,label,15,11,r(m,0x2a46),r(m,0x2a44),r(m,0x2a4a),r(m,0x2a48));year=(int16_t)(year+interval);}
 return good;
}

