#include "graphics_pack.h"

#include <string.h>

#define PALETTE_OFFSET 0u
#define PALETTE_BYTES 768u
#define TILES_OFFSET 0x300u
#define TILES_BYTES 0xf000u
#define FONT_OFFSET 0xf300u
#define FONT_BYTES 0x2000u
#define LOOKUP_OFFSET 0x11300u
#define RECORDS_OFFSET 0x116c3u
#define VALIDATED_TAG UINT32_C(0x53434750)

static uint16_t read_u16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

static int contains(size_t size, size_t offset, size_t count)
{
    return offset <= size && count <= size - offset;
}

static int valid_pack(const sim_graphics_pack *pack)
{
    return pack != NULL && pack->validated == VALIDATED_TAG && pack->data != NULL;
}

static sim_graphics_status section_span(const sim_graphics_pack *pack,
                                        size_t offset, size_t count,
                                        sim_graphics_span *span)
{
    if (span == NULL) return SIM_GRAPHICS_BAD_ARGUMENT;
    span->data = NULL;
    span->size = 0;
    if (!valid_pack(pack)) return SIM_GRAPHICS_BAD_ARGUMENT;
    if (!contains(pack->size, offset, count)) return SIM_GRAPHICS_TRUNCATED;
    span->data = pack->data + offset;
    span->size = count;
    return SIM_GRAPHICS_OK;
}

/* The subtraction/division checks avoid size_t wrap even on RV32. */
static sim_graphics_status frame_extent(const uint8_t *data, size_t end,
                                        size_t position, size_t *next,
                                        uint16_t *width, uint16_t *height)
{
    size_t pixels, available;
    if (!contains(end, position, 4)) return SIM_GRAPHICS_BAD_FRAME;
    *width = read_u16(data + position);
    *height = read_u16(data + position + 2);
    if (*width == 0 || *height == 0) return SIM_GRAPHICS_BAD_FRAME;
    available = end - position - 4;
    if ((size_t)*height > available / (size_t)*width) return SIM_GRAPHICS_BAD_FRAME;
    pixels = (size_t)*width * (size_t)*height;
    *next = position + 4 + pixels;
    return SIM_GRAPHICS_OK;
}

sim_graphics_status sim_graphics_pack_open(sim_graphics_pack *pack,
                                          const uint8_t *data, size_t size)
{
    sim_graphics_pack result;
    size_t position, index;
    if (pack == NULL) return SIM_GRAPHICS_BAD_ARGUMENT;
    memset(pack, 0, sizeof(*pack));
    if (data == NULL) return SIM_GRAPHICS_BAD_ARGUMENT;
    if (size < RECORDS_OFFSET) return SIM_GRAPHICS_TRUNCATED;
    for (index = 0; index < PALETTE_BYTES; ++index) {
        if (data[index] > 63) return SIM_GRAPHICS_BAD_PALETTE;
    }
    for (index = FONT_OFFSET; index < LOOKUP_OFFSET; ++index) {
        if (data[index] != 0 && data[index] != 255) return SIM_GRAPHICS_BAD_MASK;
    }
    if (data[LOOKUP_OFFSET] != 8 ||
        read_u16(data + LOOKUP_OFFSET + 1) != SIM_GRAPHICS_TILE_COUNT) {
        return SIM_GRAPHICS_BAD_LOOKUP;
    }
    memset(&result, 0, sizeof(result));
    result.data = data;
    result.size = size;
    position = RECORDS_OFFSET;
    for (index = 0; index < SIM_GRAPHICS_RECORD_COUNT; ++index) {
        sim_graphics_record *record = &result.records[index];
        size_t end;
        if (!contains(size, position, 3)) return SIM_GRAPHICS_TRUNCATED;
        record->offset = position;
        record->kind = data[position];
        record->payload_size = read_u16(data + position + 1);
        if ((record->kind != 1 && record->kind != 8) || record->payload_size < 4) {
            return SIM_GRAPHICS_BAD_RECORD;
        }
        position += 3;
        if (!contains(size, position, record->payload_size)) return SIM_GRAPHICS_TRUNCATED;
        end = position + record->payload_size;
        while (position < end) {
            uint16_t width, height;
            size_t next;
            sim_graphics_status status = frame_extent(data, end, position, &next, &width, &height);
            if (status != SIM_GRAPHICS_OK) return status;
            /* Image masks may contain partial bit values in original old
             * themed PGFs. The MCGA primitive combines every source byte
             * bitwise; only the fixed font section requires binary masks. */
            ++record->frame_count;
            position = next;
        }
    }
    if (position != size) return SIM_GRAPHICS_TRAILING_DATA;
    result.validated = VALIDATED_TAG;
    *pack = result;
    return SIM_GRAPHICS_OK;
}

sim_graphics_status sim_graphics_pack_section(const sim_graphics_pack *pack,
                                             sim_graphics_section section,
                                             sim_graphics_span *span)
{
    switch (section) {
    case SIM_GRAPHICS_PALETTE: return section_span(pack, PALETTE_OFFSET, PALETTE_BYTES, span);
    case SIM_GRAPHICS_TILES: return section_span(pack, TILES_OFFSET, TILES_BYTES, span);
    case SIM_GRAPHICS_FONT_MASKS: return section_span(pack, FONT_OFFSET, FONT_BYTES, span);
    case SIM_GRAPHICS_MAP_COLORS: return section_span(pack, LOOKUP_OFFSET + 3, SIM_GRAPHICS_TILE_COUNT, span);
    default:
        if (span != NULL) { span->data = NULL; span->size = 0; }
        return SIM_GRAPHICS_BAD_ARGUMENT;
    }
}

sim_graphics_status sim_graphics_pack_tile(const sim_graphics_pack *pack,
                                          size_t index, sim_graphics_span *tile)
{
    if (tile == NULL) return SIM_GRAPHICS_BAD_ARGUMENT;
    tile->data = NULL;
    tile->size = 0;
    if (!valid_pack(pack)) return SIM_GRAPHICS_BAD_ARGUMENT;
    if (index >= SIM_GRAPHICS_TILE_COUNT) return SIM_GRAPHICS_OUT_OF_RANGE;
    return section_span(pack, TILES_OFFSET + index * SIM_GRAPHICS_CELL_BYTES,
                        SIM_GRAPHICS_CELL_BYTES, tile);
}

sim_graphics_status sim_graphics_pack_font_mask(const sim_graphics_pack *pack,
                                               size_t index, sim_graphics_span *mask)
{
    if (mask == NULL) return SIM_GRAPHICS_BAD_ARGUMENT;
    mask->data = NULL;
    mask->size = 0;
    if (!valid_pack(pack)) return SIM_GRAPHICS_BAD_ARGUMENT;
    if (index >= SIM_GRAPHICS_FONT_COUNT) return SIM_GRAPHICS_OUT_OF_RANGE;
    return section_span(pack, FONT_OFFSET + index * SIM_GRAPHICS_CELL_BYTES,
                        SIM_GRAPHICS_CELL_BYTES, mask);
}

sim_graphics_status sim_graphics_pack_record(const sim_graphics_pack *pack,
                                            size_t index, sim_graphics_record *record)
{
    if (record == NULL) return SIM_GRAPHICS_BAD_ARGUMENT;
    memset(record, 0, sizeof(*record));
    if (!valid_pack(pack)) return SIM_GRAPHICS_BAD_ARGUMENT;
    if (index >= SIM_GRAPHICS_RECORD_COUNT) return SIM_GRAPHICS_OUT_OF_RANGE;
    *record = pack->records[index];
    if (!contains(pack->size, record->offset, 3) ||
        !contains(pack->size, record->offset + 3, record->payload_size)) {
        memset(record, 0, sizeof(*record));
        return SIM_GRAPHICS_TRUNCATED;
    }
    return SIM_GRAPHICS_OK;
}

sim_graphics_status sim_graphics_pack_frame(const sim_graphics_pack *pack,
                                           size_t record_index, size_t frame_index,
                                           sim_graphics_frame *frame)
{
    sim_graphics_record record;
    sim_graphics_status status;
    size_t position, end, index;
    if (frame == NULL) return SIM_GRAPHICS_BAD_ARGUMENT;
    memset(frame, 0, sizeof(*frame));
    status = sim_graphics_pack_record(pack, record_index, &record);
    if (status != SIM_GRAPHICS_OK) return status;
    if (frame_index >= record.frame_count) return SIM_GRAPHICS_OUT_OF_RANGE;
    position = record.offset + 3;
    end = position + record.payload_size;
    for (index = 0; index <= frame_index; ++index) {
        uint16_t width, height;
        size_t next;
        status = frame_extent(pack->data, end, position, &next, &width, &height);
        if (status != SIM_GRAPHICS_OK) return status;
        if (index == frame_index) {
            frame->width = width;
            frame->height = height;
            frame->kind = record.kind;
            frame->pixels.data = pack->data + position + 4;
            frame->pixels.size = next - position - 4;
            return SIM_GRAPHICS_OK;
        }
        position = next;
    }
    return SIM_GRAPHICS_OUT_OF_RANGE;
}

size_t sim_graphics_pack_sizeof(void)
{
    return sizeof(sim_graphics_pack);
}
