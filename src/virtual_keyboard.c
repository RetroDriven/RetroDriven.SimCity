#include "virtual_keyboard.h"
#include "dos/mcga_draw.h"
#include "dos/ui_draw.h"
#include <string.h>
static const char rows[4][11]={"1234567890","QWERTYUIOP","ASDFGHJKL'","ZXCVBNM &?"};
static const char *actions[5]={"Space","Back","Clear","Cancel","Done"};
static const unsigned codes[5]={32,8,SIM_VK_CLEAR,SIM_VK_CANCEL,SIM_VK_DONE};
void sim_virtual_keyboard_init(SimVirtualKeyboard *k){memset(k,0,sizeof(*k));k->row=1;}
void sim_virtual_keyboard_step(SimVirtualKeyboard *k,int dx,int dy)
{
    int old=k->row;k->row=(k->row+dy+5)%5;
    if(old==4&&k->row!=4)k->column=k->column*2;
    else if(old!=4&&k->row==4)k->column/=2;
    int count=k->row==4?5:10;k->column=(k->column+dx+count)%count;
}
unsigned sim_virtual_keyboard_selected(const SimVirtualKeyboard *k)
{
    if(k->row==4)return codes[k->column];
    unsigned c=(unsigned char)rows[k->row][k->column];
    if(k->lowercase&&c>='A'&&c<='Z')c+='a'-'A';
    return c;
}
int sim_virtual_keyboard_hit(SimVirtualKeyboard *k,int x,int y)
{
    if(x<20||x>=300)return 0;
    if(y>=86&&y<150){int row=(y-86)/16,col=(x-20)/28;
        if((x-20)%28>=26||(y-86)%16>=14)return 0;
        k->row=row;k->column=col;return 1;}
    if(y>=155&&y<=172){k->row=4;k->column=(x-20)/56;return 1;}
    return 0;
}
static void key(uint8_t *fb,const uint8_t *font,int x,int y,int width,int height,const char *label,int focus)
{
    uint8_t bg=focus?11:7;
    sim_mcga_fill(fb,x,y,x+width-1,y+height-1,bg,SIM_MCGA_COPY);
    sim_ui_frame(fb,x,y,x+width-1,y+height-1,1,focus?14:15);
    sim_ui_text(fb,font,x+(width-(int)strlen(label)*8)/2,y+(height-8)/2,label,1,bg);
}
void sim_virtual_keyboard_render(uint8_t *fb,const uint8_t *font,const SimVirtualKeyboard *k,const sim_name_edit *edit,int blink)
{
    sim_mcga_fill(fb,12,40,307,191,7,SIM_MCGA_COPY);sim_ui_frame(fb,12,40,307,191,2,0);
    sim_mcga_fill(fb,16,44,303,55,11,SIM_MCGA_COPY);sim_ui_text(fb,font,24,46,"City Name",1,11);
    sim_mcga_fill(fb,24,60,295,79,15,SIM_MCGA_COPY);sim_ui_frame(fb,24,60,295,79,1,0);
    sim_ui_text(fb,font,28,65,edit->text,9,15);
    if(blink)sim_ui_text(fb,font,28+edit->cursor*8,69,"_",9,15);
    for(int r=0;r<4;++r)for(int c=0;c<10;++c){
        char label[2]={rows[r][c],0};
        if(k->lowercase&&label[0]>='A'&&label[0]<='Z')label[0]+='a'-'A';
        key(fb,font,20+c*28,86+r*16,26,14,label,r==k->row&&c==k->column);
    }
    for(int c=0;c<5;++c)key(fb,font,20+c*56,155,54,18,actions[c],k->row==4&&c==k->column);
    sim_ui_text(fb,font,28,179,"X Back  Y Case  Start Done",1,7);
}
