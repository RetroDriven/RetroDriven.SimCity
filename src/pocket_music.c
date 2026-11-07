#include "pocket_music.h"
#ifndef OF_PC
#ifdef SIM_MUSIC_TEST
#include "pocket_music_test_sdk.h"
#else
#include "of.h"
#include "of_smp_voice.h"
#include "of_smp_bank.h"
#endif
#include "dos/xmidi.h"
#include <string.h>

static struct {
    sim_xmidi player;
    uint8_t program[16],volume[16],expression[16],brightness[16],resonance[16];
    uint32_t last_us,envelope_us;
    uint64_t sequence_fraction;
    int playing,initialized;
} music;

static void channel_event(void *user,uint64_t tick,uint8_t status,uint8_t a,uint8_t b)
{
    unsigned channel=status&15,kind=status>>4;
    (void)user;(void)tick;
    switch(kind) {
    case 8:smp_voice_note_off((int)channel,a);break;
    case 9:
        if (!b) smp_voice_note_off((int)channel,a);
        else {
            const ofsf_zone_t *zones[4];
            int count=of_smp_zone_lookup(channel==9?128:0,music.program[channel],a,b,zones,4);
            const void *samples=of_smp_bank_sample_base();
            for(int i=0;i<count;++i)smp_voice_note_on(zones[i],(int)channel,a,b,samples);
        }
        break;
    case 11:
        switch(a) {
        case 1:smp_voice_update_mod((int)channel,b);break;
        case 7:music.volume[channel]=b;smp_voice_update_volume((int)channel,b,music.expression[channel]);break;
        case 10:smp_voice_update_pan((int)channel,b);break;
        case 11:music.expression[channel]=b;smp_voice_update_volume((int)channel,music.volume[channel],b);break;
        case 64:smp_voice_update_sustain((int)channel,b>=64);break;
        case 71:music.resonance[channel]=b;smp_voice_update_filter((int)channel,music.brightness[channel],b);break;
        case 74:music.brightness[channel]=b;smp_voice_update_filter((int)channel,b,music.resonance[channel]);break;
        case 91:smp_voice_update_reverb_send((int)channel,b);break;
        case 93:smp_voice_update_chorus_send((int)channel,b);break;
        case 120:case 123:smp_voice_all_off((int)channel);break;
        case 121:
            music.expression[channel]=127;music.brightness[channel]=music.resonance[channel]=64;
            smp_voice_update_sustain((int)channel,0);smp_voice_update_mod((int)channel,0);
            smp_voice_update_volume((int)channel,music.volume[channel],127);
            smp_voice_update_filter((int)channel,64,64);smp_voice_update_bend((int)channel,0);
            break;
        default:break;
        }
        break;
    case 12:music.program[channel]=a;break;
    case 14:smp_voice_update_bend((int)channel,((int)b*128+a)-8192);break;
    default:break;
    }
}

static void pump(void)
{
    uint32_t now,elapsed,start,ticks;
    if(!music.playing)return;
    /* The timer service is safe inside the ISR; of_time_us uses an ecall. */
    now=OF_SVC->timer_get_us();elapsed=now-music.last_us;music.last_us=now;
    music.sequence_fraction+=(uint64_t)elapsed*120;
    ticks=(uint32_t)(music.sequence_fraction/1000000u);
    music.sequence_fraction%=1000000u;
    if(sim_xmidi_advance_ticks120(&music.player,ticks,channel_event,NULL)!=SIM_XMIDI_OK || music.player.done) {
        pocket_music_stop();return;
    }
    music.envelope_us+=elapsed;start=OF_SVC->timer_get_us();
    while(music.envelope_us>=1000 && OF_SVC->timer_get_us()-start<2000) {
        smp_voice_tick();music.envelope_us-=1000;
    }
}

void pocket_music_stop(void)
{
    if(!music.initialized)return;
    of_timer_stop();music.playing=0;
    sim_xmidi_stop(&music.player,channel_event,NULL);
    smp_voice_all_off_global();
}

int pocket_music_start(const uint8_t *xmi,size_t size)
{
    pocket_music_stop();
    if(!xmi || of_smp_bank_bind_preloaded()!=1)return 0;
    if(!music.initialized){smp_voice_init();music.initialized=1;}
    if(sim_xmidi_open(&music.player,xmi,size)!=SIM_XMIDI_OK)return 0;
    memset(music.program,0,sizeof(music.program));
    for(int channel=0;channel<16;++channel) {
        music.volume[channel]=100;music.expression[channel]=127;
        music.brightness[channel]=music.resonance[channel]=64;
        smp_voice_update_volume(channel,100,127);smp_voice_update_pan(channel,64);
        smp_voice_update_bend(channel,0);smp_voice_update_sustain(channel,0);
    }
    smp_voice_set_master_volume(128);
    music.last_us=of_time_us();music.envelope_us=0;music.sequence_fraction=0;
    if(sim_xmidi_advance_ticks120(&music.player,0,channel_event,NULL)!=SIM_XMIDI_OK || music.player.done) {
        pocket_music_stop();return 0;
    }
    music.playing=1;of_timer_set_callback(pump,50);return 1;
}
int pocket_music_playing(void){return music.playing;}
#else
/* The pinned SDK's desktop sample voice engine has no audio device backend. */
int pocket_music_start(const uint8_t *xmi,size_t size){(void)xmi;(void)size;return 0;}
void pocket_music_stop(void){}
int pocket_music_playing(void){return 0;}
#endif
