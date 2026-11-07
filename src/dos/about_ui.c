#include "about_ui.h"
#include "ui_draw.h"
#include "mcga_draw.h"
#include <string.h>
#define DG 0x4d2c0u
int sim_about_ui_render(uint8_t*fb,const uint8_t*font,const uint8_t*m,size_t size){static const unsigned offsets[7]={0x22fc,0x231f,0x2342,0x2365,0x2388,0x23ab,0x23ce};static const int ys[7]={60,68,76,87,95,103,111};int i,good;if(!fb||!font||!m||size<1048576u)return 0;good=sim_mcga_fill(fb,8,48,311,143,25,SIM_MCGA_COPY)&&sim_ui_frame(fb,8,48,311,143,10,9)&&sim_ui_frame(fb,8,48,311,143,3,14);for(i=0;i<7;++i){const char*s=(const char*)m+DG+offsets[i];unsigned n=0;while(n<128u&&s[n])++n;if(n==128u)return 0;good=good&&sim_ui_text(fb,font,24,ys[i],s,9,15);}return good;}
