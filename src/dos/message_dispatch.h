#ifndef SIMCITY_DOS_MESSAGE_DISPATCH_H
#define SIMCITY_DOS_MESSAGE_DISPATCH_H
#include <stdint.h>
#include <stddef.h>
typedef struct { int x,y; char text[256]; } SimMessageText;
typedef struct {
 int clear,x0,y0,x1,y1;
 int icon,icon_x,icon_y,alert3,open_editor,viewport_changed;
 int modal_notification,modal_image_line,refresh_status,text_count;
 SimMessageText text[2];
} SimMessageEffects;
/* Effects are reset at entry. Render clear in color8, then icon, then text
 * in MCGA foreground7/background8. Text strings are copied, caller owned.
 * A modal notification is its original signed ID; caller must acknowledge UI.
 */
/* Called at the original synchronous Editor activation boundary. Pending320c
 * is temporarily zero during the callback, then restored before dispatch
 * resumes. The callback may activate Editor and execute its real repaint.
 * open_editor still records the event; do not activate a second time afterward.
 */
typedef void (*SimMessageEditorOpen)(uint8_t *memory,size_t size,uint32_t clock,void *user);
int sim_message_dispatch_with_editor(uint8_t *memory,size_t size,int notify,uint32_t clock,SimMessageEffects *effects,SimMessageEditorOpen hook,void *user);
int sim_message_pump_with_editor(uint8_t *memory,size_t size,uint32_t clock,SimMessageEffects *effects,SimMessageEditorOpen hook,void *user);
int sim_message_flush_with_editor(uint8_t *memory,size_t size,uint32_t clock,SimMessageEffects *effects,SimMessageEditorOpen hook,void *user);
int sim_message_dispatch(uint8_t *memory,size_t size,int notify,uint32_t clock,SimMessageEffects *effects);
int sim_message_pump(uint8_t *memory,size_t size,uint32_t clock,SimMessageEffects *effects);
const char *sim_message_modal_text(const uint8_t *memory,size_t size,int signed_id);
int sim_message_flush(uint8_t *memory,size_t size,uint32_t clock,SimMessageEffects *effects);
int sim_messages_configure_mcga(uint8_t *memory,size_t size);
int sim_messages_bind_icon(uint8_t *memory,size_t size,const uint8_t *frame,size_t frame_size);
#endif
