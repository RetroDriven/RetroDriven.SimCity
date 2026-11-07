#ifndef SIM_POCKET_MUSIC_H
#define SIM_POCKET_MUSIC_H
#include <stddef.h>
#include <stdint.h>
/* Original XMIDI event stream rendered by the SDK's sample-bank synthesizer.
 * The caller retains the original XMI buffer while playback is active. */
int pocket_music_start(const uint8_t *xmi,size_t size);
void pocket_music_stop(void);
int pocket_music_playing(void);
#endif
