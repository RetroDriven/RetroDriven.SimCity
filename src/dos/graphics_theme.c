#include "graphics_theme.h"
#include "resource_lzss.h"
#include <string.h>

static unsigned u16(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
static uint32_t u32(const uint8_t *p){return u16(p)|((uint32_t)u16(p+2)<<16);}
static void p16(uint8_t *p,unsigned n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static int field(char *out,size_t capacity,const uint8_t *raw,size_t limit,size_t *position)
{
    size_t start=*position,length;
    while (*position<limit && raw[*position]) ++*position;
    if (*position==limit) return 0;
    length=*position-start;if (!length || length>=capacity) return 0;
    memcpy(out,raw+start,length);out[length]=0;++*position;return 1;
}

int sim_graphics_theme_header(sim_graphics_theme_info *info,const uint8_t *raw,size_t size)
{
    size_t position=0,limit=size<256?size:256;
    if (!info) return SIM_THEME_BAD_ARGUMENT;
    memset(info,0,sizeof(*info));
    if (!raw) return SIM_THEME_BAD_ARGUMENT;
    if (limit<8 || raw[0]!=0xc9) return SIM_THEME_BAD_HEADER;
    for (position=1;position<8;++position) if (raw[position]!=0xcd) return SIM_THEME_BAD_HEADER;
    while (position<limit && raw[position]!=26) ++position;
    if (position==limit) return SIM_THEME_BAD_HEADER;
    ++position;
    if (!field(info->title,sizeof(info->title),raw,limit,&position) || position==limit) return SIM_THEME_BAD_HEADER;
    info->graphics_mode=raw[position++];
    if (info->graphics_mode!='2') return SIM_THEME_BAD_HEADER;
    if (!field(info->message_file,sizeof(info->message_file),raw,limit,&position) ||
        !field(info->sound_file,sizeof(info->sound_file),raw,limit,&position) ||
        !field(info->fallback_graphics,sizeof(info->fallback_graphics),raw,limit,&position)) return SIM_THEME_BAD_HEADER;
    info->compressed_offset=position;return SIM_THEME_OK;
}

int sim_graphics_theme_unpack(sim_graphics_theme_info *info,const uint8_t *raw,size_t size,
                               uint8_t *decoded,size_t capacity,size_t *written)
{
    sim_lzss decoder;uint8_t extra;size_t count;int result;
    if (written) *written=0;
    if (!info || !decoded || !capacity || !written) return SIM_THEME_BAD_ARGUMENT;
    result=sim_graphics_theme_header(info,raw,size);if (result) return result;
    sim_lzss_init(&decoder,raw+info->compressed_offset,size-info->compressed_offset);
    count=sim_lzss_read(&decoder,decoded,capacity);
    if (sim_lzss_malformed(&decoder)) return SIM_THEME_BAD_DATA;
    if (count==capacity && sim_lzss_read(&decoder,&extra,1)) return SIM_THEME_NO_SPACE;
    if (sim_lzss_malformed(&decoder) || !count) return SIM_THEME_BAD_DATA;
    info->decoded_size=count;*written=count;return SIM_THEME_OK;
}

int sim_graphics_theme_normalize(sim_graphics_theme_info *info,const uint8_t *data,
                                  size_t size,uint8_t *out,size_t capacity,size_t *written)
{
    size_t records[16],lengths[16],position=62225,needed=71363,index,dest;
    uint8_t kinds[16];sim_graphics_pack check;
    if (written) *written=0;
    if (!info || !data || !out || !written || out==data) return SIM_THEME_BAD_ARGUMENT;
    if (size<62225 || u16(data)!=17 || data[2]!=8 || u16(data+773)!=8 ||
        u16(data+775)!=8 || u16(data+777)!=960 || u32(data+781)!=61440) return SIM_THEME_BAD_DATA;
    for (index=0;index<16;++index) {
        size_t bytes,area,frames;
        unsigned width,height;
        if (position>size || size-position<12) return SIM_THEME_BAD_DATA;
        width=u16(data+position);height=u16(data+position+2);frames=u16(data+position+4);
        bytes=u32(data+position+8);
        if (!width || !height || !frames || bytes>65535 || bytes>size-position-12 ||
            data[position+6]!=1 || data[position+7]>1) return SIM_THEME_BAD_DATA;
        if ((size_t)height>(bytes>=4?bytes-4:0)/width) return SIM_THEME_BAD_DATA;
        area=(size_t)width*height+4;
        if (frames>bytes/area || frames*area!=bytes) return SIM_THEME_BAD_DATA;
        kinds[index]=data[position+7]?1:8;records[index]=position+12;lengths[index]=bytes;
        if (needed>SIZE_MAX-bytes-3) return SIM_THEME_BAD_DATA;
        needed+=bytes+3;position+=bytes+12;
        if (index==9) {info->train_frames=(uint16_t)frames;info->extended_train=(uint8_t)(frames==8);}
    }
    if (position>size || size-position!=963+8192 || data[position]!=8 || u16(data+position+1)!=960) return SIM_THEME_BAD_DATA;
    info->base_year=(uint16_t)u16(data+3);info->decoded_size=size;info->normalized_size=needed;
    if (capacity<needed) return SIM_THEME_NO_SPACE;
    for (index=0;index<768;++index) out[index]=(uint8_t)(data[5+index]>>2);
    memcpy(out+768,data+785,61440);memcpy(out+62208,data+position+963,8192);
    memcpy(out+70400,data+position,963);dest=71363;
    for (index=0;index<16;++index) {
        out[dest]=kinds[index];p16(out+dest+1,(unsigned)lengths[index]);
        memcpy(out+dest+3,data+records[index],lengths[index]);dest+=lengths[index]+3;
    }
    if (sim_graphics_pack_open(&check,out,needed)!=SIM_GRAPHICS_OK) return SIM_THEME_BAD_DATA;
    *written=needed;return SIM_THEME_OK;
}

int sim_graphics_themes_entry(const sim_graphics_themes *a,unsigned index,sim_graphics_theme_asset *asset)
{
    const uint8_t *entry;uint32_t offset,bytes;unsigned j;
    static const unsigned starts[5]={0,32,48,64,80},sizes[5]={32,16,16,16,16};
    char *strings[5];
    if (!asset) return 0;
    memset(asset,0,sizeof(*asset));
    if (!a || !a->data || index>=a->count || a->size<16+(size_t)a->count*128) return 0;
    entry=a->data+16+(size_t)index*128;
    strings[0]=asset->title;strings[1]=asset->source_file;strings[2]=asset->message_file;
    strings[3]=asset->sound_file;strings[4]=asset->fallback_graphics;
    for (j=0;j<5;++j) {
        if (!memchr(entry+starts[j],0,sizes[j])) return 0;
        memcpy(strings[j],entry+starts[j],sizes[j]);
    }
    asset->base_year=(uint16_t)u16(entry+96);asset->train_frames=(uint16_t)u16(entry+98);
    for (j=0;j<2;++j) {
        sim_graphics_span *span=j?&asset->messages:&asset->graphics;
        offset=u32(entry+100+j*8);bytes=u32(entry+104+j*8);
        if (offset<16+(size_t)a->count*128 || offset>a->size || bytes>a->size-offset) return 0;
        span->data=a->data+offset;span->size=bytes;
    }
    return 1;
}

int sim_graphics_themes_open(sim_graphics_themes *a,const uint8_t *data,size_t size)
{
    sim_graphics_themes result;unsigned i;
    if (!a) return 0;
    memset(a,0,sizeof(*a));
    if (!data || size<16 || memcmp(data,"SCTHMCG1",8) || u32(data+12)!=128 ||
        !u32(data+8) || u32(data+8)>64 || (size_t)u32(data+8)>(size-16)/128) return 0;
    result.data=data;result.size=size;result.count=u32(data+8);
    for (i=0;i<result.count;++i) {
        sim_graphics_theme_asset entry;sim_graphics_pack check;
        if (!sim_graphics_themes_entry(&result,i,&entry) ||
            sim_graphics_pack_open(&check,entry.graphics.data,entry.graphics.size)!=SIM_GRAPHICS_OK ||
            !entry.messages.size) return 0;
    }
    *a=result;return 1;
}
