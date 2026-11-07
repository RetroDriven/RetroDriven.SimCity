#ifndef SIM_DOS_AUDIO_BANK_H
#define SIM_DOS_AUDIO_BANK_H
#include <stdint.h>
#include <stddef.h>
enum sim_sound_id {SIM_SOUND_COPTER,SIM_SOUND_EXPLODE,SIM_SOUND_MONSTER,
    SIM_SOUND_POLICE,SIM_SOUND_TRAIN,SIM_SOUND_BULLDOZE,SIM_SOUND_BUILD,SIM_SOUND_HONK};
typedef struct sim_pcm_sound { const uint8_t *pcm16; uint32_t count,rate; } sim_pcm_sound;
/* Caller-owned port container of original VOC samples converted losslessly
 * from unsigned8 PCM to signed16. No allocation or sound device access. */
int sim_audio_sound(const uint8_t *bank,size_t size,unsigned id,sim_pcm_sound *out);
int sim_audio_music(const uint8_t *bank,size_t size,const uint8_t **xmi,size_t *length);
#endif
