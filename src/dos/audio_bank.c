#include "audio_bank.h"
#include <string.h>
static uint32_t le32(const uint8_t *p)
{ return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
int sim_audio_sound(const uint8_t *bank,size_t size,unsigned id,sim_pcm_sound *out)
{
    uint32_t offset,length,rate,count;
    if (!out) return 0;
    memset(out,0,sizeof(*out));
    if (!bank || id>=8 || size<84 || memcmp(bank,"SCAUD001",8) || le32(bank+8)!=9) return 0;
    offset=le32(bank+12+id*8);length=le32(bank+16+id*8);
    if (offset<84 || offset>size || length>size-offset || length<8 || (offset&1)) return 0;
    rate=le32(bank+offset);count=le32(bank+offset+4);
    if (rate<1000 || rate>48000 || !count || count>(length-8)/2 || count*2!=length-8) return 0;
    out->pcm16=bank+offset+8;out->count=count;out->rate=rate;
    return 1;
}
int sim_audio_music(const uint8_t *bank,size_t size,const uint8_t **xmi,size_t *length)
{
    uint32_t offset,count;
    if(!xmi || !length)return 0;
    *xmi=NULL;*length=0;
    if(!bank || size<84 || memcmp(bank,"SCAUD001",8) || le32(bank+8)!=9)return 0;
    offset=le32(bank+76);count=le32(bank+80);
    if(offset<84 || offset>size || count>size-offset || count<12 || memcmp(bank+offset,"FORM",4))return 0;
    *xmi=bank+offset;*length=count;return 1;
}
