#ifndef SIMCITY_DOS_XMIDI_H
#define SIMCITY_DOS_XMIDI_H
#include <stddef.h>
#include <stdint.h>

enum sim_xmidi_result {
    SIM_XMIDI_OK=0, SIM_XMIDI_BAD_FORMAT=-1, SIM_XMIDI_TRUNCATED=-2,
    SIM_XMIDI_UNSUPPORTED=-3, SIM_XMIDI_EVENT_LIMIT=-4
};
typedef void (*sim_xmidi_event_fn)(void *user,uint64_t tick120,
                                  uint8_t status,uint8_t a,uint8_t b);
typedef struct sim_xmidi_note {
    uint64_t deadline;
    uint8_t channel,note,active;
} sim_xmidi_note;
typedef struct sim_xmidi {
    const uint8_t *events;
    size_t size,position,loop_position[4];
    uint16_t loop_count[4];
    uint64_t tick;
    uint32_t wait;
    sim_xmidi_note notes[32];
    int started,done,error;
} sim_xmidi;

/* Caller owns input bytes until playback ends. Open accepts one-sequence IFF
 * XMIDI, or open_events accepts a raw EVNT payload for fixtures. */
int sim_xmidi_open(sim_xmidi *player,const uint8_t *xmi,size_t size);
int sim_xmidi_open_events(sim_xmidi *player,const uint8_t *events,size_t size);
/* Emit initial tick0 events on the first call, including advance_ticks120(0).
 * Then advance exactly the requested number of original120Hz service ticks.
 * Callbacks include explicit queued NoteOffs and source channel messages.
 * Private FOR116/NEXT117 are handled as four GLOBAL sequence slots; a fifth
 * FOR is ignored and NEXT<64 is ignored, matching shipped ADLIB.ADV.
 * Tempo metadata affects Miles beat counters, not120Hz event timing.
 */
int sim_xmidi_advance_ticks120(sim_xmidi *player,uint32_t ticks,
                              sim_xmidi_event_fn callback,void *user);
/* Explicit port stop: emit queued NoteOffs and mark playback done. */
void sim_xmidi_stop(sim_xmidi *player,sim_xmidi_event_fn callback,void *user);
#endif
