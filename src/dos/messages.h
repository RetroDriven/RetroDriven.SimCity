#ifndef SIMCITY_DOS_MESSAGES_H
#define SIMCITY_DOS_MESSAGES_H
#include <stddef.h>
#include <stdint.h>
/* Original MESSAGE.PTF table installation using reserved original-address heap
 * 0x70000..0x8ffff. Caller retains neither pointers nor input buffer afterward.
 * Decoded input must not overlap the simulation image. Validation is atomic.
 */
int sim_messages_install(uint8_t *memory,size_t memory_size,
                          const uint8_t *decoded,size_t decoded_size);
#endif
