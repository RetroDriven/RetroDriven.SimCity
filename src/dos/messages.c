#include "messages.h"
#include <string.h>

#define DG 0x4d2c0u
typedef struct message_target { uint16_t offset,segment,count;uint8_t tagged; } message_target;
static const message_target targets[22]={
    {0x60ae,0x38a3,50,1},{0xca46,0x4d2c,16,0},{0x512c,0x4d2c,21,1},
    {0xca00,0x4d2c,4,0},{0x50d6,0x4d2c,12,0},{0x5f4e,0x4d2c,3,0},
    {0x5106,0x4d2c,8,0},{0x532e,0x4d2c,8,0},{0xcade,0x4d2c,3,0},
    {0x5f3a,0x4d2c,3,0},{0xca14,0x4d2c,11,0},{0xc97a,0x4d2c,11,0},
    {0x4fb8,0x4d2c,6,0},{0xd236,0x4d2c,7,0},{0xd322,0x4d2c,37,0},
    {0xd2bc,0x4d2c,5,0},{0x4f88,0x4d2c,5,0},{0xd2d2,0x4d2c,14,0},
    {0xd140,0x4d2c,8,0},{0x4f9c,0x4d2c,6,0},{0xca94,0x4d2c,9,0},
    {0x508c,0x4d2c,10,0}
};
static unsigned r16(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
static void w16(uint8_t *p,unsigned value){p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8);}

static int validate(const uint8_t *data,size_t size)
{
    size_t position=0,total=0;unsigned record;
    if(!data)return 0;
    for(record=0;record<22;++record){
        size_t cursor,end;unsigned length,count,i;
        if(position>size || size-position<4)return 0;
        length=r16(data+position);count=r16(data+position+2);position+=4;
        if(count!=targets[record].count || length>size-position)return 0;
        total+=(length+15u)&~15u;if(total>0x20000)return 0;
        cursor=position;end=position+length;
        for(i=0;i<count;++i){
            const uint8_t *nul;
            if(cursor>=end)return 0;
            if(data[cursor]==255)++cursor;
            else {nul=memchr(data+cursor,0,end-cursor);if(!nul)return 0;cursor=(size_t)(nul-data)+1;}
            if(targets[record].tagged){
                if(cursor>end || end-cursor<3 || data[cursor]!=254)return 0;
                cursor+=3;
            }
        }
        if(cursor!=end)return 0;
        position=end;
    }
    return position==size;
}

int sim_messages_install(uint8_t *memory,size_t memory_size,const uint8_t *data,size_t size)
{
    size_t position=0,heap=0x70000;unsigned record;
    if(!memory || memory_size<1048576 || !validate(data,size))return 0;
    for(record=0;record<22;++record){
        unsigned length=r16(data+position),i,offset=0;
        unsigned segment=(unsigned)(heap>>4);
        uint8_t *destination=memory+((unsigned)targets[record].segment<<4)+targets[record].offset;
        position+=4;memcpy(memory+heap,data+position,length);
        w16(memory+DG+0x4e3e +record*4,0);w16(memory+DG+0x4e40+record*4,segment);
        for(i=0;i<targets[record].count;++i){
            if(data[position+offset]==255){w16(destination,0);w16(destination+2,0);++offset;}
            else {
                w16(destination,offset);w16(destination+2,segment);
                offset+=(unsigned)strlen((const char *)(data+position+offset))+1;
            }
            if(targets[record].tagged){
                w16(destination+4,r16(data+position+offset+1));offset+=3;destination+=6;
            }else destination+=4;
        }
        position+=length;heap+=(length+15u)&~15u;
    }
    memory[DG+0x4ea2]=22;
    memmove(memory+DG+0xd154,memory+DG+0xd150,16);
    w16(memory+DG+0xd150,0x3930);w16(memory+DG+0xd152,0x4d2c);
    memmove(memory+DG+0x50a4,memory+DG+0x50a0,16);
    w16(memory+DG+0x50a0,0x393b);w16(memory+DG+0x50a2,0x4d2c);
    return 1;
}
