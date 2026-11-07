#ifndef SIMCITY_DOS_SIMULATION_INTERNAL_H
#define SIMCITY_DOS_SIMULATION_INTERNAL_H
#include "simulation.h"

static inline uint32_t linear(uint16_t segment,uint16_t offset)
{ return (((uint32_t)segment<<4)+offset)&0xfffffu; }
static inline uint8_t m8(const sim_simulation *s,uint32_t a)
{ return s->memory[a&0xfffffu]; }
static inline uint16_t m16(const sim_simulation *s,uint32_t a)
{ return (uint16_t)(m8(s,a)|((uint16_t)m8(s,a+1)<<8)); }
static inline uint32_t m32(const sim_simulation *s,uint32_t a)
{ return m16(s,a)|((uint32_t)m16(s,a+2)<<16); }
static inline void w8(sim_simulation *s,uint32_t a,uint32_t v)
{ s->memory[a&0xfffffu]=(uint8_t)v; }
static inline void w16(sim_simulation *s,uint32_t a,uint32_t v)
{ w8(s,a,v);w8(s,a+1,v>>8); }
static inline void w32(sim_simulation *s,uint32_t a,uint32_t v)
{ w16(s,a,v);w16(s,a+2,v>>16); }
static inline void push(sim_simulation *s,uint16_t value)
{ s->r[4]=(uint16_t)(s->r[4]-2);w16(s,linear(s->segment[2],s->r[4]),value); }
static inline uint16_t pop(sim_simulation *s)
{ uint16_t value=m16(s,linear(s->segment[2],s->r[4]));s->r[4]=(uint16_t)(s->r[4]+2);return value; }
static inline void setco(sim_simulation *s,int value)
{ s->flags=(uint16_t)((s->flags&~2049u)|(value?2049u:0)); }
static inline uint16_t szp(uint32_t value,unsigned bits)
{
    unsigned low=value&255u,parity=0,i;
    for(i=0;i<8;++i)parity^=(low>>i)&1u;
    return (uint16_t)((value==0?64:0)|((value>>(bits-1))&1u)*128u|(parity?0:4));
}
/* Arithmetic and logical instructions preserve unaffected architectural flags.
 * AF is cleared for logical operations, matching the Unicorn comparison
 * model; x86 leaves that flag architecturally undefined for these operations. */
static inline uint32_t alu(sim_simulation *s,uint32_t a,uint32_t b,
                            unsigned bits,unsigned operation,unsigned carry)
{
    uint32_t mask=(1u<<bits)-1u,sign=1u<<(bits-1),result,full;
    unsigned cf=0,of=0,af=0;
    a&=mask;b&=mask;
    if(operation==0){
        full=a+b+carry;result=full&mask;cf=full>mask;
        of=((~(a^b)&(a^result)&sign)!=0);af=((a^b^result)&16u)!=0;
    }else if(operation==1){
        result=(a-b-carry)&mask;cf=a<b+carry;
        of=(((a^b)&(a^result)&sign)!=0);af=((a^b^result)&16u)!=0;
    }else{
        result=operation==2?(a&b):(operation==3?(a|b):(a^b));
        af=0;
    }
    s->flags=(uint16_t)((s->flags&~2261u)|szp(result,bits)|cf|(af?16u:0)|(of?2048u:0));
    return result;
}

static inline uint32_t shift(sim_simulation *s,uint32_t value,unsigned bits,
                              unsigned operation,unsigned count)
{
    uint32_t mask=(1u<<bits)-1u,sign=1u<<(bits-1),original=value&mask;
    unsigned cf=(s->flags&1u)!=0,of=(s->flags&2048u)!=0,i;
    count&=31u;value&=mask;if(!count)return value;
    for(i=0;i<count;++i){
        unsigned next;
        switch(operation){
        case 0:cf=(value&sign)!=0;value=(value<<1)&mask;break;
        case 1:cf=value&1u;value>>=1;break;
        case 2:cf=value&1u;value=(value>>1)|(value&sign);break;
        case 3:cf=(value&sign)!=0;value=((value<<1)|cf)&mask;break;
        case 4:cf=value&1u;value=(value>>1)|(cf?sign:0);break;
        case 5:next=(value&sign)!=0;value=((value<<1)|cf)&mask;cf=next;break;
        default:next=value&1u;value=(value>>1)|(cf?sign:0);cf=next;break;
        }
    }
    if(count==1){
        if(operation==0||operation==3||operation==5)of=((value&sign)!=0)^cf;
        else if(operation==1)of=(original&sign)!=0;
        else if(operation==2)of=0;
        else of=((value&sign)!=0)^((value&(sign>>1))!=0);
    }
    s->flags=(uint16_t)((s->flags&~2049u)|cf|(of?2048u:0));
    if(operation<3)s->flags=(uint16_t)((s->flags&~196u)|szp(value,bits));
    return value;
}

static inline int divide(sim_simulation *s,uint32_t divisor,unsigned bits,int sign)
{
    uint32_t dividend;int32_t numerator,denominator,q,r;
    divisor&=(1u<<bits)-1u;if(!divisor)return 0;
    dividend=bits==8?s->r[0]:((uint32_t)s->r[2]<<16)|s->r[0];
    if(sign){
        numerator=bits==8?(int16_t)dividend:(int32_t)dividend;
        denominator=bits==8?(int8_t)divisor:(int16_t)divisor;
        if(numerator==(-2147483647-1)&&denominator==-1)return 0;
        q=numerator/denominator;r=numerator%denominator;
        if(bits==8){if(q< -128||q>127)return 0;s->r[0]=(uint16_t)((uint8_t)q|((uint16_t)(uint8_t)r<<8));}
        else {if(q< -32768||q>32767)return 0;s->r[0]=(uint16_t)q;s->r[2]=(uint16_t)r;}
    }else{
        uint32_t uq=dividend/divisor,ur=dividend%divisor;
        if(bits==8){if(uq>255)return 0;s->r[0]=(uint16_t)(uq|(ur<<8));}
        else{if(uq>65535)return 0;s->r[0]=(uint16_t)uq;s->r[2]=(uint16_t)ur;}
    }
    return 1;
}

static inline void string_op(sim_simulation *s,unsigned operation,unsigned bits,
                              int repeat,unsigned rep_kind)
{
    unsigned n=repeat?s->r[1]:1,bytes=bits/8;
    int direction=(s->flags&1024u)?-(int)bytes:(int)bytes;
    while(n--){
        uint32_t source=linear(s->segment[3],s->r[6]),dest=linear(s->segment[0],s->r[7]);
        uint32_t a=bits==8?m8(s,source):m16(s,source),b=bits==8?m8(s,dest):m16(s,dest);
        if(operation==0){if(bits==8)w8(s,dest,a);else w16(s,dest,a);}
        if(operation==1){if(bits==8)s->r[0]=(uint16_t)((s->r[0]&0xff00u)|a);else s->r[0]=(uint16_t)a;}
        if(operation==2){if(bits==8)w8(s,dest,s->r[0]);else w16(s,dest,s->r[0]);}
        if(operation==3)alu(s,s->r[0],b,bits,1,0);
        if(operation==4)alu(s,a,b,bits,1,0);
        if(operation==0||operation==1||operation==4)s->r[6]=(uint16_t)(s->r[6]+direction);
        if(operation!=1)s->r[7]=(uint16_t)(s->r[7]+direction);
        if(repeat){s->r[1]=(uint16_t)(s->r[1]-1);if(operation>=3&&(((s->flags&64u)!=0)!=(rep_kind==1)))break;}
    }
}

static inline sim_run_result fault(sim_simulation *s)
{ s->fault_pc=s->pc;s->running=0;return SIM_RUN_FAULT; }
sim_run_result sim_external(sim_simulation *s);
sim_run_result sim_lifted_run(sim_simulation *s,uint32_t budget);
#endif
