#ifndef SIMCITY_DOS_CITY_SAVE_H
#define SIMCITY_DOS_CITY_SAVE_H
#include <stddef.h>
#include <stdint.h>

#define SIM_CITY_SAVE_BYTES 27248u
#define SIM_CITY_MEMORY_BYTES 1048576u
#define SIM_CITY_DGROUP 0x4d2c0u
#define SIM_CITY_SAVE_MAP_OFFSET 3248u

/* DOS2.00 CITYMCRP serialization over an original-address flat state image.
 * External buffers must not overlap the memory image. No filesystem calls.
 * The complete header/history/misc payload is retained, including unknowns.
 */
int sim_city_save_encode(const uint8_t *memory, size_t memory_size,
                          uint8_t *out, size_t out_size);
int sim_city_save_load_blocks(uint8_t *memory, size_t memory_size,
                               const uint8_t *source, size_t source_size);
/* Original load order: load_blocks -> simulation init0x1254a -> restore_globals.
 * scenario_id0 means an ordinary city;1..8 selects a scenario.
 */
int sim_city_save_restore_globals(uint8_t *memory, size_t memory_size,
                                  unsigned scenario_id);
int sim_city_save_valid(const uint8_t *source, size_t source_size);
#endif
