#include "opl_patch.h"
#include <string.h>
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t longword(const uint8_t *p){return word(p)|(uint32_t)word(p+2)<<16;}
static void operator_decode(sim_opl_operator *o,const uint8_t *p)
{
    o->characteristic=p[0]&0xf0;
    o->multiplier=(uint16_t)((unsigned)p[0]<<12);
    o->ksl=p[1]&0xc0;
    o->level=(uint16_t)(((~(unsigned)p[1])&0x3f)<<10);
    o->attack_decay=p[2];o->sustain_release=p[3];o->waveform=p[4];
}
int sim_opl_patch_decode(sim_opl_patch *out,const uint8_t *record,size_t size)
{
    sim_opl_patch p;unsigned length,algorithm;
    static const uint8_t masks[4]={0,1,2,1},extra[4]={2,2,2,3};
    if(!out||!record||size<2)return 0;
    length=word(record);if((length!=14&&length!=25)||size<length)return 0;
    memset(&p,0,sizeof(p));p.transpose=record[2];p.connection=record[8]&1;
    p.feedback=(uint16_t)((unsigned)record[8]<<12);p.operators=2;
    operator_decode(p.op,record+3);operator_decode(p.op+1,record+9);
    p.volume_mask=p.connection|2;
    if(length==25){
        p.operators=4;p.connection|=(record[8]&0x80)>>6;
        algorithm=p.connection;p.volume_mask=masks[algorithm];p.extra_volume_mask=extra[algorithm];
        operator_decode(p.op+2,record+14);operator_decode(p.op+3,record+20);
    }
    *out=p;return 1;
}
int sim_opl_library_find(const uint8_t *library,size_t size,unsigned patch,
    unsigned bank,const uint8_t **record,size_t *record_size)
{
    size_t pos;uint32_t offset;unsigned length;
    if(!library||!record||!record_size||patch>254||bank>255)return 0;
    for(pos=0;pos<size;pos+=6){
        if(library[pos]==255)return 0;
        if(size-pos<6)return 0;
        if(library[pos]!=patch||library[pos+1]!=bank)continue;
        offset=longword(library+pos+2);
        if(offset>size||size-offset<2)return 0;
        length=word(library+offset);
        if(length<2||size-offset<length)return 0;
        *record=library+offset;*record_size=length;return 1;
    }
    return 0;
}
int sim_opl_patch_program2(const sim_opl_patch *p,unsigned channel,
    unsigned gain,unsigned modulation,sim_opl_write out[11])
{
    static const uint8_t offsets[9]={0,1,2,8,9,10,16,17,18};
    unsigned i,n=0,op_offset[2],level;
    if(!p||!out||p->operators!=2||channel>=9||gain>127||modulation>127)return 0;
    op_offset[0]=offsets[channel];op_offset[1]=offsets[channel]+3;
#define WRITE(r,v) do{out[n].reg=(uint16_t)(r);out[n++].value=(uint8_t)(v);}while(0)
    for(i=0;i<2;++i)WRITE(0x20+op_offset[i],(p->op[i].multiplier>>12)|p->op[i].characteristic|(modulation>=64?0x40:0));
    for(i=0;i<2;++i){
        level=p->op[i].level>>10;
        if(p->volume_mask&(1u<<i))level=level*gain/127;
        WRITE(0x40+op_offset[i],((~level)&0x3f)|p->op[i].ksl);
    }
    for(i=0;i<2;++i)WRITE(0x60+op_offset[i],p->op[i].attack_decay);
    for(i=0;i<2;++i)WRITE(0x80+op_offset[i],p->op[i].sustain_release);
    WRITE(0xe0+op_offset[1],p->op[1].waveform);
    WRITE(0xe0+op_offset[0],p->op[0].waveform);
    WRITE(0xc0+channel,((p->feedback>>12)&0xe)|(p->connection&1));
#undef WRITE
    return n==11;
}
