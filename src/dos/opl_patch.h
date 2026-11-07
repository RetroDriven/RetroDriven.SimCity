#ifndef SIMCITY_OPL_PATCH_H
#define SIMCITY_OPL_PATCH_H
#include <stddef.h>
#include <stdint.h>

/* Original Miles14/25-byte record, including its little-endian length word.
 * Stored parameters retain the driver's16-bit truncation, not floats. */
typedef struct sim_opl_operator {
    uint16_t multiplier,level;
    uint8_t characteristic,ksl,attack_decay,sustain_release,waveform;
} sim_opl_operator;
typedef struct sim_opl_patch {
    sim_opl_operator op[4];
    uint16_t feedback;
    uint8_t transpose,connection,volume_mask,extra_volume_mask,operators;
} sim_opl_patch;
typedef struct sim_opl_write { uint16_t reg;uint8_t value; } sim_opl_write;

/*0 failure,1 success; input/output buffers are caller-owned. */
int sim_opl_patch_decode(sim_opl_patch *out,const uint8_t *record,size_t size);
/* Original six-byte patch/bank/u32-offset index, terminated by patch255.
 * Returns record span including length; does not cache/upload/allocate voices. */
int sim_opl_library_find(const uint8_t *library,size_t size,unsigned patch,
    unsigned bank,const uint8_t **record,size_t *record_size);
/* Exact ADLIB.ADV1807 two-operator static programming order, dirty flagsF8.
 * hardware_channel0..8, effective gain0..127, modulation0..127.
 * Writes11 register/value pairs; excludes pitch/key-on and synthesis. */
int sim_opl_patch_program2(const sim_opl_patch *patch,unsigned hardware_channel,
    unsigned gain,unsigned modulation,sim_opl_write out[11]);
#endif
