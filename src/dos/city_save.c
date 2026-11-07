#include "city_save.h"
#include <string.h>

typedef struct city_block { uint32_t address; uint16_t size; } city_block;
static const city_block blocks[9] = {
    {0x4f76e,128}, {0x3ed9a,480}, {0x4c4be,480}, {0x4d0da,480},
    {0x3fb32,480}, {0x4ca5e,480}, {0x4c87e,480}, {0x4c69e,240}, {0x53e38,24000}
};

static uint16_t get16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static void put16(uint8_t *p, uint16_t value) { p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8); }
static uint32_t get32(const uint8_t *p) { return get16(p) | ((uint32_t)get16(p+2)<<16); }
static void put32(uint8_t *p, uint32_t value) { put16(p,(uint16_t)value);put16(p+2,(uint16_t)(value>>16)); }
static void be16(uint8_t *p, uint16_t value) { p[0]=(uint8_t)(value>>8);p[1]=(uint8_t)value; }

int sim_city_save_valid(const uint8_t *source, size_t size)
{
    /* Extra scenario padding is ignored by the original loader. Checking the
     * supported file signature is a port guard, not original DOS validation. */
    return source && size >= SIM_CITY_SAVE_BYTES && !memcmp(source+65,"CITYMCRP",8);
}

int sim_city_save_encode(const uint8_t *memory, size_t memory_size,
                          uint8_t *out, size_t out_size)
{
    unsigned b, i, position=0;
    const uint8_t *d;
    uint8_t *misc;
    if (!memory || memory_size<SIM_CITY_MEMORY_BYTES || !out || out_size<SIM_CITY_SAVE_BYTES) return 0;
    d=memory+SIM_CITY_DGROUP;
    for (b=0;b<9;++b) {
        const uint8_t *source=memory+blocks[b].address;
        if (!b) memcpy(out+position,source,blocks[b].size);
        else for (i=0;i<blocks[b].size;i+=2) {
            out[position+i]=source[i+1];out[position+i+1]=source[i];
        }
        position+=blocks[b].size;
    }
    /* DOS writer0x2b592 prepares name and aliases before serializing. */
    for (i=0;i<20 && d[0xcaca+i];++i) out[2+i]=d[0xcaca+i];
    for (;i<20;++i) out[2+i]=0;
    misc=out+3008;
    be16(misc+100,get16(d+0x2962));be16(misc+102,get16(d+0x2960));
    be16(misc+16,get16(d+0x5f72));be16(misc+18,get16(d+0x5f70));
    for (i=0;i<4;++i) be16(misc+104+i*2,(uint16_t)(int16_t)(int8_t)d[0x295b+i]);
    be16(misc+112,get16(d+0xd192));be16(misc+114,get16(d+0x3a6e));
    for (i=0;i<3;++i) {
        static const unsigned percent[3]={0x36c8,0x36b4,0x36be};
        int value=(int16_t)get16(d+percent[i]);
        int32_t fraction=((int32_t)value*65536)/100;
        be16(misc+116+i*4,(uint16_t)(value==100));
        be16(misc+118+i*4,(uint16_t)fraction);
    }
    return 1;
}

int sim_city_save_load_blocks(uint8_t *memory, size_t memory_size,
                               const uint8_t *source, size_t source_size)
{
    unsigned b,i,position=0;
    if (!memory || memory_size<SIM_CITY_MEMORY_BYTES || !sim_city_save_valid(source,source_size)) return 0;
    for (b=0;b<9;++b) {
        uint8_t *destination=memory+blocks[b].address;
        /* Original2b25e reads the header into its local stack buffer, not
         * the active process header at24ae. Only the bounded name is copied. */
        if (b) for (i=0;i<blocks[b].size;i+=2) {
            destination[i]=source[position+i+1];destination[i+1]=source[position+i];
        }
        position+=blocks[b].size;
    }
    /* Original strncpy20 pads through byte19 and leaves byte20 unchanged. */
    for (i=0;i<20 && source[2+i];++i) memory[SIM_CITY_DGROUP+0xcaca+i]=source[2+i];
    for (;i<20;++i) memory[SIM_CITY_DGROUP+0xcaca+i]=0;
    return 1;
}

int sim_city_save_restore_globals(uint8_t *memory, size_t memory_size, unsigned scenario)
{
    uint8_t *d,*misc;
    int value;
    unsigned i;
    if (!memory || memory_size<SIM_CITY_MEMORY_BYTES || scenario>8) return 0;
    d=memory+SIM_CITY_DGROUP;misc=memory+0x4c69e;
    put16(d+0x5f0e,0);put16(d+0x5f10,0);
    put16(d+0x2960,get16(misc+102));put16(d+0x2962,get16(misc+100));
    put16(d+0x5f70,get16(misc+18));put16(d+0x5f72,get16(misc+16));
    put32(d+0xd168,get32(d+0x50ba));put32(d+0x532a,get32(d+0x4f7a));
    for (i=0;i<4;++i) d[0x295b+i]=(uint8_t)get16(misc+104+i*2);
    put16(d+0xd192,get16(misc+112));put16(d+0x3a6e,get16(misc+114));
    for (i=0;i<3;++i) {
        static const unsigned percent[3]={0x36c8,0x36b4,0x36be};
        uint32_t bits=((uint32_t)get16(misc+116+i*4)<<16)|get16(misc+118+i*4);
        int32_t fraction;memcpy(&fraction,&bits,sizeof(fraction));
        value=(int16_t)(fraction/655);
        if (value<0) value=0;
        if (value>100) value=100;
        put16(d+percent[i],(uint16_t)value);
    }
    if ((int16_t)get16(d+0x5f72)<0) { put16(d+0x5f70,0);put16(d+0x5f72,0); }
    value=(int16_t)get16(d+0xd192);if(value<0||value>20)put16(d+0xd192,7);
    value=(int16_t)get16(d+0x3a6e);if(value<0||value>3)put16(d+0x3a6e,1);
    put16(d+0xcab8,(uint16_t)scenario);
    return 1;
}
