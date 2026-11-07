#ifndef SIMCITY_DOS_GRAPHICS_THEME_H
#define SIMCITY_DOS_GRAPHICS_THEME_H
#include <stddef.h>
#include <stdint.h>
#include "graphics_pack.h"

enum sim_theme_status { SIM_THEME_OK=0,SIM_THEME_BAD_ARGUMENT,SIM_THEME_BAD_HEADER,
                         SIM_THEME_BAD_DATA,SIM_THEME_NO_SPACE };
typedef struct sim_graphics_theme_info {
    char title[64],message_file[64],sound_file[64],fallback_graphics[64];
    size_t compressed_offset,decoded_size,normalized_size;
    uint16_t base_year,train_frames;
    uint8_t graphics_mode,extended_train;
} sim_graphics_theme_info;
/* Original23094 text preamble/header. Four strings follow DOS1A, with a
 * single graphics-mode byte between title NUL and message filename ('2'=MCGA).
 * Only old MCGA packs are accepted; standard MCGADAT uses existing APIs. */
int sim_graphics_theme_header(sim_graphics_theme_info *info,const uint8_t *raw,size_t size);
/* Decode the resource stream after its actual header. Caller supplies buffers;
 * old maximum decoded size for the shipped six MCGA themes is159400bytes. */
int sim_graphics_theme_unpack(sim_graphics_theme_info *info,const uint8_t *raw,size_t size,
                               uint8_t *decoded,size_t capacity,size_t *written);
/* Normalize original old PGF sections to existing graphics_pack layout.
 * Art/font/lookup bytes are copied exactly; original RGB8 palette shifts>>2.
 * No resizing/recoloring/padding occurs. Input/output buffers must be distinct.
 * Info preserves header metadata and receives year/train/count/size fields. */
int sim_graphics_theme_normalize(sim_graphics_theme_info *info,const uint8_t *decoded,
                                  size_t size,uint8_t *output,size_t capacity,size_t *written);

/* Stable archive produced by tools/prepare_themes.py: fixed little-endian
 * header/index with six normalized packs and original decoded message files. */
typedef struct sim_graphics_themes { const uint8_t *data;size_t size;unsigned count; } sim_graphics_themes;
typedef struct sim_graphics_theme_asset {
    char title[32],source_file[16],message_file[16],sound_file[16],fallback_graphics[16];
    uint16_t base_year,train_frames;
    sim_graphics_span graphics,messages;
} sim_graphics_theme_asset;
int sim_graphics_themes_open(sim_graphics_themes *archive,const uint8_t *data,size_t size);
int sim_graphics_themes_entry(const sim_graphics_themes *archive,unsigned index,sim_graphics_theme_asset *asset);
#endif
