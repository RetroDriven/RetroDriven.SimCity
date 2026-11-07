#ifndef SIMCITY_DOS_RESOURCE_LZSS_H
#define SIMCITY_DOS_RESOURCE_LZSS_H
#include <stddef.h>
#include <stdint.h>

/* Portable reconstruction of DOS 2.00's resource decoder.
 * Original analysis addresses: init 0x20dd2; read 0x20e15.
 * The source buffer must remain valid for the lifetime of the decoder.
 * Each read may stop within a literal/backreference; all state is retained.
 */
typedef struct sim_lzss {
    uint8_t ring[4096];
    const uint8_t *source;
    size_t source_size, source_pos;
    unsigned write_pos, flags, flag_bits;
    unsigned match_pos, match_left;
    int malformed;
} sim_lzss;

void sim_lzss_init(sim_lzss *state, const uint8_t *source, size_t size);
size_t sim_lzss_read(sim_lzss *state, uint8_t *output, size_t requested);
size_t sim_lzss_state_size(void);
int sim_lzss_malformed(const sim_lzss *state);
#endif
