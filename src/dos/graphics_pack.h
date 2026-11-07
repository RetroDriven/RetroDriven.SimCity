#ifndef SIMCITY_DOS_GRAPHICS_PACK_H
#define SIMCITY_DOS_GRAPHICS_PACK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Port-side parser for the standard decoded DOS 2.00 MCGA layout, including
 * old themed packs normalized by graphics_theme.c.
 * The caller owns the decoded bytes and must keep them unchanged and alive.
 * No allocation, decompression, VGA access, or asset copying occurs here.
 * Fixed-section sizes agree with DOS loader 0x1f0dc. The image-record layout
 * is structurally validated, but its game roles are not assumed by this API.
 */
#define SIM_GRAPHICS_TILE_COUNT 960u
#define SIM_GRAPHICS_FONT_COUNT 128u
#define SIM_GRAPHICS_RECORD_COUNT 16u
#define SIM_GRAPHICS_CELL_BYTES 64u

typedef enum sim_graphics_status {
    SIM_GRAPHICS_OK = 0,
    SIM_GRAPHICS_BAD_ARGUMENT,
    SIM_GRAPHICS_TRUNCATED,
    SIM_GRAPHICS_BAD_PALETTE,
    SIM_GRAPHICS_BAD_MASK,
    SIM_GRAPHICS_BAD_LOOKUP,
    SIM_GRAPHICS_BAD_RECORD,
    SIM_GRAPHICS_BAD_FRAME,
    SIM_GRAPHICS_TRAILING_DATA,
    SIM_GRAPHICS_OUT_OF_RANGE
} sim_graphics_status;

typedef enum sim_graphics_section {
    SIM_GRAPHICS_PALETTE = 0, /* 256 RGB triples in VGA DAC range 0..63 */
    SIM_GRAPHICS_TILES,       /* 960 row-major 8x8 indexed images */
    SIM_GRAPHICS_FONT_MASKS,  /* 128 row-major 8x8 byte masks */
    SIM_GRAPHICS_MAP_COLORS   /* 960 observed per-tile color candidates */
} sim_graphics_section;

typedef struct sim_graphics_span {
    const uint8_t *data;
    size_t size;
} sim_graphics_span;

typedef struct sim_graphics_record {
    size_t offset;       /* Three-byte record header within the pack */
    size_t payload_size;
    size_t frame_count;
    uint8_t kind;        /* 8=indexed image; 1=bitwise byte mask (may be partial) */
} sim_graphics_record;

typedef struct sim_graphics_frame {
    uint16_t width;
    uint16_t height;
    uint8_t kind;
    sim_graphics_span pixels;
} sim_graphics_frame;

typedef struct sim_graphics_pack {
    const uint8_t *data;
    size_t size;
    sim_graphics_record records[SIM_GRAPHICS_RECORD_COUNT];
    uint32_t validated;
} sim_graphics_pack;

/* Invalid open leaves pack closed. Accessors clear their output on failure. */
sim_graphics_status sim_graphics_pack_open(sim_graphics_pack *pack,
                                          const uint8_t *data, size_t size);
sim_graphics_status sim_graphics_pack_section(const sim_graphics_pack *pack,
                                             sim_graphics_section section,
                                             sim_graphics_span *span);
sim_graphics_status sim_graphics_pack_tile(const sim_graphics_pack *pack,
                                          size_t index, sim_graphics_span *tile);
sim_graphics_status sim_graphics_pack_font_mask(const sim_graphics_pack *pack,
                                               size_t index, sim_graphics_span *mask);
sim_graphics_status sim_graphics_pack_record(const sim_graphics_pack *pack,
                                            size_t index, sim_graphics_record *record);
sim_graphics_status sim_graphics_pack_frame(const sim_graphics_pack *pack,
                                           size_t record_index, size_t frame_index,
                                           sim_graphics_frame *frame);
size_t sim_graphics_pack_sizeof(void);

#ifdef __cplusplus
}
#endif
#endif
