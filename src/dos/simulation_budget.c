#include "simulation_budget.h"

#define DG 0x4d2c0u
#define MEMORY_SIZE 0x100000u
static uint16_t rd16(const uint8_t *m,unsigned o)
{return (uint16_t)(m[DG+o]|((uint16_t)m[DG+o+1]<<8));}
static uint32_t rd32(const uint8_t *m,unsigned o)
{return rd16(m,o)|((uint32_t)rd16(m,o+2)<<16);}
static void wr16(uint8_t *m,unsigned o,uint16_t v)
{m[DG+o]=(uint8_t)v;m[DG+o+1]=(uint8_t)(v>>8);}
static void wr32(uint8_t *m,unsigned o,uint32_t v)
{wr16(m,o,(uint16_t)v);wr16(m,o+2,(uint16_t)(v>>16));}

int sim_budget_recalculate(uint8_t *m,size_t size)
{
    static const unsigned percentage[3]={0x36b4,0x36c8,0x36be};
    static const unsigned requested[3]={0x50ba,0xcac2,0x4f7a};
    static const unsigned allocation[3]={0x36ac,0x36c0,0x36b6};
    static const unsigned committed[3]={0xd168,0x52be,0x532a};
    uint32_t total=0;unsigned i;
    if(!m||size<MEMORY_SIZE)return 0;
    for(i=0;i<3;++i){
        /* The DOS helper first multiplies to32 bits, then signed-divides100.
         * uint32 truncation makes its wrap explicit before the signed divide. */
        uint32_t product=(uint32_t)((int64_t)(int16_t)rd16(m,percentage[i])*(int32_t)rd32(m,requested[i]));
        int32_t result=(int32_t)product/100;
        wr32(m,allocation[i],(uint32_t)result);wr32(m,committed[i],(uint32_t)result);
        total+=(uint32_t)result;
    }
    wr32(m,0x4dcc,total);return 1;
}

sim_budget_result sim_budget_prepare(uint8_t *m,size_t size,int annual)
{
    uint32_t available,remaining;
    if(!m||size<MEMORY_SIZE)return SIM_BUDGET_INVALID;
    wr16(m,0x36d2,rd16(m,0xd192));
    if(!sim_budget_recalculate(m,size))return SIM_BUDGET_INVALID;
    wr32(m,0x36b0,rd32(m,0x50ba));
    wr32(m,0x36c4,rd32(m,0xcac2));
    wr32(m,0x36ba,rd32(m,0x4f7a));
    wr32(m,0x36ce,20);
    available=rd32(m,0x5f7a)+rd32(m,0x2960);
    if((int32_t)available<(int32_t)rd32(m,0x4dcc)){
        m[DG+0x295c]=0;
        if((int32_t)available>=(int32_t)rd32(m,0x532a)){
            remaining=available-rd32(m,0x532a);
            if((int32_t)remaining>=(int32_t)rd32(m,0xd168)){
                remaining-=rd32(m,0xd168);
                if((int32_t)remaining<(int32_t)rd32(m,0x52be)){
                    wr32(m,0x52be,remaining);wr32(m,0x36c0,remaining);
                }
            }else{
                wr32(m,0xd168,remaining);wr32(m,0x36ac,remaining);
                wr32(m,0x52be,0);wr32(m,0x36c0,0);
            }
        }else{
            wr32(m,0x532a,available);wr32(m,0x36b6,available);
            wr32(m,0x52be,0);wr32(m,0x36c0,0);
            wr32(m,0xd168,0);wr32(m,0x36ac,0);
        }
        wr32(m,0x4dcc,available);
    }
    if(annual&&m[DG+0x295c]&&(int32_t)available>=(int32_t)rd32(m,0x4dcc))
        return SIM_BUDGET_AUTOMATIC;
    return SIM_BUDGET_INTERACTIVE;
}

int sim_budget_commit(uint8_t *m,size_t size,int annual)
{
    if(!m||size<MEMORY_SIZE)return 0;
    if(annual)wr32(m,0x2960,rd32(m,0x2960)+rd32(m,0x5f7a)-rd32(m,0x4dcc));
    wr16(m,0xd192,rd16(m,0x36d2));wr16(m,0x4f86,0);
    return 1;
}
