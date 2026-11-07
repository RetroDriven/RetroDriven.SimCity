#include "simulation_internal.h"
#include <string.h>

size_t sim_simulation_sizeof(void){return sizeof(sim_simulation);}
uint8_t *sim_simulation_dgroup(sim_simulation *s){return s?s->memory+SIM_DOS_DGROUP:NULL;}
uint8_t *sim_simulation_memory(sim_simulation *s){return s?s->memory:NULL;}

static unsigned read16(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}

int sim_simulation_init(sim_simulation *s,const uint8_t *exe,size_t size)
{
    size_t header,bytes,relocations,table,i;
    if(!s||!exe||size!=282913||exe[0]!='M'||exe[1]!='Z')return 0;
    header=read16(exe+8)*16u;relocations=read16(exe+6);table=read16(exe+0x18);
    if(header>size||table>header||relocations>(header-table)/4)return 0;
    bytes=size-header;if(bytes>SIM_DOS_MEMORY_SIZE-0x10000u)return 0;
    memset(s,0,sizeof(*s));memcpy(s->memory+0x10000,exe+header,bytes);
    for(i=0;i<relocations;++i){
        uint32_t at=read16(exe+table+i*4)+read16(exe+table+i*4+2)*16u;
        if(at+2>bytes){memset(s,0,sizeof(*s));return 0;}
        w16(s,0x10000u+at,m16(s,0x10000u+at)+0x1000u);
    }
    s->segment[2]=s->segment[3]=0x4d2c;s->flags=2;
    return 1;
}

void sim_simulation_return_far(sim_simulation *s,unsigned pop_bytes)
{
    uint16_t ip;if(!s)return;
    ip=pop(s);s->segment[1]=pop(s);s->r[4]=(uint16_t)(s->r[4]+pop_bytes);
    s->pc=linear(s->segment[1],ip);
}

void sim_simulation_cpu_save(const sim_simulation *s,sim_cpu_context *c)
{
    if(!s||!c)return;
    memcpy(c->r,s->r,sizeof(c->r));memcpy(c->segment,s->segment,sizeof(c->segment));
    c->flags=s->flags;c->pc=s->pc;c->fault_pc=s->fault_pc;c->running=s->running;
}

void sim_simulation_cpu_restore(sim_simulation *s,const sim_cpu_context *c)
{
    if(!s||!c)return;
    memcpy(s->r,c->r,sizeof(s->r));memcpy(s->segment,c->segment,sizeof(s->segment));
    s->flags=c->flags;s->pc=c->pc;s->fault_pc=c->fault_pc;s->running=c->running;
}

int sim_simulation_begin_aux(sim_simulation *s,uint32_t address,
                              uint16_t cs,uint16_t stack_top,
                              const uint16_t *arguments,size_t count)
{
    size_t i;
    if(!s||address>=SIM_DOS_MEMORY_SIZE||address<(uint32_t)cs*16u||
        address-(uint32_t)cs*16u>65535u||count>32||(!arguments&&count)||
        stack_top<0xd800u||stack_top>0xef00u)return 0;
    memset(s->r,0,sizeof(s->r));s->r[4]=stack_top;
    s->segment[0]=s->segment[2]=s->segment[3]=0x4d2c;s->segment[1]=cs;
    s->flags=2;s->pc=address;s->fault_pc=0;s->running=1;
    for(i=count;i>0;--i)push(s,arguments[i-1]);
    push(s,0xffff);push(s,0);return 1;
}

int sim_simulation_begin_aux_near(sim_simulation *s,uint32_t address,
                                   uint16_t cs,uint16_t stack_top)
{
    if(!sim_simulation_begin_aux(s,address,cs,stack_top,NULL,0))return 0;
    push(s,0xfff0);return 1;
}

sim_run_result sim_external(sim_simulation *s)
{
    if(s->pc==0xffff0u){s->running=0;return SIM_RUN_RETURNED;}
    if(s->pc==(uint32_t)s->segment[1]*16u+0xfff0u){
        sim_simulation_return_far(s,0);return SIM_RUN_BUDGET;
    }
    return SIM_RUN_EXTERNAL;
}

sim_run_result sim_simulation_resume(sim_simulation *s,uint32_t budget)
{
    uint32_t delta;
    if(!s)return SIM_RUN_FAULT;
    delta=s->clock_ticks-s->clock_last;s->clock_last=s->clock_ticks;
    w32(s,0x46c,s->clock_ticks);
    w32(s,0x220e0+0x1a,m32(s,0x220e0+0x1a)+delta);
    /* Starting a coroutine uses budget0 to prepare its state without
     * executing it. Keep this path in the small wrapper: RV32 far-branch
     * expansion in the giant executor can clobber a shrink-wrapped RA. */
    if(!budget)return SIM_RUN_BUDGET;
    return sim_lifted_run(s,budget);
}

sim_run_result sim_simulation_call(sim_simulation *s,uint32_t address,
                                  int near_body,uint32_t budget)
{
    uint16_t cs;
    if(!s||address>=SIM_DOS_MEMORY_SIZE)return SIM_RUN_FAULT;
    if(address<0x165a0u)cs=0x106c;
    else if(address<0x1bf70u)cs=0x165a;
    else if(address<0x35ea0u)cs=0x35e2;
    else if(address<0x35ed0u)cs=0x35ea;
    else cs=0x35ed;
    memset(s->r,0,sizeof(s->r));s->r[4]=0xff00;
    s->segment[1]=cs;s->segment[2]=s->segment[3]=0x4d2c;s->segment[0]=0x4d2c;
    s->flags=2;s->pc=address;s->fault_pc=0;s->running=1;
    if(near_body){
        /* A near-only entry returns to a small synthetic far-return bridge
         * in its own CS. The translated executor recognizes that address. */
        push(s,0xffff);push(s,0);push(s,0xfff0);
    }else{push(s,0xffff);push(s,0);}
    return sim_simulation_resume(s,budget);
}
