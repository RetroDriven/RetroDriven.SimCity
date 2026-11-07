#include "placement_preview.h"
#include "dos/placement.h"
int sim_placement_preview_bounds(const sim_city_view *v,int x,int y,unsigned tool,uint16_t tile,SimPlacementPreview *r)
{
    sim_city_tool_info info;
    if(!v||!r||!sim_city_tool_info_get(tool,&info)||!v->columns||!v->rows||
       x<v->screen_x||y<v->screen_y||x>=v->screen_x+(int)v->columns*8||y>=v->screen_y+(int)v->rows*8)return 0;
    int size=(int)info.width,dx=-(size-1)/2,dy=dx;
    if(tool==SIM_CITY_BULLDOZE&&(tile&0x400)){
        size=sim_city_demolition_size(tile&1023,&dx,&dy);
        if(!size)return 0;
    }
    r->left=v->screen_x+((x-v->screen_x)/8+dx)*8;
    r->top=v->screen_y+((y-v->screen_y)/8+dy)*8;
    r->right=r->left+size*8-1;r->bottom=r->top+size*8-1;
    return 1;
}
static void pixel(uint8_t *fb,const sim_city_view *v,int x,int y,uint8_t color)
{
    if(x>=0&&x<320&&y>=0&&y<200&&x>=v->screen_x&&y>=v->screen_y&&
       x<v->screen_x+(int)v->columns*8&&y<v->screen_y+(int)v->rows*8)fb[y*320+x]=color;
}
static void border(uint8_t *fb,const sim_city_view *v,const SimPlacementPreview *r,int pad,uint8_t color)
{
    int l=r->left-pad,t=r->top-pad,rr=r->right+pad,b=r->bottom+pad;
    for(int x=l;x<=rr;++x){pixel(fb,v,x,t,color);pixel(fb,v,x,b,color);}
    for(int y=t;y<=b;++y){pixel(fb,v,l,y,color);pixel(fb,v,rr,y,color);}
}
int sim_placement_preview_render(uint8_t *fb,const sim_city_view *v,int x,int y,unsigned tool,uint16_t tile)
{
    SimPlacementPreview r;
    if(!fb||!sim_placement_preview_bounds(v,x,y,tool,tile,&r))return 0;
    border(fb,v,&r,1,0);border(fb,v,&r,0,14);
    for(int x=r.left+8;x<r.right;x+=8)for(int d=1;d<=2;++d){pixel(fb,v,x,r.top+d,14);pixel(fb,v,x,r.bottom-d,14);}
    for(int y=r.top+8;y<r.bottom;y+=8)for(int d=1;d<=2;++d){pixel(fb,v,r.left+d,y,14);pixel(fb,v,r.right-d,y,14);}
    return 1;
}
