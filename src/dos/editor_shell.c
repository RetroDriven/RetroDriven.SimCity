/* Original2fcca frame,30018/300fa toolbar,305ae caption. */
#include "editor_shell.h"
#include "mcga_draw.h"
#include "ui_draw.h"
#include "city_stats.h"
#include <string.h>
#define DG 0x4d2c0u
static unsigned u16(const uint8_t*p){return p[0]|((unsigned)p[1]<<8);}
static int r(const uint8_t*d,unsigned o){return (int16_t)u16(d+o);}
static int32_t r32(const uint8_t*d,unsigned o){uint32_t u=u16(d+o)|((uint32_t)u16(d+o+2)<<16);int32_t n;memcpy(&n,&u,4);return n;}
static const char *ptr(const uint8_t*m,unsigned o){unsigned a=u16(m+DG+o)+16u*u16(m+DG+o+2),i=0;if(!a)return "";while(a+i<1048576u&&i<255u){if(!m[a+i])return (const char*)m+a;++i;}return "";}
static int fill(uint8_t*fb,const uint8_t*d,unsigned o,uint8_t c){return sim_mcga_fill(fb,r(d,o+2),r(d,o),r(d,o+6),r(d,o+4),c,SIM_MCGA_COPY);}
static int frame(uint8_t*fb,const uint8_t*d,unsigned o,int thick,uint8_t c){return sim_ui_frame(fb,r(d,o+2),r(d,o),r(d,o+6),r(d,o+4),thick,c);}
int sim_editor_shell_metrics(uint8_t*m,size_t n){int cap;if(!m||n<1048576u)return 0;cap=(r(m+DG,0x29be)-r(m+DG,0x298a)+1)/8;m[DG+0x4cca]=(uint8_t)cap;m[DG+0x4ccb]=(uint8_t)((unsigned)cap>>8);return 1;}
int sim_editor_tool_rectangle(const uint8_t*m,size_t n,unsigned tool,SimWindowRect*q){unsigned i;const uint8_t*d;if(!m||n<1048576u||!q||tool<1||tool>16)return 0;d=m+DG;if(tool==16){q->top=r(d,0x29b0);q->left=r(d,0x29b2);q->bottom=r(d,0x29b4);q->right=r(d,0x29b6);return 1;}i=(tool==15?12:tool)-1;q->left=r(d,0x299a)+3+(int)(i&1)*26;q->top=r(d,0x2998)+(int)(i/2)*17;q->right=q->left+23;q->bottom=q->top+14;return 1;}
int sim_editor_tool_hit(const uint8_t*m,size_t n,int x,int y){unsigned t;SimWindowRect q;if(!m||n<1048576u)return 0;for(t=1;t<=14;++t)if(sim_editor_tool_rectangle(m,n,t,&q)&&x>=q.left&&x<=q.right&&y>=q.top&&y<=q.bottom)return t==12&&m[DG+0x2b1b]==1?15:(int)t;if(sim_editor_tool_rectangle(m,n,16,&q)&&x>=q.left&&x<=q.right&&y>=q.top&&y<=q.bottom)return 16;return 0;}
int sim_editor_shell_render(uint8_t*fb,const sim_graphics_pack*pack,const uint8_t*m,size_t n){const uint8_t*d;sim_graphics_frame toolbar,demand;sim_graphics_span font;char name[96];const char*label;unsigned t,i;int good,pad,field,x,y;size_t len;if(!fb||!pack||!m||n<1048576u)return 0;d=m+DG;if(d[0x2950]!=6)return 0;if(sim_graphics_pack_frame(pack,1,0,&toolbar)!=SIM_GRAPHICS_OK||sim_graphics_pack_frame(pack,2,0,&demand)!=SIM_GRAPHICS_OK||sim_graphics_pack_section(pack,SIM_GRAPHICS_FONT_MASKS,&font)!=SIM_GRAPHICS_OK||toolbar.width!=56||toolbar.height!=119||demand.width!=48||demand.height!=32)return 0;
 good=fill(fb,d,0x2978,7)&&fill(fb,d,0x29b8,8)&&sim_mcga_fill(fb,r(d,0x299a),r(d,0x2998),r(d,0x299e),r(d,0x29a4),16,SIM_MCGA_COPY)&&fill(fb,d,0x29a8,17);
 label=d[0x2b1a]==16?(const char*)d+0x321a:ptr(m,0xca42+(unsigned)d[0x2b1a]*4);good=good&&sim_ui_text(fb,font.data,r(d,0x2992),r(d,0x29a8)+1,label,1,15)&&sim_mcga_blit(fb,r(d,0x299a)+(int8_t)d[0x3202],r(d,0x2998),toolbar.pixels.data,toolbar.pixels.size,toolbar.width,toolbar.height,0,0,SIM_MCGA_COPY);
 for(t=1;t<=14;++t){unsigned p=t==12?14:t-1;int32_t price=r32(d,0x2414+p*4);if(r32(d,0x2960)<price){SimWindowRect q;if(!sim_editor_tool_rectangle(m,n,t,&q))return 0;for(y=q.top;y<=q.bottom;++y)for(x=q.left;x<=q.right;++x)if((x+y)&1){if(x<0||x>=320||y<0||y>=200)return 0;fb[y*320+x]=255;}}}
 t=d[0x2b1a];if(t>=1&&t<=15){SimWindowRect q;if(!sim_editor_tool_rectangle(m,n,t,&q))return 0;good=good&&sim_ui_frame(fb,q.left,q.top,q.right,q.bottom,2,14);}
 good=good&&fill(fb,d,0x29b0,15)&&sim_ui_text(fb,font.data,r(d,0x29b2),r(d,0x29b0)+1,"+",1,15)&&sim_mcga_blit(fb,r(d,0x29a2)+(int8_t)d[0x31fa],r(d,0x29a0),demand.pixels.data,demand.pixels.size,demand.width,demand.height,0,0,SIM_MCGA_COPY)&&frame(fb,d,0x2970,3,0)&&frame(fb,d,0x2970,1,0)&&frame(fb,d,0x2990,-1,15);
 good=good&&sim_city_stats_funds_render(fb,font.data,d,65536)&&sim_city_stats_date_render(fb,font.data,d,65536)&&sim_city_stats_demand_render(fb,d,65536);
 len=0;while(len<21u&&d[0xcaca+len])++len;if(len==21u)return 0;field=(r(d,0x297e)-r(d,0x297a))/8-12;pad=field>(int)len?(field-(int)len)/2:0;if((size_t)pad+len>=sizeof(name))return 0;memset(name,32,(size_t)pad);for(i=0;i<len;++i)name[(unsigned)pad+i]=(char)d[0xcaca+i];name[(unsigned)pad+len]=0;y=r(d,0x2978)+(r(d,0x297c)-8-r(d,0x2978)+1)/2;good=good&&sim_ui_text(fb,font.data,r(d,0x297a)+8,y,name,8,7)&&sim_ui_text(fb,font.data,r(d,0x2982),r(d,0x2980),"\x0f",8,7);return good;
}

int sim_editor_shell_repaint(uint8_t*m,size_t n,uint32_t clock,SimMessageEffects*e)
{
    uint8_t *d;char money[32];size_t length;SimWindowRect q;unsigned tool;
    int pending;
    if(!m||n<1048576u||!e)return 0;
    d=m+DG;
    if(d[0x32f2]){memset(e,0,sizeof(*e));return 1;}
    if(!sim_editor_shell_metrics(m,n))return 0;
    /* Last original toolbar lookup is its highlighted tool, or the14th
     * cost-mask rectangle when Query has no highlight. */
    tool=d[0x2b1a]==16?14:d[0x2b1a];
    if(!sim_editor_tool_rectangle(m,n,tool,&q))return 0;
    d[0x4d00]=(uint8_t)q.top;d[0x4d01]=(uint8_t)((unsigned)q.top>>8);
    d[0x4d02]=(uint8_t)q.left;d[0x4d03]=(uint8_t)((unsigned)q.left>>8);
    d[0x4d04]=(uint8_t)q.bottom;d[0x4d05]=(uint8_t)((unsigned)q.bottom>>8);
    d[0x4d06]=(uint8_t)q.right;d[0x4d07]=(uint8_t)((unsigned)q.right>>8);
    d[0x3264]=9;d[0x3265]=0;
    if(d[0x2969]&&(d[0x2959]||d[0x5326]==1)){
        if(!sim_city_stats_money(money,sizeof(money),r32(d,0x2960)))return 0;
        length=strlen(money);memcpy(d+0x4e24,money,length+1);
        if(length>10)memmove(d+0x4e24,d+0x4e25,length);
    }
    d[0x124]=d[0x125]=8;d[0x126]=d[0x127]=7;
    d[0x295a]=0;d[0x3210]=d[0x3211]=0;pending=r(d,0x320c);
    if(!sim_message_dispatch(m,n,0,clock,e))return 0;
    if(pending>0&&!r(d,0x320c)){
        d[0x124]=d[0x125]=7;d[0x126]=d[0x127]=8;
    }
    return 1;
}
