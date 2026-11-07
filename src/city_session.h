#ifndef SIMCITY_CITY_SESSION_H
#define SIMCITY_CITY_SESSION_H
#include "dos/simulation.h"
#include "dos/message_dispatch.h"
#include "dos/city_save.h"

/* The original simulation owns the city. Platform commands are admitted only
 * at its cooperative scheduler boundary, never half way through a map update. */
typedef struct {
    sim_simulation cpu;
    const uint8_t *exe,*messages,*icon;
    size_t exe_size,message_size;
    int active,safe,failed,budget_open,budget_annual,modal;
    int modal_image,modal_count,modal_queue[16],modal_images[16];
    uint32_t stopped_pc;
    SimMessageEffects effects;
    unsigned effects_serial;
    int (*sound)(void *,unsigned);
    void *sound_user;
    SimMessageEditorOpen editor_open;
    void *editor_user;
    void (*progress)(void *,const char *);
    void *progress_user;
} city_session;
void city_session_assets(city_session *,const uint8_t *,size_t,const uint8_t *,size_t,const uint8_t *);
int city_session_new(city_session *,const uint16_t *,const char *,unsigned,uint32_t,uint32_t);
int city_session_load(city_session *,const uint8_t *,size_t,unsigned,uint32_t);
void city_session_step(city_session *,uint32_t,unsigned);
uint8_t *city_session_dgroup(city_session *);
uint16_t city_session_place(city_session *,int,int,unsigned,int,int);
int city_session_save(city_session *,uint8_t *,size_t);
int city_session_budget(city_session *,int);
void city_session_budget_commit(city_session *);
int city_session_disaster(city_session *,unsigned);
int city_session_animate(city_session *);
void city_session_acknowledge(city_session *);
#endif
