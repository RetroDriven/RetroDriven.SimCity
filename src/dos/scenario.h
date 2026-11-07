#ifndef SIMCITY_DOS_SCENARIO_H
#define SIMCITY_DOS_SCENARIO_H
#include <stddef.h>
#include <stdint.h>
#include "city_save.h"

const char *sim_scenario_filename(unsigned scenario_id);
/* Read-only port asset container SCPS0001: count8, then LE32 length + original
 * compressed PSN bytes per scenario in the original MESSAGE.PTF order. */
int sim_scenario_archive_find(const uint8_t *archive, size_t archive_size,
                              unsigned scenario_id, const uint8_t **source,
                              size_t *source_size);
/* Decode the original city payload, ignoring its16 trailing padding bytes. */
int sim_scenario_decode(const uint8_t *source, size_t source_size,
                         uint8_t *city_payload, size_t payload_size);
#endif
