#include "city_session.h"
#include "dos/messages.h"
#include "dos/simulation_budget.h"
#include "dos/placement.h"
#include "dos/maps.h"
#include "dos/windows.h"
#include <string.h>

static unsigned word(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
static void put(uint8_t *p,unsigned n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void progress(city_session *s,const char *stage){if(s->progress)s->progress(s->progress_user,stage);}
static uint32_t elapsed(city_session *s){const uint8_t *p=s->cpu.memory+0x220fa;return word(p)|((uint32_t)word(p+2)<<16);}
uint8_t *city_session_dgroup(city_session *s){return sim_simulation_dgroup(&s->cpu);}
void city_session_assets(city_session *s,const uint8_t *exe,size_t n,const uint8_t *messages,size_t m,const uint8_t *icon)
{s->exe=exe;s->exe_size=n;s->messages=messages;s->message_size=m;s->icon=icon;}

static int bootstrap(city_session *s,uint32_t clock)
{
    uint8_t *d;int restore=s->active;
    s->active=s->failed=s->budget_open=s->modal=s->modal_count=0;s->safe=1;s->effects_serial=0;s->modal_image=-1;
    memset(&s->effects,0,sizeof(s->effects));
    /* DOS keeps the process alive between cities. In particular1254a reads
     * existing histories and RNG state, and window geometry survives. */
    if(restore){s->cpu.clock_ticks=clock;return 1;}
    progress(s,"City: initialize MZ memory");
    if(!sim_simulation_init(&s->cpu,s->exe,s->exe_size))return 0;
    progress(s,"City: install message tables");
    if(!sim_messages_install(s->cpu.memory,SIM_DOS_MEMORY_SIZE,s->messages,s->message_size))return 0;
    progress(s,"City: configure MCGA");
    if(!sim_messages_configure_mcga(s->cpu.memory,SIM_DOS_MEMORY_SIZE))return 0;
    progress(s,"City: bind icon");
    if(!sim_messages_bind_icon(s->cpu.memory,SIM_DOS_MEMORY_SIZE,s->icon,68))return 0;
    s->cpu.clock_ticks=s->cpu.clock_last=clock;
    d=city_session_dgroup(s);d[0x295b]=1;d[0x295e]=1;d[0x2969]=1;d[0x5326]=1;d[0x3468]=1;
    if(!sim_maps_configure_mcga(d,65536))return 0;
    d[0x2959]=d[0x2958]=1;put(d+0x1562,1);put(d+0x2b1c,8);put(d+0x2b1e,8);
    put(d+0x2b14,1);put(d+0x2b16,1);put(d+0x2b18,1);
    put(d+0x296e,31);put(d+0x296c,18);put(d+0x5f10,0);put(d+0x5f0e,0);
    return 1;
}
static int initialize(city_session *s)
{
    progress(s,"City: original initializer1254a");
    sim_run_result r=sim_simulation_call(&s->cpu,0x1254a,0,5000000);
    progress(s,"City: original initializer returned");
    if(r!=SIM_RUN_RETURNED){s->failed=1;s->stopped_pc=s->cpu.pc;return 0;}return 1;
}
static int start(city_session *s,unsigned flag)
{
    uint8_t *d=city_session_dgroup(s);unsigned i;
    progress(s,"City: reset windows");
    /* Original23bec resets visibility and caches, retaining window geometry
     * and the live process state. Maps opens in front of the City window. */
    if(d[0x296a])sim_windows_close(s->cpu.memory,SIM_DOS_MEMORY_SIZE,2);
    if(word(d+0x3270)||word(d+0x3272)){
        put(d+0x3270,0);put(d+0x3272,0);put(d+0x526e,0);put(d+0x5270,0);
    }
    for(i=1;i<=3;++i)if(d[0x2968+i])sim_windows_close(s->cpu.memory,SIM_DOS_MEMORY_SIZE,i);
    sim_windows_activate(s->cpu.memory,SIM_DOS_MEMORY_SIZE,0);
    memset(s->cpu.memory+0x43440,255,24000);
    memset(s->cpu.memory+0x41770,255,3200);
    sim_windows_activate(s->cpu.memory,SIM_DOS_MEMORY_SIZE,1);
    progress(s,"City: repaint Editor");
    if(s->editor_open)s->editor_open(s->cpu.memory,SIM_DOS_MEMORY_SIZE,elapsed(s),s->editor_user);
    sim_windows_activate(s->cpu.memory,SIM_DOS_MEMORY_SIZE,2);
    put(d+0x4034,flag);
    progress(s,"City: start simulation task");
    if(sim_simulation_call(&s->cpu,0x13048,0,0)!=SIM_RUN_BUDGET)return 0;
    progress(s,"City: task ready");
    s->active=1;s->safe=0;return 1;
}
static void notification(city_session *s,const SimMessageEffects *e)
{
    if(!e->modal_notification)return;
    if(!s->modal){s->modal=e->modal_notification;s->modal_image=e->modal_image_line;}
    else if(s->modal_count<16){s->modal_queue[s->modal_count]=e->modal_notification;s->modal_images[s->modal_count++]=e->modal_image_line;}
    else {s->failed=1;s->stopped_pc=s->cpu.pc;}
}
void city_session_acknowledge(city_session *s)
{
    s->modal=0;s->modal_image=-1;
    if(s->modal_count){
        s->modal=s->modal_queue[0];s->modal_image=s->modal_images[0];--s->modal_count;
        memmove(s->modal_queue,s->modal_queue+1,(size_t)s->modal_count*sizeof(s->modal_queue[0]));
        memmove(s->modal_images,s->modal_images+1,(size_t)s->modal_count*sizeof(s->modal_images[0]));
    }
}
int city_session_new(city_session *s,const uint16_t *map,const char *name,unsigned difficulty,uint32_t funds,uint32_t clock)
{
    uint8_t *d,previous_name[21]={0};unsigned i;
    if(s->active)memcpy(previous_name,city_session_dgroup(s)+0xcaca,sizeof(previous_name));
    if(!map||!name||!bootstrap(s,clock))return 0;
    d=city_session_dgroup(s);
    /* Original35668 defaults and106ac precede1254a. The wrapper resets
     * city time after terrain generation and disables automatic budgeting. */
    put(d+0x2960,funds);put(d+0x2962,funds>>16);put(d+0xc9fe,difficulty);
    put(d+0x36c8,100);put(d+0x36be,100);put(d+0x36b4,100);put(d+0xd192,7);
    put(d+0x5f70,50);put(d+0x5f72,0);
    if(!initialize(s))return 0;
    for(i=0;i<12000;++i)put(d+0x6b78+2*i,map[i]);
    /* Original26dac uses strcpy for its default and accepted name. Retain
     * unused bytes after NUL too: the original writer saves20 raw bytes. */
    memcpy(d+0xcaca,previous_name,sizeof(previous_name));memcpy(d+0xcaca,"HERESVILLE",11);
    for(i=0;i<20&&name[i];++i)d[0xcaca+i]=(uint8_t)name[i];
    d[0xcaca+i]=0;
    put(d+0x2960,funds);put(d+0x2962,funds>>16);put(d+0xc9fe,difficulty);
    put(d+0x5f70,0);put(d+0x5f72,0);put(d+0xd192,7);
    put(d+0x36c8,100);put(d+0x36be,100);put(d+0x36b4,100);
    d[0x295c]=0;
    return start(s,4);
}
int city_session_load(city_session *s,const uint8_t *save,size_t size,unsigned scenario,uint32_t clock)
{
    if(!sim_city_save_valid(save,size)||!bootstrap(s,clock)||
       !sim_city_save_load_blocks(s->cpu.memory,SIM_DOS_MEMORY_SIZE,save,size)||!initialize(s)||
       !sim_city_save_restore_globals(s->cpu.memory,SIM_DOS_MEMORY_SIZE,scenario))return 0;
    return start(s,8);
}
static int external(city_session *s)
{
    sim_simulation *c=&s->cpu;SimMessageEffects e;int ok;
    if(c->pc==0x33c24){
        uint8_t *d=city_session_dgroup(s);
        uint32_t a=(uint32_t)c->segment[2]*16u+(uint16_t)(c->r[4]+4);
        unsigned id=word(c->memory+a);
        if(id<8&&d[0x3468]&&d[0x295e]&&s->sound&&s->sound(s->sound_user,id))put(d+0x346c,id);
        sim_simulation_return_far(c,0);return 1;
    }
    if(c->pc==0x30a26){
        uint32_t a=(uint32_t)c->segment[2]*16u+(uint16_t)(c->r[4]+4);
        ok=sim_message_dispatch_with_editor(c->memory,SIM_DOS_MEMORY_SIZE,(int16_t)word(c->memory+a),elapsed(s),&e,s->editor_open,s->editor_user);
    }else if(c->pc==0x305fc)ok=sim_message_flush_with_editor(c->memory,SIM_DOS_MEMORY_SIZE,elapsed(s),&e,s->editor_open,s->editor_user);
    else return 0;
    if(!ok)return 0;
    s->effects=e;++s->effects_serial;notification(s,&e);
    sim_simulation_return_far(c,0);return 1;
}
int city_session_budget(city_session *s,int annual)
{
    sim_budget_result r;
    if(!s->active||!s->safe||s->failed)return 0;
    r=sim_budget_prepare(s->cpu.memory,SIM_DOS_MEMORY_SIZE,annual);
    if(r==SIM_BUDGET_INVALID)return 0;
    if(r==SIM_BUDGET_AUTOMATIC)return sim_budget_commit(s->cpu.memory,SIM_DOS_MEMORY_SIZE,annual);
    s->budget_open=1;s->budget_annual=annual;return 1;
}
void city_session_budget_commit(city_session *s)
{if(s->budget_open){sim_budget_commit(s->cpu.memory,SIM_DOS_MEMORY_SIZE,s->budget_annual);s->budget_open=0;}}
void city_session_step(city_session *s,uint32_t clock,unsigned budget)
{
    sim_run_result r;
    if(!s->active||s->failed||s->budget_open||s->modal)return;
    s->cpu.clock_ticks=clock;s->safe=0;
    {
        r=sim_simulation_resume(&s->cpu,budget);
        if(r==SIM_RUN_BUDGET)return;
        if(r==SIM_RUN_EXTERNAL&&s->cpu.pc==0x35c9a){
            sim_simulation_return_far(&s->cpu,0);s->safe=1;
            if(sim_message_pump_with_editor(s->cpu.memory,SIM_DOS_MEMORY_SIZE,elapsed(s),&s->effects,s->editor_open,s->editor_user)){
                ++s->effects_serial;
                notification(s,&s->effects);
            }
            if(word(city_session_dgroup(s)+0x4f86))city_session_budget(s,1);
            return;
        }
        if(r!=SIM_RUN_EXTERNAL||!external(s)){s->failed=1;s->stopped_pc=s->cpu.pc;return;}
        /* Let the frontend apply each raster effect before another dispatch
         * can clear or scroll the same message strip. */
        return;
    }
}
uint16_t city_session_place(city_session *s,int x,int y,unsigned tool,int vx,int vy)
{
    uint8_t *d=city_session_dgroup(s);
    if(!s->active||!s->safe||s->failed||s->budget_open||s->modal)return 0;
    put(d+0x5f10,(unsigned)vx);put(d+0x5f0e,(unsigned)vy);d[0x2b1b]=tool==15;
    return sim_city_place_dgroup(d,65536,x,y,tool);
}
int city_session_save(city_session *s,uint8_t *out,size_t size)
{return s->active&&s->safe&&!s->failed&&sim_city_save_encode(s->cpu.memory,SIM_DOS_MEMORY_SIZE,out,size);}
int city_session_disaster(city_session *s,unsigned id)
{
    uint8_t *d=city_session_dgroup(s);
    if(!s->safe||!s->active||s->failed||id<20||id>25)return 0;
    /* Original menu only posts a request; the simulation executes it on its
     * own coroutine and can yield normally during long disaster routines. */
    put(d+0x4fd8,2u<<(id-20));d[0x3a64]=1;return 1;
}
int city_session_animate(city_session *s)
{
    sim_cpu_context saved;sim_run_result r;unsigned n=0;
    if(!s->safe||!s->active||s->failed||s->budget_open||s->modal)return 0;
    sim_simulation_cpu_save(&s->cpu,&saved);
    sim_simulation_begin_aux(&s->cpu,0x35ed2,0x35ed,0xe000,NULL,0);
    do{
        r=sim_simulation_resume(&s->cpu,1000000);
        if(r==SIM_RUN_RETURNED)break;
        if(r==SIM_RUN_EXTERNAL&&!external(s))break;
    }while((r==SIM_RUN_BUDGET||r==SIM_RUN_EXTERNAL)&&++n<100);
    if(r!=SIM_RUN_RETURNED){s->failed=1;s->stopped_pc=s->cpu.pc;}
    sim_simulation_cpu_restore(&s->cpu,&saved);
    if(!s->failed){uint8_t *d=city_session_dgroup(s);if(word(d+0x3a6e))put(d+0x1c28,word(d+0x1c28)+1);}
    return !s->failed;
}
