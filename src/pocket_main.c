/* Native Pocket frontend for recovered DOS2.00 behavior and assets. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "of.h"
#include "dos/resource_lzss.h"
#include "dos/graphics_pack.h"
#include "dos/picture.h"
#include "dos/mcga_draw.h"
#include "dos/title_menu.h"
#include "dos/new_city.h"
#include "dos/terrain.h"
#include "dos/ui_draw.h"
#include "dos/name_edit.h"
#include "virtual_keyboard.h"
#include "placement_preview.h"
#include "dos/city_view.h"
#include "dos/city_tools.h"
#include "dos/placement.h"
#include "dos/audio_bank.h"
#include "city_session.h"
#include "dos/scenario.h"
#include "dos/menus.h"
#include "dos/budget_ui.h"
#include "dos/sprites.h"
#include "dos/city_stats.h"
#include "dos/evaluation_ui.h"
#include "dos/graphs_ui.h"
#include "dos/maps.h"
#include "dos/message_popup.h"
#include "dos/question_ui.h"
#include "dos/query_ui.h"
#include "dos/about_ui.h"
#include "dos/terrain_edit.h"
#include "dos/windows.h"
#include "dos/editor_shell.h"
#include "dos/graphics_theme.h"
#include "dos/messages.h"
#include "pocket_music.h"

static uint8_t title[64000], scenario[64000], canvas[64000],city_layer[64000];
static int city_layer_valid;
static SimWindowDrag window_drag;
static int dragging_window,queued_center,queued_center_x,queued_center_y;
static int drag_mouse,drag_mouse_pending,mouse_press_x,mouse_press_y,drag_origin_x,drag_origin_y;
static uint8_t graphics[262144];
static size_t graphics_size=146939;
static uint8_t *standard_graphics,*themes_data;
static size_t themes_size;
static sim_graphics_themes themes;
static unsigned graphics_choice;
static uint16_t graphics_year=1900,graphics_train;
static const uint8_t *font_data,*tile_data,*graph_art,*popup_art,*icon_art,*active_messages;
static size_t graph_art_size,popup_art_size,active_message_size;
static uint8_t cursor[520];
static uint32_t palette[256],standard_palette[256],game_palette[256];
static sim_graphics_pack graphics_pack;
static uint32_t city_funds;
static unsigned city_tool = SIM_CITY_ROAD;
static sim_city_placement_state placement;
static uint8_t *audio_data;
static size_t audio_size;
static int sound_on = 1;
static of_mixer_handle_t sfx_handle;
static int stroke_active,stroke_failed,last_build_x,last_build_y;
static unsigned last_build_tool;
static city_session session;
static uint8_t *code_data,*message_data,*scenario_data;
static size_t code_size,message_size,scenario_size;
static uint8_t *terrain_toolbar;
static size_t terrain_toolbar_size;
static int edit_focus;
static uint8_t save_buffer[SIM_CITY_SAVE_BYTES],message_strip[320*16];
static sim_menus game_menus;
static int open_menu=-1,menu_row,budget_focus=8,queued_event=-1;
static int queued_place,queued_x,queued_y,queued_tool;
static unsigned message_serial;
static const char *notice;
static const char *system_popup;
static const char *question_text;
static int question_kind=-1,question_event,question_row,question_approved=-1;
static int report_window;
static int graph_focus;
static int map_focus;
static int map_navigation,map_pointer_x=60,map_pointer_y=50;
static int popup_id;
static uint32_t popup_clock;
static SimQueryInfo query_info;
static int query_row=3;
static int animation_pending,quake_active,quake_pending,video_ready,foreground_done;
static int display_selection=-1;
static uint32_t quake_start;
static of_video_mode_t active_video_mode;
static int file_mode,file_selection,save_slot;
static char save_names[6][20]={"SimCity_0.sav","SimCity_1.sav","SimCity_2.sav","SimCity_3.sav","SimCity_4.sav","SimCity_5.sav"};
static char city_name[21];
static unsigned rw(const uint8_t *p){return p[0]|((unsigned)p[1]<<8);}
static void ww(uint8_t *p,unsigned n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void sync_city(void);
static void simulation_frame(void);
static void graphics_state(void);
static void render(int focus);
#ifdef SIM_VIEW_TEST
uint32_t sim_core_settings_word(void);
#else
static uint32_t sim_core_settings_word(void)
{
#if defined(__riscv) && !defined(OF_PC)
    return *(volatile const uint32_t *)(uintptr_t)0x40000080u;
#else
    return 0;
#endif
}
#endif
#ifdef SIM_VIEW_TEST
uint32_t sim_core_volume_word(void);
#else
static uint32_t sim_core_volume_word(void)
{
#if defined(__riscv) && !defined(OF_PC)
    /* Same dedicated F700000C latch / 4000006C mirror as Commander Keen. */
    return *(volatile const uint32_t *)(uintptr_t)0x4000006cu;
#else
    return 0;
#endif
}
#endif
#ifdef SIM_AUDIO_SETTINGS_TEST
void sim_set_audio_levels(int master,int effects);
#else
static void sim_set_audio_levels(int master,int effects)
{
    of_mixer_set_master_volume(master);
    of_mixer_set_group_volume(OF_MIXER_GROUP_SFX,effects);
}
#endif
static int placement_preview_enabled(void)
{return (sim_core_settings_word()&0x00080000u)==0;}
static unsigned view_height(void)
{return sim_core_settings_word()&0x00100000u?256u:240u;}
static void select_display(int force)
{
    int wanted=OF_DISPLAY_FRAMEBUFFER;
    if(force||display_selection!=wanted){
        of_video_set_display_mode(wanted);display_selection=wanted;
        if(wanted==OF_DISPLAY_FRAMEBUFFER)of_video_palette_bulk(palette,256);
    }
}
static int active_report(void)
{unsigned id=city_session_dgroup(&session)[0x5326];return id==2?3:id==3?2:0;}
static int modal_report(void)
{return report_window&&report_window!=2&&report_window!=3;}
static int foreground_allowed(void)
{return !modal_report()&&open_menu<0&&!file_mode&&!notice&&!system_popup&&!dragging_window&&question_kind<0;}
static void open_editor_for_message(uint8_t *memory,size_t size,uint32_t clock,void *user)
{
    SimMessageEffects nested;(void)user;
    sim_windows_activate(memory,size,1);
    sim_editor_shell_repaint(memory,size,clock,&nested);
    memset(message_strip,8,sizeof(message_strip));city_layer_valid=0;report_window=0;
}
static void repaint_editor(unsigned count)
{
    uint8_t *clock=session.cpu.memory+0x220fa;
    uint32_t now=rw(clock)|((uint32_t)rw(clock+2)<<16);
    if(!city_session_dgroup(&session)[0x2969])return;
    while(count--){
        SimMessageEffects effect;
        if(sim_editor_shell_repaint(session.cpu.memory,SIM_DOS_MEMORY_SIZE,now,&effect)){
            session.effects=effect;++session.effects_serial;memset(message_strip,8,sizeof(message_strip));
        }
    }
    city_layer_valid=0;
}
static void activate_window(unsigned id)
{
    unsigned previous=city_session_dgroup(&session)[0x5326];
    sim_windows_activate(session.cpu.memory,SIM_DOS_MEMORY_SIZE,id);
    if(previous!=id&&id==1)repaint_editor(1);
    report_window=active_report();
}
static void leave_terrain_edit(void)
{
    /* Original3270e precedes Scenario, New City and Load dialogs. It also
     * reactivates/repaints the normal City controls before a canceled dialog. */
    if(!session.active||!city_session_dgroup(&session)[0x32f2])return;
    sim_terrain_edit_exit(session.cpu.memory,SIM_DOS_MEMORY_SIZE);
    activate_window(1);repaint_editor(1);sync_city();
}

static uint32_t game_clock(void)
{
    static uint32_t previous;static uint64_t elapsed_us;
    uint32_t now=of_time_us();elapsed_us+=(uint32_t)(now-previous);previous=now;
    return (uint32_t)(elapsed_us/54925);
}
static int pan_button(uint32_t mask,unsigned index)
{
    static uint32_t due[4];uint32_t now=of_time_ms();
    if(of_btn_pressed(mask)){due[index]=now+300;return 1;}
    if(of_btn(mask)&&(int32_t)(now-due[index])>=0){due[index]=now+60;return 1;}
    return 0;
}

static sim_title_menu menu;
static sim_new_city new_city;
static sim_terrain terrain;
static sim_city_view city_view;
static sim_name_edit name_edit;
static SimVirtualKeyboard name_keyboard;
static int keyboard_mouse_active;
static int editing_name;
#define TITLE 0
#define SCENARIO 1
#define NEW_CITY 2
#define CITY 3
static int screen, scenario_selected;
static int cursor_x = 103, cursor_y = 162;
static uint32_t controller_consumed;
static int ui_click_consumed;
static int controller_pressed(uint32_t mask){return of_btn_pressed(mask&~controller_consumed);}
static int controller_held(uint32_t mask){return of_btn(mask&~controller_consumed);}
static int pan_down,pan_pending;
static int music_on=1;
static int core_music_on=1,core_sound_on=1,core_volume=255;

static void apply_music(void)
{
    if(!music_on||!core_music_on){pocket_music_stop();return;}
#ifndef OF_PC
    const uint8_t *xmi;size_t length;
    if(!sim_audio_music(audio_data,audio_size,&xmi,&length)||!pocket_music_start(xmi,length)){
        music_on=0;notice="Unable to start music";
    }
#endif
}

static void refresh_audio_settings(int force)
{
    uint32_t settings=sim_core_settings_word();
    int music=(settings&0x00200000u)==0,sound=(settings&0x00400000u)==0;
    int volume;
    switch(sim_core_volume_word()){
    case 75:volume=191;break;case 50:volume=128;break;
    case 25:volume=64;break;case 1:volume=0;break;
    default:volume=255;break; /* Unset/unknown defaults to full volume. */
    }
    int music_changed=music!=core_music_on;
    if(force||volume!=core_volume||sound!=core_sound_on)sim_set_audio_levels(volume,sound?255:0);
    core_music_on=music;core_sound_on=sound;core_volume=volume;
    if(force||music_changed)apply_music();
}

/* Cumulative milli-pixels for a gradual, frame-rate independent ramp.
 * Start gently, then accelerate through 240, 480 and 640 pixels/second. */
static uint64_t pointer_distance(uint32_t age)
{
    uint64_t distance=0;uint32_t span=age<120?age:120;
    distance+=(uint64_t)span*80;
    if(age<=120)return distance;
    age-=120;
    span=age<230?age:230;distance+=(uint64_t)span*240;
    if(age<=230)return distance;
    age-=230;
    span=age<300?age:300;distance+=(uint64_t)span*480;
    if(age>300)distance+=(uint64_t)(age-300)*640;
    return distance;
}

/* A tap moves one pixel. Holding moves continuously without a repeat delay. */
static int pointer_button(uint32_t mask,unsigned index)
{
    static uint32_t last[4],started[4],fraction[4];
    static int held[4];
    uint32_t now=of_time_ms();
    if(!of_btn(mask)){held[index]=0;fraction[index]=0;return 0;}
    if(!held[index]||of_btn_pressed(mask)){
        held[index]=1;last[index]=started[index]=now;fraction[index]=0;return 1;
    }
    uint32_t elapsed=now-last[index];last[index]=now;
    /* Avoid a large jump after a stalled frame or a modal window. */
    if(elapsed>50)elapsed=50;
    uint32_t age=now-started[index];
    fraction[index]+=(uint32_t)(pointer_distance(age)-pointer_distance(age-elapsed));
    int pixels=(int)(fraction[index]/1000);fraction[index]%=1000;
    /* Keep held construction contiguous at the original eight-pixel tiles. */
    if(of_btn(OF_BTN_A)&&pixels>4)pixels=4;
    return pixels;
}

/* Map movement uses elapsed time, with immediate response and no key delay. */
static int map_pan_button(uint32_t mask,unsigned index)
{
    static uint32_t last[4],started[4],fraction[4];static int held[4];
    uint32_t now=of_time_ms();
    if(!of_btn(mask)){held[index]=0;fraction[index]=0;return 0;}
    if(!held[index]||of_btn_pressed(mask)){
        held[index]=1;last[index]=started[index]=now;fraction[index]=0;return 1;
    }
    uint32_t elapsed=now-last[index];last[index]=now;
    if(elapsed>100)elapsed=100;
    fraction[index]+=elapsed*((uint32_t)(now-started[index])<250?24u:48u);
    int tiles=(int)(fraction[index]/1000);fraction[index]%=1000;return tiles;
}

/* Holding into a map edge scrolls after a brief dwell. A fresh press at
 * that edge exits to the surrounding UI instead of trapping the hand. */
typedef struct {
    int direction,edge,moving;
    uint32_t since,last,fraction;
} EdgeScroll;
static EdgeScroll edge_scroll[2];
static uint64_t edge_distance(uint32_t age)
{
    return age<300?(uint64_t)age*24:7200u+(uint64_t)(age-300)*48;
}
static int edge_pointer_delta(unsigned axis,int position,int delta,int low,int high,
                              int orthogonal,int orth_low,int orth_high,
                              uint32_t negative,uint32_t positive,int enabled,
                              int *origin,int maximum)
{
    EdgeScroll *e=&edge_scroll[axis];uint32_t now=of_time_ms();
    int neg=of_btn(negative)!=0,pos=of_btn(positive)!=0;
    int direction=neg==pos?0:(pos?1:-1);
    if(!enabled||!direction||orthogonal<orth_low||orthogonal>orth_high){
        memset(e,0,sizeof(*e));return delta;
    }
    if(e->direction&&of_btn_pressed(negative|positive)){
        memset(e,0,sizeof(*e));return delta;
    }
    if(e->direction&&(e->direction!=direction||position!=e->edge))memset(e,0,sizeof(*e));
    if(!e->direction){
        if(position>low&&position<high&&
           ((direction<0&&position+delta<=low)||(direction>0&&position+delta>=high))){
            e->direction=direction;e->edge=direction<0?low:high;e->since=e->last=now;
            return e->edge-position;
        }
        return delta;
    }
    uint32_t age=now-e->since,elapsed=now-e->last;e->last=now;
    if(age<250)return 0;
    if(elapsed>100)elapsed=100;
    uint32_t motion=age-250,previous=age>elapsed+250?age-elapsed-250:0;
    e->fraction+=(uint32_t)(edge_distance(motion)-edge_distance(previous));
    int tiles=(int)(e->fraction/1000);e->fraction%=1000;
    if(!e->moving){e->moving=1;++tiles;}
    int next=*origin+direction*tiles;
    if(next<0)next=0;
    if(next>maximum)next=maximum;
    if(next!=*origin){*origin=next;city_layer_valid=0;}
    return 0;
}

static int expanded_city_layout,menu_revealed;
static uint32_t menu_reveal_time;
static int city_menu_visible(void)
{
    return open_menu>=0||cursor_y<12||
        (menu_revealed&&(uint32_t)(of_time_ms()-menu_reveal_time)<1200);
}

/* Reuse the recovered geometry transactions: shift the Editor up twelve
 * pixels and spend the free height on complete map rows. Menus overlay it. */
static void expand_city_layout(void)
{
    SimWindowRect r;SimWindowDrag drag={0};
    if(expanded_city_layout||!session.active||
       !sim_window_rectangle(session.cpu.memory,SIM_DOS_MEMORY_SIZE,1,0,&r))return;
    expanded_city_layout=1;
    int shift=r.top>=13?-12:0;
    drag.window=1;drag.mode=SIM_WINDOW_POSITION;drag.dy=shift;
    sim_window_drag_commit(session.cpu.memory,SIM_DOS_MEMORY_SIZE,&drag);
    int height=(199-r.bottom-shift)/8*8;if(height>16)height=16;
    if(height>0){drag.mode=SIM_WINDOW_RESIZE;drag.dy=height;
        sim_window_drag_commit(session.cpu.memory,SIM_DOS_MEMORY_SIZE,&drag);}
    sim_editor_shell_metrics(session.cpu.memory,SIM_DOS_MEMORY_SIZE);
    city_layer_valid=0;memset(message_strip,8,sizeof(message_strip));
}

static void new_city_action(sim_new_city_action action)
{
    if (action == SIM_NEW_CITY_ACCEPT) {
        sim_terrain_generate(&terrain, (uint16_t)(of_time_us() / 54925));
        sim_city_view_init(&city_view);
        city_funds = sim_new_city_funds(&new_city);
        report_window=file_mode=0;open_menu=-1;city_layer_valid=0;
        if(!city_session_new(&session,terrain.tiles,new_city.name,(unsigned)new_city.difficulty,
                             city_funds,game_clock())) {
            notice="City initialization failed";
        }else {
            for(unsigned i=0;i<5;++i)ww(city_session_dgroup(&session)+0x3b48+i*2,terrain.random[i]);
            ww(city_session_dgroup(&session)+0x4ea8,(unsigned)terrain.start_x);
            ww(city_session_dgroup(&session)+0x4eaa,(unsigned)terrain.start_y);
            ww(city_session_dgroup(&session)+0x4eac,(unsigned)terrain.direction);
            ww(city_session_dgroup(&session)+0x4eae,(unsigned)terrain.initial_direction);
            graphics_state();
            city_tool=city_session_dgroup(&session)[0x2b1a];
            if(city_tool<1||city_tool>16)city_tool=SIM_CITY_ROAD;
            report_window=active_report();
            memset(message_strip,8,sizeof(message_strip));foreground_done=0;quake_active=quake_pending=0;sync_city();
        }
        cursor_x = city_view.screen_x + ((int)city_view.columns/2)*8+4;
        cursor_y = city_view.screen_y + ((int)city_view.rows/2)*8+4;
        screen = CITY;
    } else if (action == SIM_NEW_CITY_EDIT_NAME) {
        sim_name_edit_init(&name_edit, new_city.name);
        sim_virtual_keyboard_init(&name_keyboard);keyboard_mouse_active=0;
        editing_name = 1;
    }
}

static void edit_key(unsigned key)
{
    if (sim_name_edit_key(&name_edit, key)) {
        char result[19];
        sim_name_edit_result(&name_edit, result);
        sim_new_city_set_name(&new_city, result);
        editing_name = 0;
    }
}

static void name_keyboard_action(unsigned code)
{
    if(code==SIM_VK_CLEAR){name_edit.cursor=name_edit.length=0;name_edit.text[0]=0;}
    else if(code==SIM_VK_CANCEL){name_edit.cancelled=1;edit_key(13);}
    else edit_key(code==SIM_VK_DONE?13:code);
}

static int key_pressed(const of_keyboard_state_t *keys, unsigned usage)
{
    return keys->present && usage < OF_KEYBOARD_MAX_USAGE &&
           (keys->keys_pressed[usage / 32] & (1u << (usage % 32)));
}

static int editor_pad_pressed(uint32_t mask, const of_keyboard_state_t *keys)
{
#ifdef OF_PC
    /* The SDK desktop backend also maps Z/X/A/S to pad buttons. During
     * typing those must remain text, rather than duplicate editor actions. */
    if ((mask & OF_BTN_A) && key_pressed(keys, 0x1d)) mask &= ~OF_BTN_A;
    if ((mask & OF_BTN_B) && key_pressed(keys, 0x1b)) mask &= ~OF_BTN_B;
    if ((mask & OF_BTN_X) && key_pressed(keys, 4)) mask &= ~OF_BTN_X;
    if ((mask & OF_BTN_Y) && key_pressed(keys, 0x16)) mask &= ~OF_BTN_Y;
    if ((mask & OF_BTN_L1) && key_pressed(keys, 0x14)) mask &= ~OF_BTN_L1;
    if ((mask & OF_BTN_R1) && key_pressed(keys, 0x1a)) mask &= ~OF_BTN_R1;
#else
    (void)keys;
#endif
    return of_btn_pressed(mask);
}

static int play_sound(unsigned id)
{
    sim_pcm_sound sample;
    if(!sound_on || !core_sound_on || (sfx_handle && of_mixer_handle_active(sfx_handle)))return 0;
    if(sim_audio_sound(audio_data,audio_size,id,&sample)){
        sfx_handle=of_mixer_alloc_for_group_h(OF_MIXER_GROUP_SFX,sample.pcm16,sample.count,sample.rate,2,180);
        return sfx_handle!=0;
    }return 0;
}
static int session_sound(void *user,unsigned id){(void)user;return play_sound(id);}

static char resource_error[128];
static uint8_t *read_resource(const char *name, size_t *size)
{
    snprintf(resource_error,sizeof(resource_error),"%s: open/read failed",name);
    FILE *file = fopen(name, "rb");
    uint8_t *data;
    long length;
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) || (length = ftell(file)) <= 0 || length > 1048576 ||
        fseek(file, 0, SEEK_SET)) { fclose(file); return NULL; }
    data = malloc((size_t)length);
    if (!data) { fclose(file); return NULL; }
    *size = fread(data, 1, (size_t)length, file);
    fclose(file);
    if (*size != (size_t)length) { snprintf(resource_error,sizeof(resource_error),"%s: short read %lu/%lu",name,(unsigned long)*size,(unsigned long)length);free(data); return NULL; }
    snprintf(resource_error,sizeof(resource_error),"%s: validation failed (%lu bytes)",name,(unsigned long)length);
    return data;
}

static int cache_graphics(void)
{
    sim_graphics_span span;sim_graphics_frame frame;sim_graphics_record record;
    if(sim_graphics_pack_open(&graphics_pack,graphics,graphics_size)!=SIM_GRAPHICS_OK)return 0;
    if(sim_graphics_pack_section(&graphics_pack,SIM_GRAPHICS_FONT_MASKS,&span)!=SIM_GRAPHICS_OK)return 0;
    font_data=span.data;
    if(sim_graphics_pack_section(&graphics_pack,SIM_GRAPHICS_TILES,&span)!=SIM_GRAPHICS_OK)return 0;
    tile_data=span.data;
    if(sim_graphics_pack_frame(&graphics_pack,0,0,&frame)!=SIM_GRAPHICS_OK||frame.width!=8||frame.height!=8)return 0;
    icon_art=frame.pixels.data-4;
    if(sim_graphics_pack_frame(&graphics_pack,3,0,&frame)!=SIM_GRAPHICS_OK)return 0;
    graph_art=frame.pixels.data-4;graph_art_size=frame.pixels.size+4;
    if(sim_graphics_pack_frame(&graphics_pack,7,0,&frame)!=SIM_GRAPHICS_OK)return 0;
    popup_art=frame.pixels.data-4;popup_art_size=frame.pixels.size+4;
    if(sim_graphics_pack_record(&graphics_pack,9,&record)!=SIM_GRAPHICS_OK)return 0;
    graphics_train=graphics_choice&&record.frame_count==8;
    sim_palette_rgb8(graphics,game_palette);memcpy(palette,game_palette,sizeof(palette));return 1;
}
static void graphics_state(void)
{
    uint8_t *d=city_session_dgroup(&session);
    ww(d+0x294c,graphics_year);ww(d+0xd1ce,graphics_train);ww(d+0x1562,graphics_choice?0:1);
    city_session_assets(&session,code_data,code_size,active_messages,active_message_size,icon_art);
}
static int choose_graphics(unsigned choice)
{
    sim_graphics_theme_asset asset;
    if(choice>themes.count)return 0;
    if(choice){
        if(!sim_graphics_themes_entry(&themes,choice-1,&asset)||asset.graphics.size>sizeof(graphics))return 0;
        memcpy(graphics,asset.graphics.data,asset.graphics.size);graphics_size=asset.graphics.size;
        graphics_year=asset.base_year;active_messages=asset.messages.data;active_message_size=asset.messages.size;
    }else{memcpy(graphics,standard_graphics,146939);graphics_size=146939;graphics_year=1900;active_messages=message_data;active_message_size=message_size;}
    graphics_choice=choice;
    if(!cache_graphics()||!sim_menus_open(&game_menus,active_messages,active_message_size))return 0;
    graphics_state();
    if(session.active&&(!sim_messages_install(session.cpu.memory,SIM_DOS_MEMORY_SIZE,active_messages,active_message_size)||!sim_messages_bind_icon(session.cpu.memory,SIM_DOS_MEMORY_SIZE,icon_art,68)))return 0;
    memset(message_strip,8,sizeof(message_strip));city_layer_valid=0;
    if(video_ready)of_video_palette_bulk(palette,256);
    return 1;
}

static int load_resources(void)
{
    size_t size;
    uint8_t *data;
    sim_lzss codec;
#ifndef OF_PC
    of_file_slot_register(4, "simgfx.pgf");
    of_file_slot_register(5, "simtitle.ppf");
    of_file_slot_register(6, "simscen.ppf");
    of_file_slot_register(7, "cursor.dat");
    of_file_slot_register(25, "simaudio.dat");
    of_file_slot_register(23, "simcode.dat");
    of_file_slot_register(24, "scenarios.dat");
    of_file_slot_register(20, "messages.dat");
    of_file_slot_register(21, "terrainbar.dat");
    of_file_slot_register(22, "simthemes.dat");
    for(unsigned i=0;i<6;++i)of_file_slot_register(10+i,save_names[i]);
#endif
    data = read_resource("simgfx.pgf", &size);
    if (!data) return 0;
    sim_lzss_init(&codec, data, size);
    int good = sim_lzss_read(&codec, graphics, 146939) == 146939 &&
               !sim_lzss_malformed(&codec);
    free(data);
    if(!good||!cache_graphics())return 0;
    standard_graphics=malloc(146939);if(!standard_graphics)return 0;memcpy(standard_graphics,graphics,146939);memcpy(standard_palette,palette,sizeof(palette));
    data = read_resource("simtitle.ppf", &size);
    if (!data) return 0;
    good = sim_picture_decode(data, size, title);
    free(data);
    if (!good) return 0;
    data = read_resource("simscen.ppf", &size);
    if (!data) return 0;
    good = sim_picture_decode(data, size, scenario);
    free(data);
    if (!good) return 0;
    data = read_resource("cursor.dat", &size);
    if (!data) return 0;
    good = size == sizeof(cursor) && data[0] == 16 && data[1] == 0 &&
           data[2] == 16 && data[3] == 0 && data[260] == 16 && data[261] == 0 &&
           data[262] == 16 && data[263] == 0;
    if (good) memcpy(cursor, data, sizeof(cursor));
    free(data);
    if (!good) return 0;
    audio_data = read_resource("simaudio.dat", &audio_size);
    if (!audio_data) return 0;
    for (unsigned i=0; i<8; ++i) {
        sim_pcm_sound sample;
        if (!sim_audio_sound(audio_data, audio_size, i, &sample)) return 0;
    }
    code_data=read_resource("simcode.dat",&code_size);
    message_data=read_resource("messages.dat",&message_size);
    scenario_data=read_resource("scenarios.dat",&scenario_size);
    terrain_toolbar=read_resource("terrainbar.dat",&terrain_toolbar_size);
    if(!code_data||!message_data||!scenario_data||!terrain_toolbar||terrain_toolbar_size!=6578||!sim_menus_open(&game_menus,message_data,message_size))return 0;
    themes_data=read_resource("simthemes.dat",&themes_size);
    if(!themes_data||!sim_graphics_themes_open(&themes,themes_data,themes_size))return 0;
    active_messages=message_data;active_message_size=message_size;
    city_session_assets(&session,code_data,code_size,active_messages,active_message_size,icon_art);
    session.sound=session_sound;
    session.editor_open=open_editor_for_message;
    return 1;
}

static void sync_city(void)
{
    uint8_t *d=city_session_dgroup(&session);
    expand_city_layout();
    ww(d+0xc9ea,music_on);
    for(unsigned i=0;i<12000;++i)terrain.tiles[i]=(uint16_t)rw(d+0x6b78+2*i);
    for(unsigned i=0;i<5;++i)terrain.random[i]=(uint16_t)rw(d+0x3b48+i*2);
    city_funds=rw(d+0x2960)|((uint32_t)rw(d+0x2962)<<16);
    sound_on=d[0x295e]!=0;
    memcpy(city_name,d+0xcaca,20);city_name[20]=0;
    city_view.screen_x=(int16_t)rw(d+0x2992);city_view.screen_y=(int16_t)rw(d+0x2990);
    city_view.columns=rw(d+0x296e);city_view.rows=rw(d+0x296c);
    city_view.tick=(uint16_t)rw(d+0x1c28);
    city_view.power_blink=(int16_t)rw(d+0x3a6e)!=0;
    city_view.editor=d[0x32f2]!=0;
}

static void load_city(unsigned scenario_id)
{
    uint8_t *file=NULL;size_t size=0;int good=0;
    if(scenario_id){
        const uint8_t *compressed;
        if(sim_scenario_archive_find(scenario_data,scenario_size,scenario_id,&compressed,&size)&&
           sim_scenario_decode(compressed,size,save_buffer,sizeof(save_buffer)))
            good=city_session_load(&session,save_buffer,sizeof(save_buffer),scenario_id,game_clock());
    }else{
        file=read_resource(save_names[save_slot],&size);
        if(file)good=city_session_load(&session,file,size,0,game_clock());
        free(file);
    }
    if(!good){notice=scenario_id?"Unable to load scenario":"No valid saved city found";return;}
    sim_city_view_init(&city_view);city_tool=city_session_dgroup(&session)[0x2b1a];
    if(city_tool<1||city_tool>16)city_tool=SIM_CITY_ROAD;
    screen=CITY;open_menu=-1;report_window=active_report();file_mode=0;queued_place=0;city_layer_valid=0;
    memset(message_strip,8,sizeof(message_strip));message_serial=session.effects_serial;
    graphics_state();sync_city();cursor_x=188;cursor_y=111;quake_active=quake_pending=foreground_done=0;
    if(scenario_id){
        uint8_t *d=city_session_dgroup(&session);
        /* Original scenario chooser2f6e6 shows the intro directly via2f79a,
         * without posting an ordinary message or forcing AutoGoto. */
        session.modal=-(int)scenario_id;session.modal_image=(int8_t)d[0x2955];
        d[0x2955]=255;d[0x295c]=0;popup_id=0;
    }
}

static int save_city_file(void)
{
    if(!city_session_save(&session,save_buffer,sizeof(save_buffer))){notice="City is not ready to save";return 0;}
    FILE *f=fopen(save_names[save_slot],"wb");
    if(!f){notice="Unable to open city save";return 0;}
    int good=fwrite(save_buffer,1,sizeof(save_buffer),f)==sizeof(save_buffer);
    if(fclose(f))good=0;
    if(!good){notice="City save failed";return 0;}
    city_session_dgroup(&session)[0xd171]=0;
    return 1;
}

static void ask_question(int event,int kind,const char *text)
{question_event=event;question_kind=kind;question_text=text;question_row=0;}

static void answer_question(int answer)
{
    int event=question_event;
    if(answer<0)return;
    if(question_kind==1){
        if(answer==2){question_kind=-1;return;}
        if(answer==0&&!save_city_file())return; /* Original retries after a failed save. */
        if(event==5)city_session_dgroup(&session)[0xd171]=0;
    }else if(answer!=0){question_kind=-1;return;}
    question_kind=-1;question_approved=event;queued_event=event;
}

static void game_event(int event)
{
    uint8_t *d=city_session_dgroup(&session);open_menu=-1;
    int approved=question_approved==event;question_approved=-1;
    if(!approved&&(event==6||event==12)){
        ask_question(event,0,event==6?"NEW GAME: Are you sure?":"EXIT: Are you sure?");return;
    }
    if(event==5||event==6||event==8)leave_terrain_edit();
    if(!approved&&(event==5||event==8)&&d[0xd171]){
        ask_question(event,1,(const char *)d+(event==5?0x31a2:0x256a));return;
    }
    if(event==5){report_window=0;screen=SCENARIO;scenario_selected=0;sim_scenario_center(0,&cursor_x,&cursor_y);}
    else if(event==6){report_window=0;sim_new_city_init(&new_city);screen=NEW_CITY;sim_new_city_cursor(0,&cursor_x,&cursor_y);}
    else if(event==8){file_mode=1;file_selection=save_slot;}
    else if(event==9){file_mode=2;file_selection=save_slot;}
    else if(event==10){
        if(save_city_file())notice="City saved";
    }else if(event==12){screen=TITLE;report_window=file_mode=0;}
    else if(event==0)report_window=5;
    else if(event==2)system_popup=(const char *)d+0x212e;
    else if(event==3){file_mode=3;file_selection=(int)graphics_choice;}
    else if(event==16)d[0x295b]^=1;
    else if(event==17)d[0x295c]^=1;
    else if(event==18)d[0x295d]^=1;
    else if(event==19){d[0x295e]^=1;sound_on=d[0x295e]!=0;}
    else if(event==20){
        music_on=!music_on;apply_music();ww(d+0xc9ea,music_on);
    }else if(event==21){open_menu=SIM_MENU_SPEED;menu_row=sim_menus_next(&game_menus,SIM_MENU_SPEED,-1,1);}
    else if(event==22)d[0x2959]^=1;
    else if(event==23)d[0x2958]^=1;
    else if(event>=32&&event<=37)city_session_disaster(&session,(unsigned)(event-12));
    else if(event==39)d[0x3a64]^=1;
    else if(event==50)city_session_budget(&session,0);
    else if(event==52)report_window=1;
    else if(event==51){leave_terrain_edit();activate_window(1);sync_city();}
    else if(event==54){sim_windows_close_current(session.cpu.memory,SIM_DOS_MEMORY_SIZE);report_window=active_report();repaint_editor(1+(d[0x5326]==1));}
    else if(event==55){sim_windows_hide(session.cpu.memory,SIM_DOS_MEMORY_SIZE);report_window=active_report();repaint_editor(1+(d[0x5326]==1));}
    else if(event==56||event==57){
        unsigned previous=d[0x5326];
        dragging_window=sim_window_drag_begin(session.cpu.memory,SIM_DOS_MEMORY_SIZE,event==57?SIM_WINDOW_RESIZE:SIM_WINDOW_POSITION,drag_mouse_pending?drag_origin_x:cursor_x,drag_mouse_pending?drag_origin_y:cursor_y,&window_drag);
        drag_mouse=drag_mouse_pending;drag_mouse_pending=0;
        if(event==57&&previous!=1)repaint_editor(1);
        report_window=active_report();
    }
    else if(event==58){sim_window_drag_commit(session.cpu.memory,SIM_DOS_MEMORY_SIZE,&window_drag);dragging_window=0;drag_mouse=0;city_layer_valid=0;repaint_editor(window_drag.window==1?2:1);sync_city();city_view.map_x=(int16_t)rw(d+0x5f10);city_view.map_y=(int16_t)rw(d+0x5f0e);}
    else if(event>=200&&event<=203)activate_window((unsigned)(event-200));
    else if(event>=300&&event<=306){if(!choose_graphics((unsigned)(event-300)))notice="Unable to load graphics";}
    else if(event==49){activate_window(3);graph_focus=0;}
    else if(event==48){activate_window(2);map_navigation=0;map_focus=(int)rw(d+0x3268)-1;if(map_focus<0||map_focus>8)map_focus=0;}
    else if(event==53){
        if(d[0x32f2])leave_terrain_edit();
        else {activate_window(1);sim_terrain_edit_enter(session.cpu.memory,SIM_DOS_MEMORY_SIZE);edit_focus=(int)rw(d+0x32f0);}
        sync_city();animation_pending=1;
    }
    else if(event>=100&&event<=106){sim_terrain_edit_select(session.cpu.memory,SIM_DOS_MEMORY_SIZE,(unsigned)(event-100));animation_pending=1;sync_city();}
    else if(event>=80&&event<=84)ww(d+0x3a6e,(unsigned)(event-80));
    else notice="Selection unavailable";
}

static void simulation_frame(void)
{
    if(screen!=CITY||!session.active)return;
    if(session.safe&&(!foreground_done||queued_event>=0||queued_place||queued_center)&&!session.budget_open&&!session.modal&&!notice){
        uint8_t *d=city_session_dgroup(&session);
        if((int16_t)rw(d+0x5f10)!=city_view.map_x||(int16_t)rw(d+0x5f0e)!=city_view.map_y)animation_pending=1;
        ww(d+0x5f10,(unsigned)city_view.map_x);ww(d+0x5f0e,(unsigned)city_view.map_y);
        d[0x2b1a]=(uint8_t)city_tool;
        if(city_tool==12||city_tool==15)d[0x2b1b]=city_tool==15;
        if(queued_event>=0){int e=queued_event;queued_event=-1;game_event(e);}
        if(queued_center){sim_maps_center(d,65536,queued_center_x,queued_center_y);city_view.map_x=(int16_t)rw(d+0x5f10);city_view.map_y=(int16_t)rw(d+0x5f0e);queued_center=0;}
        if(queued_place){
            if(d[0x32f2]){
                int result=sim_terrain_edit_apply(session.cpu.memory,SIM_DOS_MEMORY_SIZE,queued_x,queued_y);
                if(result&SIM_EDIT_SOUND_BUILD)play_sound(SIM_SOUND_BUILD);
                if(result&SIM_EDIT_MAP_CHANGED)animation_pending=1;
            }else if(queued_tool==16){
                if(sim_query_ui_read(session.cpu.memory,SIM_DOS_MEMORY_SIZE,queued_x,queued_y,&query_info)){
                    query_row=sim_query_ui_panel_row(3,cursor_x,cursor_y);report_window=4;
                }
            }else{
                uint16_t result=city_session_place(&session,queued_x,queued_y,(unsigned)queued_tool,city_view.map_x,city_view.map_y);
                if(result==1){unsigned id=queued_tool<=4?SIM_SOUND_BULLDOZE:SIM_SOUND_BUILD;d[0xd171]=1;if(play_sound(id))ww(d+0x346c,id);animation_pending=1;}
                else{
                    /* Original2aa62 posts each refusal to the message pump;
                     * sound is suppressed only for subsequent drag attempts. */
                    ww(d+0x4f74,result);
                    if(!stroke_failed){if(play_sound(SIM_SOUND_HONK))ww(d+0x346c,SIM_SOUND_HONK);stroke_failed=1;}
                }
            }
            queued_place=0;
        }
        if(screen==CITY&&foreground_allowed()){
            if(city_session_animate(&session)&&rw(d+0x3a6e)&&d[0x2969]&&(d[0x2959]||d[0x5326]==1))animation_pending=1;
            if(rw(d+0xca10)){
                uint8_t *clock=session.cpu.memory+0x220fa;
                uint32_t now=rw(clock)|((uint32_t)rw(clock+2)<<16);
                if(!quake_active){quake_active=1;quake_start=now;}
                quake_pending=1;
                if((int32_t)now<(int32_t)quake_start||(int32_t)now>=(int32_t)(quake_start+90u)){
                    ww(d+0xca10,0);quake_active=0;
                }
            }
        }
        sync_city();foreground_done=1;return;
    }
    if(screen!=CITY||!foreground_allowed())return;
    city_session_step(&session,game_clock(),60000);
    if(session.safe)foreground_done=0;
    sync_city();
    if(message_serial!=session.effects_serial&&session.effects.viewport_changed){uint8_t *d=city_session_dgroup(&session);city_view.map_x=(int16_t)rw(d+0x5f10);city_view.map_y=(int16_t)rw(d+0x5f0e);}
}

static void modal_render(const char *text)
{
    char line[36];unsigned n=0;int y=56;
    sim_mcga_fill(canvas,12,40,307,175,7,SIM_MCGA_COPY);
    sim_ui_frame(canvas,12,40,307,175,3,0);
    if(!text)text="Original game notification";
    while(*text&&y<152){
        n=0;while(text[n]&&text[n]!='\n'&&text[n]!='\r'&&n<34)++n;
        if(n==34&&text[n]){unsigned k=n;while(k&&text[k]!=' ')--k;if(k)n=k;}
        memcpy(line,text,n);line[n]=0;sim_ui_text(canvas,font_data,24,y,line,8,7);y+=10;text+=n;
        while(*text=='\n'||*text=='\r'||*text==' ')++text;
    }
    sim_ui_text(canvas,font_data,104,160,"A / B: OK",8,7);
}

static void present(void)
{
    static int presented;
    int mode_changed=0;
    if(!video_ready)return;
    unsigned height=view_height();
    if(active_video_mode.height!=height){
        of_video_mode_t requested={320,(uint16_t)height,320,OF_VIDEO_MODE_8BIT,0};
        if(of_video_set_mode(&requested)==0){of_video_get_mode(&active_video_mode);mode_changed=1;}
    }
    uint8_t *fb=of_video_surface();
    if(!fb)return;
    memset(fb,0,(size_t)active_video_mode.stride*active_video_mode.height);
    for(unsigned y=0;y<active_video_mode.height;++y)
        memcpy(fb+y*active_video_mode.stride,canvas+(y*200/active_video_mode.height)*320,320);
    of_video_flip();
    /* Keep the working post-flip framebuffer handoff. */
    select_display(!presented||mode_changed);
    presented=1;
}
static int initialize_video(void)
{
    of_video_init();
    of_video_mode_t requested={320,(uint16_t)view_height(),320,OF_VIDEO_MODE_8BIT,0};
    if(of_video_set_mode(&requested)!=0)return 0;
    of_video_get_mode(&active_video_mode);
    if(active_video_mode.width!=320||active_video_mode.height<200||active_video_mode.stride<320||active_video_mode.color_mode!=OF_VIDEO_MODE_8BIT)return 0;
    of_video_palette_bulk(palette,256);
    select_display(1);video_ready=1;
    render(1);present();return 1;
}

static void render(int focus)
{
    const uint32_t *wanted=(screen==TITLE||screen==SCENARIO)?standard_palette:game_palette;
    if(memcmp(palette,wanted,sizeof(palette))){memcpy(palette,wanted,sizeof(palette));if(video_ready)of_video_palette_bulk(palette,256);}
    if (screen == NEW_CITY || screen == CITY) {
        sim_mcga_fill(canvas, 0, 0, 319, 199, 7, SIM_MCGA_COPY);
        if(screen==NEW_CITY){
            sim_mcga_fill(canvas, 0, 0, 319, 11, 11, SIM_MCGA_COPY);
            sim_ui_text(canvas, font_data, 8, 2, "SYSTEM OPTIONS DISASTERS WINDOWS", 1, 11);
        }
        if (screen == NEW_CITY) sim_new_city_render(canvas, font_data, &new_city);
        if (screen == NEW_CITY && editing_name)
            sim_virtual_keyboard_render(canvas,font_data,&name_keyboard,&name_edit,(of_time_ms()/250)%2==0);
        if (screen == CITY) {
            sim_city_tool_info info;
            if(!sim_city_tool_info_get(city_tool, &info))info=(sim_city_tool_info){1,0,0,0,"Query"};
            int terrain_mode=session.active&&city_session_dgroup(&session)[0x32f2];
            int repaint_city=!session.active||!city_layer_valid||(city_session_dgroup(&session)[0x2969]&&(city_session_dgroup(&session)[0x2959]||city_session_dgroup(&session)[0x5326]==1));
            if(repaint_city){
            if(session.active)sim_editor_shell_render(canvas,&graphics_pack,session.cpu.memory,SIM_DOS_MEMORY_SIZE);
            else sim_city_shell_render(canvas,&graphics_pack,new_city.name,city_funds,"Jan 1900",info.label);
            if(terrain_mode)sim_terrain_edit_shell_render(canvas,&graphics_pack,terrain_toolbar,terrain_toolbar_size,city_session_dgroup(&session),65536,city_name);
            else if(!session.active)sim_city_tool_select(canvas,city_tool==15?11:(int)city_tool-1);
            if(session.active){uint8_t *d=city_session_dgroup(&session);
                for(unsigned i=0;i<12000;++i)terrain.tiles[i]=(uint16_t)rw(d+0x6b78+2*i);
            }
            if(quake_pending&&session.safe){
                static const int scroll[4]={-1,1,-1,1};
                uint8_t *d=city_session_dgroup(&session);
                for(unsigned i=0;i<4;++i){
                    city_view.map_x+=scroll[i];
                    if(city_view.map_x<0)city_view.map_x=0;
                    if(city_view.map_x>120-(int)city_view.columns)city_view.map_x=120-(int)city_view.columns;
                    ww(d+0x5f10,(unsigned)city_view.map_x);
                    sim_city_view_render(canvas,tile_data,terrain.tiles,&city_view);
                    sim_sprites_render(canvas,&graphics_pack,d,65536);present();
                }
                quake_pending=0;
            }else
            sim_city_view_render(canvas, tile_data, terrain.tiles, &city_view);
            if(session.active){
                uint8_t *d=city_session_dgroup(&session);
                if(animation_pending&&session.safe&&!session.budget_open&&!session.modal&&!session.failed&&foreground_allowed())
                    for(unsigned i=0;i<12000;++i)ww(d+0x6b78+2*i,terrain.tiles[i]);
                animation_pending=0;
                if(!terrain_mode){
                    sim_sprites_render(canvas,&graphics_pack,d,65536);
                    int left=(int16_t)rw(d+0x298a),right=(int16_t)rw(d+0x29be),top=(int16_t)rw(d+0x2988)+1;
                    if(left>=0&&right<320&&left<=right&&top>=0&&top+8<=200)
                        for(unsigned y=0;y<8;++y)memcpy(canvas+(top+y)*320+left,message_strip+y*320,(size_t)(right-left+1));
                }
                if(message_serial!=session.effects_serial){
                    SimMessageEffects *e=&session.effects;
                    if(e->clear)sim_mcga_fill(canvas,e->x0,e->y0,e->x1,e->y1,8,SIM_MCGA_COPY);
                    if(e->icon)sim_mcga_blit(canvas,e->icon_x,e->icon_y,icon_art+4,64,8,8,0,0,SIM_MCGA_COPY);
                    for(int i=0;i<e->text_count;++i)sim_ui_text(canvas,font_data,e->text[i].x,e->text[i].y,e->text[i].text,7,8);
                    int left=(int16_t)rw(d+0x298a),right=(int16_t)rw(d+0x29be),top=(int16_t)rw(d+0x2988)+1;
                    if(left>=0&&right<320&&left<=right&&top>=0&&top+8<=200)
                        for(unsigned y=0;y<8;++y)memcpy(message_strip+y*320,canvas+(top+y)*320+left,(size_t)(right-left+1));
                    message_serial=session.effects_serial;
                    if(e->alert3)play_sound(SIM_SOUND_POLICE);
                }
            }
            memcpy(city_layer,canvas,sizeof(city_layer));city_layer_valid=1;
            }else memcpy(canvas,city_layer,sizeof(city_layer));
            if(session.active){
                uint8_t *d=city_session_dgroup(&session);
                unsigned order[3];int count=sim_windows_draw_order(session.cpu.memory,SIM_DOS_MEMORY_SIZE,order);
                sim_mcga_fill(canvas,0,12,319,199,7,SIM_MCGA_COPY);
                for(int i=0;i<count;++i){
                    if(order[i]==1){SimWindowRect rect;
                        if(sim_window_rectangle(session.cpu.memory,SIM_DOS_MEMORY_SIZE,1,0,&rect)&&rect.left>=0&&rect.right<320&&rect.top>=0&&rect.bottom<200)
                            for(int y=rect.top;y<=rect.bottom;++y)memcpy(canvas+y*320+rect.left,city_layer+y*320+rect.left,(size_t)(rect.right-rect.left+1));
                    }else if(order[i]==2){sim_maps_render(canvas,&graphics_pack,session.cpu.memory,SIM_DOS_MEMORY_SIZE);sim_maps_viewport_render(canvas,d,65536);}
                    else if(order[i]==3)sim_graphs_ui_render(canvas,font_data,session.cpu.memory,SIM_DOS_MEMORY_SIZE,graph_art,graph_art_size);
                }
                if(session.budget_open)sim_budget_ui_render(canvas,font_data,session.cpu.memory,SIM_DOS_MEMORY_SIZE);
                if(report_window==1)sim_evaluation_ui_render(canvas,font_data,session.cpu.memory,SIM_DOS_MEMORY_SIZE);
                if(report_window==4)sim_query_ui_render(canvas,font_data,session.cpu.memory,SIM_DOS_MEMORY_SIZE,&query_info,query_row);
                if(report_window==5)sim_about_ui_render(canvas,font_data,session.cpu.memory,SIM_DOS_MEMORY_SIZE);
                if(dragging_window)sim_window_preview_render(canvas,&window_drag);
                if(open_menu>=0){sim_menu_flags flags;sim_menus_flags_dgroup(&flags,d,65536);
                    sim_menus_render(canvas,font_data,&game_menus,(sim_menu_kind)open_menu,menu_row,&flags);}
                if(session.modal){
                    const char *text=sim_message_modal_text(session.cpu.memory,SIM_DOS_MEMORY_SIZE,session.modal);
                    if(text){
                        uint8_t saved_image=d[0x2955];d[0x2955]=(uint8_t)session.modal_image;
                        sim_message_popup_render(canvas,font_data,session.cpu.memory,SIM_DOS_MEMORY_SIZE,text,1,popup_art,popup_art_size);
                        d[0x2955]=saved_image;
                    }
                    else modal_render("Original game notification");
                }
                if(session.failed){char error[80];snprintf(error,sizeof(error),"Simulation stopped at %05lx",(unsigned long)session.stopped_pc);modal_render(error);}
            }
        }
    } else {
        sim_mcga_blit(canvas, 0, 0, screen == SCENARIO ? scenario : title,
                      64000, 320, 200, 0, 0, SIM_MCGA_COPY);
    }
    if(screen==CITY&&city_menu_visible()){
        sim_mcga_fill(canvas,0,0,319,11,11,SIM_MCGA_COPY);
        sim_ui_text(canvas,font_data,8,2,"SYSTEM OPTIONS DISASTERS WINDOWS",1,11);
    }
    if(notice)modal_render(notice);
    if(system_popup){
        uint8_t *d=city_session_dgroup(&session);uint8_t old=d[0x2955];d[0x2955]=255;
        sim_message_popup_render(canvas,font_data,session.cpu.memory,SIM_DOS_MEMORY_SIZE,system_popup,0,NULL,0);d[0x2955]=old;
    }
    if(file_mode){
        sim_mcga_fill(canvas,64,32,255,176,7,SIM_MCGA_COPY);sim_ui_frame(canvas,64,32,255,176,3,0);
        sim_ui_text(canvas,font_data,80,44,file_mode==1?"Load a City":file_mode==2?"Save City As":"Load Graphics",8,7);
        int count=file_mode==3?(int)themes.count+1:6;
        for(int i=0;i<count;++i){char line[24];
            if(file_mode==3){sim_graphics_theme_asset entry;const char *label="Standard";if(i&&sim_graphics_themes_entry(&themes,(unsigned)i-1,&entry))label=entry.title;snprintf(line,sizeof(line),"%c %.18s",i==file_selection?'>':' ',label);}
            else snprintf(line,sizeof(line),"%c City slot %d",i==file_selection?'>':' ',i+1);
            sim_ui_text(canvas,font_data,80,64+14*i,line,i==file_selection?7:8,i==file_selection?8:7);}
        sim_ui_text(canvas,font_data,80,160,"A: OK  B: Cancel",8,7);
    }
    if(question_kind>=0)sim_question_render(canvas,font_data,question_text,question_kind);
    if(question_kind>=0&&notice)modal_render(notice);
    if(screen==CITY&&session.active&&report_window==0&&foreground_allowed()&&
       !session.budget_open&&!session.modal&&!session.failed&&!quake_active&&!ui_click_consumed&&
       city_session_dgroup(&session)[0x5326]==1&&!city_session_dgroup(&session)[0x32f2]&&placement_preview_enabled()){
        int x=(cursor_x-city_view.screen_x)/8,y=(cursor_y-city_view.screen_y)/8;
        uint16_t tile=0;
        if(cursor_x>=city_view.screen_x&&cursor_y>=city_view.screen_y&&x<(int)city_view.columns&&y<(int)city_view.rows){
            int mx=x+city_view.map_x,my=y+city_view.map_y;
            if(mx>=0&&mx<120&&my>=0&&my<100)tile=terrain.tiles[mx*100+my];
        }
        sim_placement_preview_render(canvas,&city_view,cursor_x,cursor_y,city_tool,tile);
    }
    if (focus && (!editing_name||keyboard_mouse_active)&&!quake_active) {
        unsigned right = cursor_x > 304 ? (unsigned)(cursor_x - 304) : 0;
        unsigned height = cursor_y > 184 ? (unsigned)(200 - cursor_y) : 16;
        /* Original 0x229d4: first cursor mask AND, second XOR. Full
         * redraw replaces DOS's capture/restore of the previous background. */
        sim_mcga_mask(canvas, cursor_x, cursor_y, cursor+4, 256,
                      16, height, 0, right, 255, 0, SIM_MCGA_AND);
        sim_mcga_mask(canvas, cursor_x, cursor_y, cursor+264, 256,
                      16, height, 0, right, 255, 0, SIM_MCGA_XOR);
    }
}

static void selected(int choice)
{
    if (!choice) return;
    if(screen==TITLE&&choice>=1&&choice<=3)leave_terrain_edit();
    if (screen == TITLE && choice == 1) {
        sim_new_city_init(&new_city);
        screen = NEW_CITY;
        sim_new_city_cursor(0, &cursor_x, &cursor_y);
        return;
    }
    if (!screen && choice == 3) {
        screen = 1; scenario_selected = 0;
        sim_scenario_center(0, &cursor_x, &cursor_y);
        return;
    }
    if(screen==SCENARIO){load_city((unsigned)choice);return;}
    if(screen==TITLE&&choice==2){file_mode=1;file_selection=save_slot;return;}
    notice="Selection unavailable";
}

static void move_selection(int direction)
{
    if (screen == NEW_CITY) {
        sim_new_city_step(&new_city, direction);
        sim_new_city_cursor(new_city.focus, &cursor_x, &cursor_y);
    } else if (screen == SCENARIO) {
        scenario_selected = (scenario_selected + direction + 8) % 8;
        sim_scenario_center(scenario_selected, &cursor_x, &cursor_y);
    } else {
        sim_title_rect r;
        sim_title_menu_step(&menu, direction);
        sim_title_rect_get(menu.selected, &r);
        cursor_x = (r.x0 + r.x1) / 2; cursor_y = (r.y0 + r.y1) / 2;
    }
}

static void frame_input_impl(void)
{
    of_keyboard_state_t kb;
    of_mouse_state_t mouse;
    of_input_poll();
    of_input_keyboard_state(&kb);
    of_input_mouse_state(&mouse);
    controller_consumed=0;
    int pan_back=0;
    if(screen==CITY){
        uint32_t directions=OF_BTN_LEFT|OF_BTN_RIGHT|OF_BTN_UP|OF_BTN_DOWN;
        int can_pan=session.active&&report_window==0&&!file_mode&&!notice&&!session.modal&&!session.budget_open&&open_menu<0&&question_kind<0&&!dragging_window;
        if(!can_pan)memset(edge_scroll,0,sizeof(edge_scroll));
        int pan=can_pan&&of_btn(OF_BTN_B)&&of_btn(directions);
        int b_down=of_btn(OF_BTN_B)!=0;
        if(can_pan){
            if(b_down&&!pan_down)pan_pending=1;
            if(pan)pan_pending=0;
            if(!b_down&&pan_down){pan_back=pan_pending;pan_pending=0;}
            controller_consumed|=OF_BTN_B;
        }else pan_pending=0;
        pan_down=b_down;
        if(!dragging_window){
            int dx=0,dy=0;
            dx-=pan?map_pan_button(OF_BTN_LEFT,0):pointer_button(OF_BTN_LEFT,0);
            dx+=pan?map_pan_button(OF_BTN_RIGHT,1):pointer_button(OF_BTN_RIGHT,1);
            dy-=pan?map_pan_button(OF_BTN_UP,2):pointer_button(OF_BTN_UP,2);
            dy+=pan?map_pan_button(OF_BTN_DOWN,3):pointer_button(OF_BTN_DOWN,3);
            if(pan){
                memset(edge_scroll,0,sizeof(edge_scroll));
                for(unsigned i=0;i<4;++i)pointer_button(0,i);
                city_view.map_x+=dx;city_view.map_y+=dy;
                int max_x=120-(int)city_view.columns,max_y=100-(int)city_view.rows;
                if(city_view.map_x<0)city_view.map_x=0;
                if(city_view.map_x>max_x)city_view.map_x=max_x;
                if(city_view.map_y<0)city_view.map_y=0;
                if(city_view.map_y>max_y)city_view.map_y=max_y;
            }else{
                for(unsigned i=0;i<4;++i)map_pan_button(0,i);
                int automatic=can_pan&&!system_popup&&!session.failed&&!of_btn(OF_BTN_A|OF_BTN_B)&&
                    !((mouse.buttons|mouse.buttons_pressed)&3)&&!mouse.dx&&!mouse.dy;
                int left=city_view.screen_x,top=city_view.screen_y;
                int right=left+(int)city_view.columns*8-1,bottom=top+(int)city_view.rows*8-1;
                dx=edge_pointer_delta(0,cursor_x,dx,left,right,cursor_y,top,bottom,
                    OF_BTN_LEFT,OF_BTN_RIGHT,automatic,&city_view.map_x,120-(int)city_view.columns);
                dy=edge_pointer_delta(1,cursor_y,dy,top,bottom,cursor_x,left,right,
                    OF_BTN_UP,OF_BTN_DOWN,automatic,&city_view.map_y,100-(int)city_view.rows);
                mouse.dx+=dx;mouse.dy+=dy;
            }
            controller_consumed|=directions|OF_BTN_A;
        }
        mouse.present=1;
        if(of_btn_pressed(OF_BTN_A))mouse.buttons_pressed|=1;
        if(of_btn(OF_BTN_A))mouse.buttons|=1;
    }else{pan_down=pan_pending=0;memset(edge_scroll,0,sizeof(edge_scroll));}
    /* A UI click owns the entire press, including held frames after the UI
     * closes. Only a release can arm a subsequent construction stroke. */
    int click_down=of_btn(OF_BTN_A)||of_btn_pressed(OF_BTN_A)||
        ((mouse.buttons|mouse.buttons_pressed)&1);
    if(!click_down)ui_click_consumed=0;
    if(screen==CITY&&click_down&&(open_menu>=0||file_mode||notice||system_popup||
       question_kind>=0||session.modal||session.budget_open||dragging_window||
       (report_window&&report_window!=4)))ui_click_consumed=1;
    int pointer_moved=mouse.present&&(mouse.dx||mouse.dy);
    if(mouse.present){cursor_x+=mouse.dx;cursor_y+=mouse.dy;mouse.dx=mouse.dy=0;
        if(cursor_x<0)cursor_x=0;
        if(cursor_x>319)cursor_x=319;
        if(cursor_y<0)cursor_y=0;
        if(cursor_y>199)cursor_y=199;}
    if(screen==CITY&&cursor_y<12){menu_revealed=1;menu_reveal_time=of_time_ms();}
    if(screen==CITY&&!notice&&!file_mode&&!session.modal&&!session.budget_open&&
       question_kind<0&&!system_popup&&!dragging_window&&controller_pressed(OF_BTN_SELECT)){
        menu_revealed=1;menu_reveal_time=of_time_ms();
        open_menu=SIM_MENU_SYSTEM;menu_row=sim_menus_next(&game_menus,SIM_MENU_SYSTEM,-1,1);
        sim_menu_layout layout;
        if(sim_menus_layout(&game_menus,SIM_MENU_SYSTEM,&layout)&&menu_row>=0){
            cursor_x=layout.text_x+layout.width_chars*4;
            cursor_y=layout.text_y+menu_row*8+4;
        }
        for(unsigned i=0;i<4;++i)pointer_button(0,i);
        memset(edge_scroll,0,sizeof(edge_scroll));return;
    }
    if(mouse.buttons_pressed&1){mouse_press_x=cursor_x;mouse_press_y=cursor_y;}
    int back = pan_back || (editing_name ? editor_pad_pressed(OF_BTN_B, &kb) : controller_pressed(OF_BTN_B)) || key_pressed(&kb, 0x29);
    if(system_popup){if(back||controller_pressed(OF_BTN_A|OF_BTN_START)||key_pressed(&kb,0x28)||key_pressed(&kb,0x06)||(mouse.buttons_pressed&1))system_popup=NULL;return;}
    if(question_kind>=0){
        if(notice){if(back||controller_pressed(OF_BTN_A|OF_BTN_START)||key_pressed(&kb,0x28)||(mouse.buttons_pressed&1))notice=NULL;return;}
        SimQuestionLayout layout;int answer=-1;
        sim_question_layout(question_text,question_kind,&layout);
        if(controller_pressed(OF_BTN_LEFT|OF_BTN_UP))question_row=(question_row+layout.count-1)%layout.count;
        if(controller_pressed(OF_BTN_RIGHT|OF_BTN_DOWN))question_row=(question_row+1)%layout.count;
        if(controller_pressed(OF_BTN_A|OF_BTN_START))answer=question_row;
        if(back)answer=sim_question_key(question_kind,27);
        if(key_pressed(&kb,0x28))answer=sim_question_key(question_kind,13);
        if(key_pressed(&kb,0x1c))answer=sim_question_key(question_kind,'Y');
        if(key_pressed(&kb,0x11))answer=sim_question_key(question_kind,'N');
        if(key_pressed(&kb,0x06))answer=sim_question_key(question_kind,'C');
        if(mouse.buttons_pressed&1)answer=sim_question_hit(&layout,cursor_x,cursor_y);
        else if(!mouse.present){cursor_x=(layout.button[question_row].x0+layout.button[question_row].x1)/2;cursor_y=(layout.button[question_row].y0+layout.button[question_row].y1)/2;}
        answer_question(answer);return;
    }
    if(screen==CITY&&!notice&&!session.modal&&!session.budget_open&&!file_mode&&
       (kb.modifiers&(OF_KEYMOD_LCTRL|OF_KEYMOD_RCTRL))){
        static const unsigned usage[]={0x0f,0x16,0x1b,0x04,0x10,0x0a,0x05,0x08,0x18,0x17,0x06,0x0b,0x13,0x15};
        static const int event[]={8,10,12,16,48,49,50,51,52,53,54,55,56,57};
        for(unsigned i=0;i<sizeof(event)/sizeof(event[0]);++i)
            if(key_pressed(&kb,usage[i])){queued_event=event[i];return;}
    }
    if(screen==CITY&&!file_mode&&!notice&&!session.modal&&!session.budget_open&&!modal_report()&&open_menu<0&&!dragging_window){
        if(mouse.present&&((mouse.buttons|mouse.buttons_pressed)&1)){
            int bar=sim_menus_bar_hit(cursor_x,cursor_y);
            if(bar>=0&&(mouse.buttons_pressed&1)){open_menu=bar;menu_row=sim_menus_next(&game_menus,(sim_menu_kind)bar,-1,1);return;}
            int hit=sim_windows_hit(session.cpu.memory,SIM_DOS_MEMORY_SIZE,cursor_x,cursor_y);
            if(hit&&hit!=city_session_dgroup(&session)[0x5326]){ui_click_consumed=1;if(mouse.buttons_pressed&1)queued_event=200+hit;return;}
            SimWindowRect close;
            if(hit&&(mouse.buttons_pressed&1)&&sim_window_rectangle(session.cpu.memory,SIM_DOS_MEMORY_SIZE,(unsigned)hit,2,&close)&&cursor_x>=close.left&&cursor_x<=close.right&&cursor_y>=close.top&&cursor_y<=close.bottom){ui_click_consumed=1;queued_event=54;return;}
            SimWindowRect caption;
            if(hit&&sim_window_rectangle(session.cpu.memory,SIM_DOS_MEMORY_SIZE,(unsigned)hit,1,&caption)&&mouse_press_x>=caption.left&&mouse_press_x<=caption.right&&mouse_press_y>=caption.top&&mouse_press_y<=caption.bottom&&((mouse.buttons_pressed&1)||pointer_moved)){
                drag_mouse_pending=1;drag_origin_x=mouse_press_x;drag_origin_y=mouse_press_y;queued_event=56;return;
            }
        }
        if((report_window==2&&controller_pressed(OF_BTN_START))||(report_window==3&&controller_pressed(OF_BTN_START)&&controller_held(OF_BTN_SELECT))){open_menu=SIM_MENU_WINDOWS;menu_row=sim_menus_next(&game_menus,SIM_MENU_WINDOWS,-1,1);return;}
    }
    if(dragging_window){
        if(back){queued_event=58;return;}
        if(pan_button(OF_BTN_LEFT,0))cursor_x-=16;
        if(pan_button(OF_BTN_RIGHT,1))cursor_x+=16;
        if(pan_button(OF_BTN_UP,2))cursor_y-=16;
        if(pan_button(OF_BTN_DOWN,3))cursor_y+=16;
        sim_window_drag_update(session.cpu.memory,SIM_DOS_MEMORY_SIZE,cursor_x,cursor_y,&window_drag);
        if(controller_pressed(OF_BTN_A|OF_BTN_START)||key_pressed(&kb,0x28)||(drag_mouse?!(mouse.buttons&1):(mouse.buttons_pressed&1)))queued_event=58;
        return;
    }
    if(file_mode){
        if(back){file_mode=0;return;}
        int count=file_mode==3?(int)themes.count+1:6;
        if(controller_pressed(OF_BTN_UP|OF_BTN_LEFT))file_selection=(file_selection+count-1)%count;
        if(controller_pressed(OF_BTN_DOWN|OF_BTN_RIGHT))file_selection=(file_selection+1)%count;
        int accept=controller_pressed(OF_BTN_A|OF_BTN_START)||key_pressed(&kb,0x28);
        if(mouse.present){cursor_x+=mouse.dx;cursor_y+=mouse.dy;
            if(cursor_x>=72&&cursor_x<=248&&cursor_y>=64&&cursor_y<64+14*count){file_selection=(cursor_y-64)/14;if(mouse.buttons_pressed&1)accept=1;}}
        if(accept){int mode=file_mode;file_mode=0;
            if(mode==3)queued_event=300+file_selection;
            else {save_slot=file_selection;if(mode==1)load_city(0);else queued_event=10;}}
        return;
    }
    if(session.modal){
        uint32_t now=game_clock();
        if(popup_id!=session.modal){popup_id=session.modal;popup_clock=now;}
        if((uint32_t)(now-popup_clock)>=540){city_session_acknowledge(&session);popup_id=0;return;}
    }
    if(screen==CITY&&report_window==4&&!notice&&!session.modal){
        if(controller_pressed(OF_BTN_X))city_tool=city_tool==16?1:city_tool+1;
        if(controller_pressed(OF_BTN_Y))city_tool=city_tool==1?16:city_tool-1;
        int px=cursor_x-city_view.screen_x,py=cursor_y-city_view.screen_y;
        if(back||!(mouse.buttons&1)||city_tool!=16||px<0||py<0||px>=(int)city_view.columns*8||py>=(int)city_view.rows*8){
            report_window=active_report();city_layer_valid=0;return;
        }
        sim_query_ui_read(session.cpu.memory,SIM_DOS_MEMORY_SIZE,city_view.map_x+px/8,city_view.map_y+py/8,&query_info);
        query_row=sim_query_ui_panel_row(query_row,cursor_x,cursor_y);
        return;
    }
    if(notice||session.modal||modal_report()){
        int acknowledge=back||controller_pressed(OF_BTN_A|OF_BTN_START)||key_pressed(&kb,0x28)||(mouse.buttons_pressed&1);
        if(session.modal&&!notice){
            acknowledge=controller_pressed(OF_BTN_A|OF_BTN_B|OF_BTN_START)||key_pressed(&kb,0x28)||key_pressed(&kb,0x06);
            if(mouse.buttons_pressed&1){
                SimMessagePopupLayout layout;uint8_t *d=city_session_dgroup(&session);uint8_t image=d[0x2955];
                const char *text=sim_message_modal_text(session.cpu.memory,SIM_DOS_MEMORY_SIZE,session.modal);
                d[0x2955]=(uint8_t)session.modal_image;
                if(text&&sim_message_popup_layout(session.cpu.memory,SIM_DOS_MEMORY_SIZE,text,1,&layout)&&
                   cursor_x>=layout.button_x0&&cursor_x<=layout.button_x1&&cursor_y>=layout.button_y0&&cursor_y<=layout.button_y1)acknowledge=1;
                d[0x2955]=image;
            }
        }
        if(acknowledge){
            notice=NULL;city_session_acknowledge(&session);popup_id=0;report_window=active_report();
        }return;
    }
    if(screen==CITY&&session.budget_open){
        if(back){city_session_budget_commit(&session);return;}
        if(controller_pressed(OF_BTN_RIGHT|OF_BTN_DOWN))budget_focus=(budget_focus+1)%9;
        if(controller_pressed(OF_BTN_LEFT|OF_BTN_UP))budget_focus=(budget_focus+8)%9;
        int event=0;
        if(controller_pressed(OF_BTN_A|OF_BTN_START)||key_pressed(&kb,0x28))event=0x800+budget_focus;
        if(mouse.present){cursor_x+=mouse.dx;cursor_y+=mouse.dy;if(mouse.buttons_pressed&1)event=sim_budget_ui_hit(cursor_x,cursor_y);}
        else sim_budget_ui_cursor((unsigned)budget_focus,&cursor_x,&cursor_y);
        if(event==0x808)city_session_budget_commit(&session);
        else if(event)sim_budget_ui_adjust(session.cpu.memory,SIM_DOS_MEMORY_SIZE,(unsigned)event,controller_held(OF_BTN_SELECT)?10:1);
        return;
    }
    if(screen==CITY&&open_menu>=0){
        if(back){open_menu=-1;return;}
        if(controller_pressed(OF_BTN_DOWN))menu_row=sim_menus_next(&game_menus,(sim_menu_kind)open_menu,menu_row,1);
        if(controller_pressed(OF_BTN_UP))menu_row=sim_menus_next(&game_menus,(sim_menu_kind)open_menu,menu_row,-1);
        if(open_menu<4&&controller_pressed(OF_BTN_RIGHT|OF_BTN_LEFT)){
            open_menu=(open_menu+(controller_pressed(OF_BTN_RIGHT)?1:3))%4;
            menu_row=sim_menus_next(&game_menus,(sim_menu_kind)open_menu,-1,1);
        }
        int activate=controller_pressed(OF_BTN_A|OF_BTN_START)||key_pressed(&kb,0x28);
        if(mouse.present){
            cursor_x+=mouse.dx;cursor_y+=mouse.dy;
            int bar=sim_menus_bar_hit(cursor_x,cursor_y);
            if(bar>=0&&bar<4){open_menu=bar;menu_row=sim_menus_next(&game_menus,(sim_menu_kind)bar,-1,1);}
            else if(pointer_moved)menu_row=sim_menus_hit(&game_menus,(sim_menu_kind)open_menu,cursor_x,cursor_y);
            if(mouse.buttons_pressed&1)activate=1;
        }
        if(activate&&menu_row>=0){
            int e=sim_menus_event((sim_menu_kind)open_menu,menu_row);
            queued_event=open_menu==SIM_MENU_SPEED?80+e:e;open_menu=-1;
        }return;
    }
    if(screen==CITY&&report_window==3){
        uint8_t *d=city_session_dgroup(&session);
        if(back){queued_event=201;return;}
        if(controller_pressed(OF_BTN_START)){open_menu=SIM_MENU_WINDOWS;menu_row=sim_menus_next(&game_menus,SIM_MENU_WINDOWS,-1,1);return;}
        if(map_navigation){
            if(pan_button(OF_BTN_LEFT,0)&&map_pointer_x>0)--map_pointer_x;
            if(pan_button(OF_BTN_RIGHT,1)&&map_pointer_x<119)++map_pointer_x;
            if(pan_button(OF_BTN_UP,2)&&map_pointer_y>0)--map_pointer_y;
            if(pan_button(OF_BTN_DOWN,3)&&map_pointer_y<99)++map_pointer_y;
            if(controller_pressed(OF_BTN_A)||key_pressed(&kb,0x28)){
                queued_center=1;queued_center_x=map_pointer_x;queued_center_y=map_pointer_y;
            }
        }else{
            if(controller_pressed(OF_BTN_RIGHT|OF_BTN_DOWN))map_focus=(map_focus+1)%9;
            if(controller_pressed(OF_BTN_LEFT|OF_BTN_UP))map_focus=(map_focus+8)%9;
            if(controller_pressed(OF_BTN_A)||key_pressed(&kb,0x28))ww(d+0x3268,(unsigned)map_focus+1);
        }
        if(controller_pressed(OF_BTN_X|OF_BTN_Y)){
            unsigned offset=map_focus==3?0x326c:0x326a;
            if(map_focus==3||map_focus==8){ww(d+0x3268,(unsigned)map_focus+1);ww(d+offset,rw(d+offset)==1?2:1);}
        }
        if(mouse.present){cursor_x+=mouse.dx;cursor_y+=mouse.dy;
            if(mouse.buttons_pressed&1){int event=sim_maps_hit(d,65536,cursor_x,cursor_y);
                if(event){map_focus=event-0x411;ww(d+0x3268,(unsigned)map_focus+1);}
                else if(cursor_x>=(int16_t)rw(d+0x2a8a)&&cursor_x<(int16_t)rw(d+0x2a8a)+120&&cursor_y>=(int16_t)rw(d+0x2a88)&&cursor_y<(int16_t)rw(d+0x2a88)+100){
                    queued_center=1;queued_center_x=cursor_x-(int16_t)rw(d+0x2a8a);queued_center_y=cursor_y-(int16_t)rw(d+0x2a88);
                }
            }
        }
        else if(map_navigation){cursor_x=(int16_t)rw(d+0x2a8a)+map_pointer_x;cursor_y=(int16_t)rw(d+0x2a88)+map_pointer_y;}
        else {cursor_x=(int16_t)rw(d+0x2a92)+(map_focus&1)*27+15;cursor_y=(int16_t)rw(d+0x2a90)+(map_focus/2)*19+11;}
        return;
    }
    if(screen==CITY&&report_window==2){
        if(back){queued_event=201;return;}
        if(controller_pressed(OF_BTN_RIGHT|OF_BTN_DOWN))graph_focus=(graph_focus+1)%8;
        if(controller_pressed(OF_BTN_LEFT|OF_BTN_UP))graph_focus=(graph_focus+7)%8;
        int hit=-1;
        if(controller_pressed(OF_BTN_A|OF_BTN_START)||key_pressed(&kb,0x28))hit=graph_focus;
        if(mouse.present){cursor_x+=mouse.dx;cursor_y+=mouse.dy;if(mouse.buttons_pressed&1)hit=sim_graphs_ui_hit(session.cpu.memory,SIM_DOS_MEMORY_SIZE,cursor_x,cursor_y);}
        else sim_graphs_ui_cursor(session.cpu.memory,SIM_DOS_MEMORY_SIZE,(unsigned)graph_focus,&cursor_x,&cursor_y);
        if(hit>=0)sim_graphs_ui_activate(session.cpu.memory,SIM_DOS_MEMORY_SIZE,(unsigned)hit);
        return;
    }
    if (back) {
        if(screen==CITY&&session.active&&city_session_dgroup(&session)[0x32f2]){queued_event=53;return;}
        if(screen==CITY){open_menu=SIM_MENU_SYSTEM;menu_row=sim_menus_next(&game_menus,SIM_MENU_SYSTEM,-1,1);return;}
        if (screen == NEW_CITY && editing_name) { name_keyboard_action(SIM_VK_CANCEL); return; }
        if(screen==TITLE&&session.active){screen=CITY;report_window=active_report();return;}
        screen = TITLE;
        if (menu.selected >= 0) move_selection(0);
        return;
    }
    if (screen == NEW_CITY) {
        if (editing_name) {
            if(of_btn(OF_BTN_LEFT|OF_BTN_RIGHT|OF_BTN_UP|OF_BTN_DOWN))keyboard_mouse_active=0;
            if(pan_button(OF_BTN_LEFT,0))sim_virtual_keyboard_step(&name_keyboard,-1,0);
            if(pan_button(OF_BTN_RIGHT,1))sim_virtual_keyboard_step(&name_keyboard,1,0);
            if(pan_button(OF_BTN_UP,2))sim_virtual_keyboard_step(&name_keyboard,0,-1);
            if(pan_button(OF_BTN_DOWN,3))sim_virtual_keyboard_step(&name_keyboard,0,1);
            if(editor_pad_pressed(OF_BTN_L1,&kb))edit_key(0x84b);
            if(editor_pad_pressed(OF_BTN_R1,&kb))edit_key(0x84d);
            if(editor_pad_pressed(OF_BTN_X,&kb))edit_key(8);
            if(editor_pad_pressed(OF_BTN_Y,&kb))name_keyboard.lowercase^=1;
            if(editor_pad_pressed(OF_BTN_A,&kb))name_keyboard_action(sim_virtual_keyboard_selected(&name_keyboard));
            if(editor_pad_pressed(OF_BTN_START,&kb)||key_pressed(&kb,0x28))name_keyboard_action(SIM_VK_DONE);
            if(mouse.present&&(pointer_moved||(mouse.buttons_pressed&1))){
                keyboard_mouse_active=1;
                if(sim_virtual_keyboard_hit(&name_keyboard,cursor_x,cursor_y)&&(mouse.buttons_pressed&1))
                    name_keyboard_action(sim_virtual_keyboard_selected(&name_keyboard));
            }
            for (unsigned usage = 4; usage < 30; ++usage) if (key_pressed(&kb, usage)) {
                unsigned character = 'a'+usage-4;
                if (kb.modifiers & (OF_KEYMOD_LSHIFT | OF_KEYMOD_RSHIFT)) character -= 'a'-'A';
                edit_key(character);
            }
            for (unsigned usage = 0x1e; usage <= 0x27; ++usage)
                if (key_pressed(&kb, usage)) edit_key(usage == 0x27 ? '0' : '1'+usage-0x1e);
            static const unsigned editor_usages[] = {0x2a,0x2c,0x50,0x4f,0x4a,0x4d,0x4c,0x49};
            static const unsigned editor_keys[] = {8,32,0x84b,0x84d,2,5,4,9};
            for (unsigned i=0;i<sizeof(editor_keys)/sizeof(editor_keys[0]);++i)
                if (key_pressed(&kb,editor_usages[i])) edit_key(editor_keys[i]);
            return;
        }
        if (controller_pressed(OF_BTN_RIGHT | OF_BTN_DOWN)) move_selection(1);
        if (controller_pressed(OF_BTN_LEFT | OF_BTN_UP)) move_selection(-1);
        static const unsigned usages[] = {0x08,0x10,0x0b,0x12,0x28,0x2c,0x2e,0x2d};
        static const unsigned ascii[] = {'E','M','H','O',13,' ','+','-'};
        for (unsigned i = 0; i < sizeof(usages)/sizeof(usages[0]); ++i)
            if (key_pressed(&kb, usages[i])) new_city_action(sim_new_city_key(&new_city, ascii[i]));
        if (controller_pressed(OF_BTN_A | OF_BTN_START)) new_city_action(sim_new_city_activate(&new_city));
        if (mouse.present) {
            cursor_x += mouse.dx; cursor_y += mouse.dy;
            if (cursor_x < 0) cursor_x = 0;
            if (cursor_x > 319) cursor_x = 319;
            if (cursor_y < 0) cursor_y = 0;
            if (cursor_y > 199) cursor_y = 199;
            if (mouse.buttons_pressed & 1)
                new_city_action(sim_new_city_event(&new_city, sim_new_city_hit_test(cursor_x, cursor_y)));
        }
        return;
    }
    if (screen == CITY) {
        int terrain_mode=session.active&&city_session_dgroup(&session)[0x32f2];
        if(session.active&&city_session_dgroup(&session)[0x5326]!=1){if(controller_pressed(OF_BTN_START)){open_menu=SIM_MENU_SYSTEM;menu_row=sim_menus_next(&game_menus,SIM_MENU_SYSTEM,-1,1);}return;}
        if(controller_pressed(OF_BTN_START)){open_menu=SIM_MENU_SYSTEM;menu_row=sim_menus_next(&game_menus,SIM_MENU_SYSTEM,-1,1);return;}
        for(unsigned i=0;i<4;++i)if(key_pressed(&kb,0x1e + i))queued_event=81+(int)i;
        if(key_pressed(&kb,0x27))queued_event=80;
        if(terrain_mode){
            if(controller_pressed(OF_BTN_L1))queued_event=105;
            if(controller_pressed(OF_BTN_R1))queued_event=106;
            if(controller_pressed(OF_BTN_X)){edit_focus=(edit_focus+1)%4;queued_event=100+edit_focus;}
            if(controller_pressed(OF_BTN_Y)){edit_focus=(edit_focus+3)%4;queued_event=100+edit_focus;}
        }else{
            if (controller_pressed(OF_BTN_L1) || key_pressed(&kb, 7)) city_tool = SIM_CITY_BULLDOZE;
            if (controller_pressed(OF_BTN_R1) || key_pressed(&kb, 0x15)) city_tool = SIM_CITY_ROAD;
            if (controller_pressed(OF_BTN_X)) city_tool = city_tool == 16 ? 1 : city_tool+1;
            if (controller_pressed(OF_BTN_Y)) city_tool = city_tool == 1 ? 16 : city_tool-1;
        }
        if (mouse.present) {
            cursor_x += mouse.dx; cursor_y += mouse.dy;
            if (cursor_x < 0) cursor_x = 0;
            if (cursor_x > 319) cursor_x = 319;
            if (cursor_y < 0) cursor_y = 0;
            if (cursor_y > 199) cursor_y = 199;
            if (mouse.buttons_pressed & 1) {
                int bar=sim_menus_bar_hit(cursor_x,cursor_y);
                if(bar>=0&&bar<4){open_menu=bar;menu_row=sim_menus_next(&game_menus,(sim_menu_kind)bar,-1,1);return;}
                if(terrain_mode){int event=sim_terrain_edit_hit(city_session_dgroup(&session),65536,cursor_x,cursor_y);if(event){ui_click_consumed=1;if(event<=0x113)edit_focus=event-0x110;queued_event=100+event-0x110;return;}}
                else {int tool=sim_editor_tool_hit(session.cpu.memory,SIM_DOS_MEMORY_SIZE,cursor_x,cursor_y);if(tool){city_tool=(unsigned)tool;ui_click_consumed=1;return;}}
            }
        }
        if(!terrain_mode&&(mouse.buttons_pressed&2))city_tool=16;
        unsigned active_tool=terrain_mode?100+(unsigned)edit_focus:city_tool;
        if(ui_click_consumed){stroke_active=stroke_failed=0;return;}
        int pressed=controller_pressed(OF_BTN_A) || (mouse.buttons_pressed&3);
        int held=controller_held(OF_BTN_A) || (mouse.buttons&1) || pressed;
        if(!held){stroke_active=0;stroke_failed=0;}
        if (held) {
            int x = (cursor_x-city_view.screen_x)/8;
            int y = (cursor_y-city_view.screen_y)/8;
            if (cursor_x >= city_view.screen_x && cursor_y >= city_view.screen_y &&
                x < (int)city_view.columns && y < (int)city_view.rows)
            {
                int mx=x+city_view.map_x,my=y+city_view.map_y;
                if(pressed || !stroke_active || mx!=last_build_x || my!=last_build_y || active_tool!=last_build_tool) {
                    if(pressed || !stroke_active)stroke_failed=0;
                    if(session.active){queued_place=1;queued_x=mx;queued_y=my;queued_tool=(int)active_tool;}
                    else {
                        uint16_t result=sim_city_place_view(terrain.tiles,&city_funds,&placement,
                            mx,my,city_tool,1,city_view.map_x,city_view.map_y,
                            (int)city_view.columns,(int)city_view.rows);
                        if(result==1)play_sound(city_tool<=4?SIM_SOUND_BULLDOZE:SIM_SOUND_BUILD);
                        else if(!stroke_failed){play_sound(SIM_SOUND_HONK);stroke_failed=1;}
                    }
                    stroke_active=1;last_build_x=mx;last_build_y=my;last_build_tool=active_tool;
                }
            }
        }
        return;
    }
    if (controller_pressed(OF_BTN_RIGHT | OF_BTN_DOWN) || key_pressed(&kb, 0x2e) ||
        key_pressed(&kb, 0x4f) || key_pressed(&kb, 0x51)) move_selection(1);
    if (controller_pressed(OF_BTN_LEFT | OF_BTN_UP) || key_pressed(&kb, 0x2d) ||
        key_pressed(&kb, 0x50) || key_pressed(&kb, 0x52)) move_selection(-1);
    if (mouse.present) {
        cursor_x += mouse.dx; cursor_y += mouse.dy;
        if (cursor_x < 0) cursor_x = 0;
        if (cursor_x > 319) cursor_x = 319;
        if (cursor_y < 0) cursor_y = 0;
        if (cursor_y > 199) cursor_y = 199;
        if (!screen && pointer_moved) sim_title_menu_select(&menu, sim_title_hit_test(cursor_x, cursor_y));
        if (screen && pointer_moved) {
            int hit = sim_scenario_hit_test(cursor_x, cursor_y);
            if (hit) scenario_selected = hit - 1;
        }
        if (mouse.buttons_pressed & 1) {
            if (screen) {
                selected(sim_scenario_hit_test(cursor_x, cursor_y));
            } else {
                sim_title_menu_select(&menu, sim_title_hit_test(cursor_x, cursor_y));
                selected(sim_title_menu_accept(&menu));
            }
        }
    }
    if (screen) {
        for (int i = 0; i < 8; ++i) if (key_pressed(&kb, 0x1e + i)) selected(i+1);
    }
    if (controller_pressed(OF_BTN_A | OF_BTN_START) || key_pressed(&kb, 0x28))
        selected(screen ? scenario_selected + 1 : sim_title_menu_accept(&menu));
}

static void frame_input(void)
{
    frame_input_impl();
    if(cursor_x<0)cursor_x=0;
    if(cursor_x>319)cursor_x=319;
    if(cursor_y<0)cursor_y=0;
    if(cursor_y>199)cursor_y=199;
}

#ifdef OF_PC
static int dump(const char *directory, const char *name)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.ppm", directory, name);
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fprintf(f, "P6\n320 200\n255\n");
    for (int i = 0; i < 64000; ++i) {
        uint32_t c = palette[canvas[i]];
        uint8_t rgb[3] = {(uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c};
        if (fwrite(rgb, 1, 3, f) != 3) { fclose(f); return 0; }
    }
    return fclose(f) == 0;
}
#endif

int main(int argc, char **argv)
{
    sim_title_menu_init(&menu);
    sim_terrain_init(&terrain);
    sim_city_placement_init(&placement);
    if (!load_resources()) { fprintf(stderr, "Resource error: %s\n",resource_error); return 1; }
#ifdef OF_PC
    if (argc == 3 && !strcmp(argv[1], "--capture")) {
        render(0); if (!dump(argv[2], "title")) return 1;
        cursor_x = 100; cursor_y = 160;
        render(1); if (!dump(argv[2], "title-reference-cursor")) return 1;
        move_selection(1);
        render(1); if (!dump(argv[2], "title-focused")) return 1;
        selected(3);
        render(0); if (!dump(argv[2], "scenarios")) return 1;
        screen = TITLE; selected(1);
        render(0); if (!dump(argv[2], "new-city-easy")) return 1;
        new_city.difficulty = 2;
        render(0); if (!dump(argv[2], "new-city-hard")) return 1;
        sim_terrain_generate(&terrain, 42); sim_city_view_init(&city_view); screen = CITY;
        city_funds = 5000;
        render(0); if (!dump(argv[2], "generated-city")) return 1;
        for (int x=4; x<29; ++x) for (int y=4; y<17; ++y) terrain.tiles[x*100+y]=0;
        city_funds=20000;
        sim_city_place(terrain.tiles,&city_funds,&placement,8,8,6,1);
        sim_city_place(terrain.tiles,&city_funds,&placement,14,8,7,1);
        sim_city_place(terrain.tiles,&city_funds,&placement,20,8,8,1);
        sim_city_place(terrain.tiles,&city_funds,&placement,25,13,15,1);
        for (int x=5; x<28; ++x) sim_city_place(terrain.tiles,&city_funds,&placement,x,11,2,1);
        city_tool=6;
        render(0); if (!dump(argv[2], "construction-test")) return 1;
        load_city(1);
        render(0);if(!dump(argv[2],"scenario-intro"))return 1;
        uint32_t clock=session.cpu.clock_ticks;
        for(unsigned i=0;i<10000&&!session.safe;++i){
            city_session_step(&session,++clock,60000);
            if(session.modal)city_session_acknowledge(&session);
            if(session.budget_open)city_session_budget_commit(&session);
            if(session.failed)return 1;
        }
        if(!session.safe)return 1;
        sim_maps_center(city_session_dgroup(&session),65536,60,50);
        city_view.map_x=(int16_t)rw(city_session_dgroup(&session)+0x5f10);
        city_view.map_y=(int16_t)rw(city_session_dgroup(&session)+0x5f0e);
        sync_city();activate_window(1);render(0);if(!dump(argv[2],"city-running"))return 1;
        activate_window(2);render(0);if(!dump(argv[2],"maps"))return 1;
        for(unsigned i=0;i<3;++i)sim_graphs_ui_activate(session.cpu.memory,SIM_DOS_MEMORY_SIZE,i);
        activate_window(3);render(0);if(!dump(argv[2],"graphs"))return 1;
        report_window=1;render(0);if(!dump(argv[2],"evaluation"))return 1;
        report_window=0;city_session_budget(&session,0);render(0);if(!dump(argv[2],"budget"))return 1;
        city_session_budget_commit(&session);
        sim_query_ui_read(session.cpu.memory,SIM_DOS_MEMORY_SIZE,60,50,&query_info);
        report_window=4;render(0);if(!dump(argv[2],"query"))return 1;
        report_window=5;render(0);if(!dump(argv[2],"about"))return 1;
        report_window=0;sim_terrain_edit_enter(session.cpu.memory,SIM_DOS_MEMORY_SIZE);sync_city();
        render(0);if(!dump(argv[2],"terrain-editor"))return 1;
        sim_terrain_edit_exit(session.cpu.memory,SIM_DOS_MEMORY_SIZE);sync_city();
        open_menu=SIM_MENU_SYSTEM;menu_row=6;render(0);if(!dump(argv[2],"system-menu"))return 1;
        open_menu=-1;file_mode=2;render(0);if(!dump(argv[2],"save-slots"))return 1;
        file_mode=0;report_window=0;activate_window(1);
        for(unsigned theme=1;theme<=themes.count;++theme){
            if(!choose_graphics(theme))return 1;
            render(0);char name[32];snprintf(name,sizeof(name),"theme-%u",theme);if(!dump(argv[2],name))return 1;
        }
        if(!choose_graphics(0))return 1;
        sim_window_drag_begin(session.cpu.memory,SIM_DOS_MEMORY_SIZE,SIM_WINDOW_RESIZE,188,111,&window_drag);
        sim_window_drag_update(session.cpu.memory,SIM_DOS_MEMORY_SIZE,140,111,&window_drag);
        sim_window_drag_commit(session.cpu.memory,SIM_DOS_MEMORY_SIZE,&window_drag);repaint_editor(2);sync_city();
        sim_window_drag_begin(session.cpu.memory,SIM_DOS_MEMORY_SIZE,SIM_WINDOW_POSITION,188,111,&window_drag);
        sim_window_drag_update(session.cpu.memory,SIM_DOS_MEMORY_SIZE,204,111,&window_drag);
        sim_window_drag_commit(session.cpu.memory,SIM_DOS_MEMORY_SIZE,&window_drag);repaint_editor(2);sync_city();
        activate_window(2);
        sim_window_drag_begin(session.cpu.memory,SIM_DOS_MEMORY_SIZE,SIM_WINDOW_POSITION,188,111,&window_drag);
        sim_window_drag_update(session.cpu.memory,SIM_DOS_MEMORY_SIZE,124,111,&window_drag);
        sim_window_drag_commit(session.cpu.memory,SIM_DOS_MEMORY_SIZE,&window_drag);repaint_editor(1);
        render(0);if(!dump(argv[2],"windows-positioned"))return 1;
        ask_question(6,0,"NEW GAME: Are you sure?");render(0);if(!dump(argv[2],"new-question"))return 1;
        ask_question(8,1,(const char *)city_session_dgroup(&session)+0x256a);render(0);if(!dump(argv[2],"load-question"))return 1;
        puts("PASS: native resource load, title, focus and scenario renders.");
        return 0;
    }
#else
    (void)argc; (void)argv;
#endif
    of_mixer_init(32, OF_MIXER_OUTPUT_RATE);
    refresh_audio_settings(1);
    if(!initialize_video()){fprintf(stderr,"Unable to initialize SimCity framebuffer.\n");return 1;}
    for (;;) {
        refresh_audio_settings(0);
        frame_input();
        simulation_frame();
        render(1);
        present();
        of_video_wait_flip();
    }
}
