#include "scenario.h"
#include "resource_lzss.h"
#include <string.h>

const char *sim_scenario_filename(unsigned id)
{
    static const char *names[8]={"dullsvil.psn","sanfran.psn","hamburg.psn","bern.psn",
        "tokyo.psn","detroit.psn","boston.psn","rio.psn"};
    return id>=1 && id<=8 ? names[id-1] : NULL;
}

int sim_scenario_archive_find(const uint8_t *archive, size_t size,
                              unsigned id, const uint8_t **source, size_t *source_size)
{
    size_t position=12, found_offset=0, found_size=0;
    unsigned index;
    if (source) *source=NULL;
    if (source_size) *source_size=0;
    if (!archive || size<12 || !source || !source_size || id<1 || id>8 ||
        memcmp(archive,"SCPS0001\x08\0\0\0",12)) return 0;
    for (index=1;index<=8;++index) {
        uint32_t length;
        if (position>size || size-position<4) return 0;
        length=(uint32_t)archive[position] | ((uint32_t)archive[position+1]<<8) |
               ((uint32_t)archive[position+2]<<16) | ((uint32_t)archive[position+3]<<24);
        position+=4;
        if (!length || length>size-position) return 0;
        if (index==id) { found_offset=position;found_size=length; }
        position+=length;
    }
    if (position!=size) return 0;
    *source=archive+found_offset;*source_size=found_size;
    return 1;
}

int sim_scenario_decode(const uint8_t *source, size_t size, uint8_t *payload, size_t capacity)
{
    sim_lzss state;
    if (!source || !size || !payload || capacity<SIM_CITY_SAVE_BYTES) return 0;
    sim_lzss_init(&state,source,size);
    if (sim_lzss_read(&state,payload,SIM_CITY_SAVE_BYTES)!=SIM_CITY_SAVE_BYTES ||
        sim_lzss_malformed(&state)) return 0;
    return sim_city_save_valid(payload,SIM_CITY_SAVE_BYTES);
}
