/* Streaming reconstruction of the used Miles XMIDI path from ADLIB.ADV.
 * Does not synthesize instruments or emulate an OPL device. */
#include "xmidi.h"
#include <string.h>

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}

static int find_events(const uint8_t *data,size_t size,unsigned depth,
                       const uint8_t **found,size_t *length)
{
    size_t position=0;
    if (depth>8) return SIM_XMIDI_BAD_FORMAT;
    while (position<size) {
        const uint8_t *chunk;
        uint32_t bytes;
        int result;
        if (size-position<8) return SIM_XMIDI_TRUNCATED;
        chunk=data+position;bytes=be32(chunk+4);position+=8;
        if (bytes>size-position) return SIM_XMIDI_TRUNCATED;
        if (!memcmp(chunk,"EVNT",4)) {
            if (*found) return SIM_XMIDI_UNSUPPORTED;
            *found=data+position;*length=bytes;
        } else if (!memcmp(chunk,"FORM",4) || !memcmp(chunk,"CAT ",4)) {
            if (bytes<4) return SIM_XMIDI_BAD_FORMAT;
            if (memcmp(data+position,"XMID",4) && memcmp(data+position,"XDIR",4))
                return SIM_XMIDI_BAD_FORMAT;
            result=find_events(data+position+4,bytes-4,depth+1,found,length);
            if (result) return result;
        }
        position+=bytes;
        if (bytes&1) {
            if (position==size) break; /* final pad may be omitted */
            ++position;
        }
    }
    return SIM_XMIDI_OK;
}

int sim_xmidi_open_events(sim_xmidi *p,const uint8_t *events,size_t size)
{
    unsigned i;
    if (!p) return SIM_XMIDI_BAD_FORMAT;
    memset(p,0,sizeof(*p));
    for (i=0;i<4;++i) p->loop_count[i]=0xffff;
    if (!events || !size) { p->error=SIM_XMIDI_BAD_FORMAT;return p->error; }
    p->events=events;p->size=size;return SIM_XMIDI_OK;
}

int sim_xmidi_open(sim_xmidi *p,const uint8_t *xmi,size_t size)
{
    const uint8_t *events=NULL;
    size_t length=0;
    int result;
    if (!p) return SIM_XMIDI_BAD_FORMAT;
    if (!xmi || size<12 || memcmp(xmi,"FORM",4))
        return sim_xmidi_open_events(p,NULL,0);
    result=find_events(xmi,size,0,&events,&length);
    if (!result) return sim_xmidi_open_events(p,events,length);
    (void)sim_xmidi_open_events(p,NULL,0);p->error=result;return result;
}

static int vlq(sim_xmidi *p,uint32_t *value)
{
    unsigned i;
    *value=0;
    for (i=0;i<4;++i) {
        uint8_t byte;
        if (p->position==p->size) return SIM_XMIDI_TRUNCATED;
        byte=p->events[p->position++];
        *value=(*value<<7)|(byte&127);
        if (!(byte&128)) return SIM_XMIDI_OK;
    }
    return SIM_XMIDI_BAD_FORMAT;
}

static void emit(sim_xmidi *p,sim_xmidi_event_fn callback,void *user,
                  uint8_t status,uint8_t a,uint8_t b)
{
    if (callback) callback(user,p->tick,status,a,b);
}

static int events_due(sim_xmidi *p,sim_xmidi_event_fn callback,void *user)
{
    unsigned safety=65536;
    while (!p->done && !p->wait) {
        uint8_t status,kind,a,b=0;
        uint32_t duration,length;
        unsigned i;
        int result;
        if (!safety--) return SIM_XMIDI_EVENT_LIMIT;
        if (p->position==p->size) return SIM_XMIDI_TRUNCATED;
        status=p->events[p->position++];
        if (status<128) {
            /* Original service yields even for an explicit zero interval. */
            p->wait=status?status:1;
            break;
        }
        kind=status>>4;
        if (kind>=8 && kind<=14) {
            if (p->position==p->size) return SIM_XMIDI_TRUNCATED;
            a=p->events[p->position++];
            if (kind!=12 && kind!=13) {
                if (p->position==p->size) return SIM_XMIDI_TRUNCATED;
                b=p->events[p->position++];
            }
            if (a>=128 || b>=128) return SIM_XMIDI_BAD_FORMAT;
            if (kind==8 || kind==9) {
                result=vlq(p,&duration);if (result) return result;
                for (i=0;i<32 && p->notes[i].active;++i) {}
                if (i==32) i=0; /* Original queue overwrites slot0 when full. */
                p->notes[i].active=1;p->notes[i].channel=status&15;
                p->notes[i].note=a;p->notes[i].deadline=p->tick+(duration?duration:1);
                emit(p,callback,user,(uint8_t)(0x90|(status&15)),a,b);
            } else if (kind==11 && a==116) {
                for (i=0;i<4 && p->loop_count[i]!=0xffff;++i) {}
                if (i<4) { p->loop_count[i]=b;p->loop_position[i]=p->position; }
            } else if (kind==11 && a==117) {
                if (b>=64) {
                    i=4;
                    while (i && p->loop_count[i-1]==0xffff) --i;
                    if (i) {
                        --i;
                        if (!p->loop_count[i] || --p->loop_count[i]) p->position=p->loop_position[i];
                        else p->loop_count[i]=0xffff;
                    }
                }
            } else if (kind==11 && a>=110 && a<=120) {
                /* No such controls occur in SCTUNE; do not fake locks, bank
                 * mapping, indirect callbacks or sequence branches. */
                return SIM_XMIDI_UNSUPPORTED;
            } else emit(p,callback,user,status,a,b);
        } else if (status==0xff) {
            if (p->position==p->size) return SIM_XMIDI_TRUNCATED;
            a=p->events[p->position++];
            result=vlq(p,&length);if (result) return result;
            if (length>p->size-p->position) return SIM_XMIDI_TRUNCATED;
            p->position+=length;
            if (a==0x2f) p->done=1;
        } else return SIM_XMIDI_UNSUPPORTED;
    }
    return SIM_XMIDI_OK;
}

int sim_xmidi_advance_ticks120(sim_xmidi *p,uint32_t ticks,
                              sim_xmidi_event_fn callback,void *user)
{
    uint32_t step;
    unsigned i;
    int result;
    if (!p || !p->events) return SIM_XMIDI_BAD_FORMAT;
    if (p->error) return p->error;
    if (!p->started) {
        p->started=1;result=events_due(p,callback,user);
        if (result) { p->error=result;return result; }
    }
    for (step=0;step<ticks && !p->done;++step) {
        ++p->tick;
        /* Original note queue is serviced before sequence events each tick. */
        for (i=0;i<32;++i) if (p->notes[i].active && p->notes[i].deadline<=p->tick) {
            p->notes[i].active=0;
            emit(p,callback,user,(uint8_t)(0x80|p->notes[i].channel),p->notes[i].note,0);
        }
        if (p->wait) --p->wait;
        if (!p->wait) {
            result=events_due(p,callback,user);
            if (result) { p->error=result;return result; }
        }
    }
    return SIM_XMIDI_OK;
}

void sim_xmidi_stop(sim_xmidi *p,sim_xmidi_event_fn callback,void *user)
{
    unsigned i;
    if (!p) return;
    for (i=0;i<32;++i) if (p->notes[i].active) {
        p->notes[i].active=0;
        emit(p,callback,user,(uint8_t)(0x80|p->notes[i].channel),p->notes[i].note,0);
    }
    p->done=1;
}
