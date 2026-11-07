#include "evaluation_ui.h"
#include "ui_draw.h"
#include "mcga_draw.h"
#include <stdio.h>
#include <string.h>
#define DG 0x4d2c0u
static unsigned u16(const uint8_t*p){return p[0]|((unsigned)p[1]<<8);}
static int r(const uint8_t*m,unsigned o){return (int16_t)u16(m+DG+o);}
static int32_t r32(const uint8_t*m,unsigned o){uint32_t v=u16(m+DG+o)|((uint32_t)u16(m+DG+o+2)<<16);int32_t s;memcpy(&s,&v,4);return s;}
static const char *ptr(const uint8_t*m,unsigned o){unsigned a=u16(m+DG+o)+16u*u16(m+DG+o+2),n=0;if(!a)return "";while(a+n<1048576u&&n<255u){if(!m[a+n])return (const char*)m+a;++n;}return "";}
static void number(char*out,size_t size,int32_t value){char b[16],v[32];unsigned i,n=0,len;uint32_t mag=value<0?0u-(uint32_t)value:(uint32_t)value;snprintf(b,sizeof(b),"%u",(unsigned)mag);len=(unsigned)strlen(b);if(value<0)v[n++]='-';for(i=0;i<len;++i){v[n++]=b[i];if(i+1<len&&(len-i-1)%3==0)v[n++]=',';}v[n]=0;snprintf(out,size,"%s",v);}
int sim_evaluation_ui_render(uint8_t*fb,const uint8_t*font,const uint8_t*m,size_t size){
 int i,good=1,category,level;char line[256],n[32];
 if(!fb||!font||!m||size<1048576u)return 0;
 category=r(m,0xd16c);level=r(m,0xc9fe);if(category<0||category>5)return 0;if(level<0||level>2)level=0;
 good=sim_mcga_fill(fb,8,40,311,191,11,SIM_MCGA_COPY)&&sim_ui_frame(fb,8,40,311,191,2,9);
 good=good&&sim_ui_frame(fb,144,72,303,183,-2,9)&&sim_ui_frame(fb,16,72,151,111,-2,9)&&sim_ui_frame(fb,16,112,151,183,-2,9);
 good=good&&sim_mcga_fill(fb,144,72,303,183,15,SIM_MCGA_COPY)&&sim_mcga_fill(fb,16,72,151,111,15,SIM_MCGA_COPY)&&sim_mcga_fill(fb,16,112,151,183,15,SIM_MCGA_COPY);
 snprintf(line,sizeof(line),"%d City Evaluation",(int16_t)(r32(m,0x5f70)/48+r(m,0x294c)));
 good=good&&sim_ui_text(fb,font,8+((38-(int)strlen(line))&~1)*4,44,line,9,15);
 good=good&&sim_ui_text(fb,font,24,56,"PUBLIC OPINION",9,15)&&sim_ui_text(fb,font,184,56,"STATISTICS",9,15);
 good=good&&sim_ui_text(fb,font,16,72,"Is the mayor",0,15)&&sim_ui_text(fb,font,16,80,"doing a good job?",0,15)&&sim_ui_text(fb,font,16,112,"What are the",0,15)&&sim_ui_text(fb,font,16,120,"worst problems?",0,15);
 snprintf(line,sizeof(line),"%8d%% YES",r(m,0xd16e));good=good&&sim_ui_text(fb,font,16,88,line,9,15);
 snprintf(line,sizeof(line),"%8d%% NO",r(m,0xc940));good=good&&sim_ui_text(fb,font,16,96,line,9,15);
 for(i=0;i<4;++i){int problem=r(m,0xc9f4+(unsigned)i*2),votes;if(problem<0||problem>6)return 0;votes=r(m,0xd1b4+(unsigned)problem*2);if(votes){snprintf(line,sizeof(line),"%3d%%  %s",votes,ptr(m,0xd236+(unsigned)problem*4));good=good&&sim_ui_text(fb,font,16,128+i*8,line,(uint8_t)(votes>=20?12:9),15);}}
 good=good&&sim_ui_text(fb,font,152,72," Population",9,15);number(n,sizeof(n),r32(m,0xd21e));good=good&&sim_ui_text(fb,font,216,80,n,9,15);
 good=good&&sim_ui_text(fb,font,152,88," Net Migration",9,15);number(n,sizeof(n),r32(m,0x5f76));snprintf(line,sizeof(line),"(last year)%s",n);good=good&&sim_ui_text(fb,font,152,96,line,9,15);
 good=good&&sim_ui_text(fb,font,152,104,"Assessed Value",9,15);number(n,sizeof(n),r32(m,0xca8c)<0?(int32_t)(0u-(uint32_t)r32(m,0xca8c)):r32(m,0xca8c));snprintf(line,sizeof(line),"%s$%s",r32(m,0xca8c)<0?"-":" ",n);good=good&&sim_ui_text(fb,font,184,112,line,9,15);
 good=good&&sim_ui_text(fb,font,176,128,"Category:",9,15)&&sim_ui_text(fb,font,176,136,ptr(m,0x4fb8+(unsigned)category*4),9,15)&&sim_ui_text(fb,font,176,144,"Game Level",9,15)&&sim_ui_text(fb,font,184,152,ptr(m,0x2f26+(unsigned)level*4),9,15);
 good=good&&sim_ui_text(fb,font,40,160,"Overall City Score (0 - 1000)",0,15)&&sim_ui_text(fb,font,40,168,"current score:annual change",0,15);
 snprintf(line,sizeof(line),"%10d %10d",r(m,0xd30a),r(m,0xc942));return good&&sim_ui_text(fb,font,40,176,line,9,15);
}
