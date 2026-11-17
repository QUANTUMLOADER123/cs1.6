#pragma once

#include "sh_types.h"

#define SH_MAX_PHYSENTS 600
#define SH_MAX_MOVEENTS 64
#define SH_MAX_PHYSINFO 512

#define sh_fl_onground 0x200
#define sh_fl_ducking 0x4
#define sh_fl_waterjump 0x800
#define sh_fl_frozen 0x4000

#define sh_ef_muzzleflash 2

#define sh_renderfx_none 0
#define sh_renderfx_glowshell 19
#define sh_renderfx_clampminscale 20

#define sh_movetype_none 0
#define sh_movetype_walk 3
#define sh_movetype_step 4
#define sh_movetype_fly 5
#define sh_movetype_toss 6
#define sh_movetype_noclip 8

#define sh_in_attack 0x001
#define sh_in_jump 0x002
#define sh_in_duck 0x004
#define sh_in_forward 0x008
#define sh_in_back 0x010
#define sh_in_use 0x020
#define sh_in_cancel 0x040
#define sh_in_left 0x080
#define sh_in_right 0x100
#define sh_in_moveleft 0x200
#define sh_in_moveright 0x400
#define sh_in_attack2 0x800
#define sh_in_run 0x1000
#define sh_in_reload 0x2000
#define sh_in_alt1 0x4000
#define sh_in_score 0x8000

typedef struct sh_usercmd_s
{
    i16 lerp_msec;
    u8 msec;
    u8 pad0;
    vec3_t viewangles;
    f32 forwardmove;
    f32 sidemove;
    f32 upmove;
    u8 lightlevel;
    u8 pad1;
    u16 buttons;
    u8 impulse;
    u8 weaponselect;
    u8 pad2[2];
    i32 impact_index;
    vec3_t impact_position;
} sh_usercmd_t;

typedef char sh_usercmd_size_check[(sizeof(sh_usercmd_t) == 0x34) ? 1 : -1];

typedef struct sh_movevars_s
{
    f32 gravity;
    f32 stopspeed;
    f32 maxspeed;
    f32 spectatormaxspeed;
    f32 accelerate;
    f32 airaccelerate;
    f32 wateraccelerate;
    f32 friction;
    f32 edgefriction;
    f32 waterfriction;
    f32 entgravity;
    f32 bounce;
    f32 stepsize;
    f32 maxvelocity;
    f32 zmax;
    f32 waveheight;
    qboolean footsteps;
    char skyname[32];
    f32 rollangle;
    f32 rollspeed;
    f32 skycolor_r;
    f32 skycolor_g;
    f32 skycolor_b;
    f32 skyvec_x;
    f32 skyvec_y;
    f32 skyvec_z;
} sh_movevars_t;

typedef struct sh_physent_s
{
    char name[32];
    i32 player;
    vec3_t origin;
    void *model;
    void *studiomodel;
    vec3_t mins;
    vec3_t maxs;
    i32 info;
    vec3_t angles;
    i32 solid;
    i32 skin;
    i32 rendermode;
    f32 frame;
    i32 sequence;
    u8 controller[4];
    u8 blending[2];
    u8 pad0[2];
    i32 movetype;
    i32 takedamage;
    i32 blooddecal;
    i32 team;
    i32 classnumber;
    i32 iuser1;
    i32 iuser2;
    i32 iuser3;
    i32 iuser4;
    f32 fuser1;
    f32 fuser2;
    f32 fuser3;
    f32 fuser4;
    vec3_t vuser1;
    vec3_t vuser2;
    vec3_t vuser3;
    vec3_t vuser4;
} sh_physent_t;

typedef char sh_physent_size_check[(sizeof(sh_physent_t) == 0xE0) ? 1 : -1];

typedef struct sh_pmplane_s
{
    vec3_t normal;
    f32 dist;
} sh_pmplane_t;

typedef struct sh_pmtrace_s
{
    qboolean allsolid;
    qboolean startsolid;
    qboolean inopen;
    qboolean inwater;
    f32 fraction;
    vec3_t endpos;
    sh_pmplane_t plane;
    i32 ent;
    vec3_t delta_velocity;
    i32 hitgroup;
} sh_pmtrace_t;

typedef char sh_pmtrace_size_check[(sizeof(sh_pmtrace_t) == 0x44) ? 1 : -1];

typedef sh_pmtrace_t (*sh_ptrace_fn)(vec3_t start, vec3_t end, i32 flags, i32 ignore_pe);
typedef i32 (*sh_pointcontents_fn)(vec3_t p, i32 *truecontents);
typedef const char *(*sh_tracetexture_fn)(i32 ground, vec3_t start, vec3_t end);

typedef struct sh_playermove_s
{
    i32 player_index;
    qboolean server;
    qboolean multiplayer;
    f32 time;
    f32 frametime;
    vec3_t forward;
    vec3_t right;
    vec3_t up;
    vec3_t origin;
    vec3_t angles;
    vec3_t oldangles;
    vec3_t velocity;
    vec3_t movedir;
    vec3_t basevelocity;
    vec3_t view_ofs;
    f32 duck_time;
    qboolean in_duck;
    i32 timestep_sound;
    i32 step_left;
    f32 fall_velocity;
    vec3_t punchangle;
    f32 swim_time;
    f32 next_primary_attack;
    i32 effects;
    i32 flags;
    i32 usehull;
    f32 gravity;
    f32 friction;
    i32 oldbuttons;
    f32 waterjumptime;
    qboolean dead;
    i32 deadflag;
    i32 spectator;
    i32 movetype;
    i32 onground;
    i32 waterlevel;
    i32 watertype;
    i32 oldwaterlevel;
    char texturename[256];
    char chtexturetype;
    f32 maxspeed;
    f32 clientmaxspeed;
    i32 iuser1;
    i32 iuser2;
    i32 iuser3;
    i32 iuser4;
    f32 fuser1;
    f32 fuser2;
    f32 fuser3;
    f32 fuser4;
    vec3_t vuser1;
    vec3_t vuser2;
    vec3_t vuser3;
    vec3_t vuser4;
    i32 numphysent;
    sh_physent_t physents[SH_MAX_PHYSENTS];
    i32 nummoveent;
    sh_physent_t moveents[SH_MAX_MOVEENTS];
    i32 numvisent;
    sh_physent_t visents[SH_MAX_PHYSENTS];
    sh_usercmd_t cmd;
    i32 numtouch;
    sh_pmtrace_t touchindex[SH_MAX_PHYSENTS];
    char physinfo[SH_MAX_PHYSINFO];
    sh_movevars_t *movevars;
    f32 player_mins[4][3];
    f32 player_maxs[4][3];

    void *(*pm_info_value_for_key)(const char *s, const char *key);
    void (*pm_particle)(f32 *origin, i32 color, f32 life, i32 zpos, i32 zvel);
    i32 (*pm_test_player_position)(f32 *pos, sh_pmtrace_t *ptrace);
    void *con_nprintf;
    void *con_dprintf;
    void *con_printf;
    f64 (*sys_float_time)(void);
    void (*pm_stuck_touch)(i32 hitent, sh_pmtrace_t *ptrace);
    i32 (*pm_point_contents)(f32 *p, i32 *true_contents);
    i32 (*pm_true_point_contents)(f32 *p);
    i32 (*pm_hull_point_contents)(void *hull, i32 num, f32 *p);
    sh_pmtrace_t (*pm_player_trace)(f32 *start, f32 *end, i32 flags,
                                    i32 ignore_pe);
    sh_pmtrace_t *(*pm_trace_line)(f32 *start, f32 *end, i32 flags,
                                   i32 use_hull, i32 ignore_pe);
    i32 (*random_long)(i32 low, i32 high);
    f32 (*random_float)(f32 low, f32 high);
    i32 (*pm_get_model_type)(void *mod);
    void (*pm_get_model_bounds)(void *mod, f32 *mins, f32 *maxs);
    void *(*pm_hull_for_bsp)(sh_physent_t *pe, f32 *offset);
    f32 (*pm_trace_model)(sh_physent_t *pe, f32 *start, f32 *end, void *trace);
    i32 (*com_file_size)(char *filename);
    u8 *(*com_load_file)(const char *path, i32 use_hunk, i32 *length);
    void (*com_free_file)(void *buffer);
    char *(*memfgets)(u8 *mem_file, i32 file_size, i32 *pos, char *buffer,
                      i32 buffer_size);
    qboolean run_funcs;
    void (*pm_play_sound)(i32 channel, const char *sample, f32 volume,
                          f32 attenuation, i32 flags, i32 pitch);
    const char *(*pm_trace_texture)(i32 ground, f32 *start, f32 *end);
} sh_playermove_t;

typedef char sh_playermove_size_check[(sizeof(sh_playermove_t) == 0x4F6BC) ? 1 : -1];
typedef char sh_playermove_exact[(sizeof(sh_playermove_t) == 325308) ? 1 : -1];
#define SH_PM_OFF_MOVEVARS ((i32)((char*)&((sh_playermove_t*)0)->movevars - (char*)0))
#define SH_PM_OFF_TRACETEX ((i32)((char*)&((sh_playermove_t*)0)->pm_trace_texture - (char*)0))
#define SH_PM_SIZE ((i32)sizeof(sh_playermove_t))

typedef struct sh_color24_s
{
    u8 r;
    u8 g;
    u8 b;
} sh_color24_t;

typedef struct sh_colorvec_s
{
    u32 r;
    u32 g;
    u32 b;
} sh_colorvec_t;

typedef struct sh_entity_state_s
{
    i32 entity_type;
    i32 number;
    f32 msg_time;
    i32 messagenum;
    vec3_t origin;
    vec3_t angles;
    i32 modelindex;
    i32 sequence;
    f32 frame;
    i32 colormap;
    i16 skin;
    i16 solid;
    i32 effects;
    f32 scale;
    u8 eflags;
    u8 pad_eflags[3];
    i32 rendermode;
    i32 renderamt;
    sh_color24_t rendercolor;
    u8 pad_rendercolor;
    i32 renderfx;
    i32 movetype;
    f32 animtime;
    f32 framerate;
    i32 body;
    u8 controller[4];
    u8 blending[4];
    vec3_t velocity;
    vec3_t mins;
    vec3_t maxs;
    i32 aiment;
    i32 owner;
    f32 friction;
    f32 gravity;
    i32 team;
    i32 playerclass;
    i32 health;
    i32 spectator;
    i32 weaponmodel;
    i32 gaitsequence;
    vec3_t basevelocity;
    i32 usehull;
    i32 oldbuttons;
    i32 onground;
    i32 step_left;
    f32 fall_velocity;
    f32 fov;
    i32 weaponanim;
    vec3_t startpos;
    vec3_t endpos;
    f32 impacttime;
    f32 starttime;
    i32 iuser1;
    i32 iuser2;
    i32 iuser3;
    i32 iuser4;
    f32 fuser1;
    f32 fuser2;
    f32 fuser3;
    f32 fuser4;
    vec3_t vuser1;
    vec3_t vuser2;
    vec3_t vuser3;
    vec3_t vuser4;
} sh_entity_state_t;

typedef char sh_entity_state_size_check[(sizeof(sh_entity_state_t) == 0x154) ? 1 : -1];

typedef struct sh_position_history_s
{
    f32 animtime;
    vec3_t origin;
    vec3_t angles;
} sh_position_history_t;

typedef struct sh_mouth_s
{
    u8 mouthopen;
    u8 sndcount;
    u8 pad0[2];
    i32 sndavg;
} sh_mouth_t;

typedef struct sh_latchedvars_s
{
    f32 prevanimtime;
    f32 sequencetime;
    u8 prevseqblending[2];
    u8 pad0[2];
    vec3_t prevorigin;
    vec3_t prevangles;
    i32 prevsequence;
    f32 prevframe;
    u8 prevcontroller[4];
    u8 prevblending[2];
    u8 pad1[2];
} sh_latchedvars_t;

typedef struct sh_cl_entity_s
{
    i32 index;
    i32 player;
    sh_entity_state_t baseline;
    sh_entity_state_t prevstate;
    sh_entity_state_t curstate;
    i32 current_position;
    sh_position_history_t ph[64];
    sh_mouth_t mouth;
    sh_latchedvars_t latched;
    f32 lastmove;
    vec3_t origin;
    vec3_t angles;
    vec3_t attachment[4];
    i32 trivial_accept;
    void *model;
    void *efrag;
    void *topnode;
    f32 syncbase;
    i32 visframe;
    sh_colorvec_t floor_color;
} sh_cl_entity_t;

typedef char sh_cl_entity_size_check[(sizeof(sh_cl_entity_t) == 0xBB4) ? 1 : -1];

typedef struct sh_triapi_s
{
    i32 version;
    void (*render_mode)(i32 mode);
    void (*begin)(i32 primitive);
    void (*end)(void);
    void (*color4f)(f32 r, f32 g, f32 b, f32 a);
    void (*color4ub)(u8 r, u8 g, u8 b, u8 a);
    void (*tex_coord_2f)(f32 u, f32 v);
    void (*vertex3fv)(f32 *p);
    void (*vertex3f)(f32 x, f32 y, f32 z);
    void (*brightness)(f32 b);
    void (*cull_face)(i32 style);
    i32 (*sprite_texture)(void *model, i32 frame);
    i32 (*world_to_screen)(f32 *world, f32 *screen);
    void (*fog)(f32 color[3], f32 start, f32 end, i32 on);
    void (*screen_to_world)(f32 *screen, f32 *world);
    void (*get_matrix)(const i32 name, f32 *matrix);
    i32 (*box_in_pvs)(f32 *mins, f32 *maxs);
    void (*light_at_point)(f32 *pos, f32 *value);
    void (*color4f_rendermode)(f32 r, f32 g, f32 b, f32 a, i32 rendermode);
    void (*fog_params)(f32 density, i32 skybox);
} sh_triapi_t;

typedef struct sh_screeninfo_s
{
    i32 width;
    i32 height;
} sh_screeninfo_t;

typedef struct sh_hud_player_info_s
{
    char name[32];
    char model[64];
    i32 topcolor;
    i32 bottomcolor;
    i32 health;
    i32 primaryweapon;
    i32 secondaryweapon;
    i32 level;
    i32 spectator;
    i32 teamnumber;
} sh_hud_player_info_t;

typedef struct sh_client_data_s
{
    vec3_t origin;
    vec3_t velocity;
    i32 viewmodel;
    vec3_t punchangle;
    i32 flags;
    i32 waterlevel;
    i32 watertype;
    vec3_t view_ofs;
    f32 health;
    i32 in_duck;
    i32 weapons;
    i32 time_step_sound;
    i32 duck_time;
    i32 swim_time;
    i32 waterjumptime;
    f32 maxspeed;
    f32 fov;
    i32 weaponanim;
    i32 weapon_id;
    i32 ammo_shells;
    i32 ammo_nails;
    i32 ammo_cells;
    i32 ammo_rockets;
    f32 next_attack;
    i32 tfstate;
    i32 pushmsec;
    i32 deadflag;
    char physinfo[256];
    i32 iuser1;
    i32 iuser2;
    i32 iuser3;
    i32 iuser4;
    f32 fuser1;
    f32 fuser2;
    f32 fuser3;
    f32 fuser4;
    vec3_t vuser1;
    vec3_t vuser2;
    vec3_t vuser3;
    vec3_t vuser4;
} sh_client_data_t;

typedef char sh_client_data_size_check[(sizeof(sh_client_data_t) == 0x1DC) ? 1 : -1];




typedef struct sh_studio_bone_s
{
    char name[32];
    i32 parent;
    i32 flags;
    i32 bonecontroller[6];
    f32 value[6];
    f32 scale[6];
} sh_studio_bone_t;

typedef char sh_studio_bone_size_check[(sizeof(sh_studio_bone_t) == 112) ? 1 : -1];

typedef struct sh_studio_header_s
{
    i32 id;
    i32 version;
    char name[64];
    i32 length;
    f32 eyeposition[3];
    f32 min[3];
    f32 max[3];
    f32 bbmin[3];
    f32 bbmax[3];
    i32 flags;
    i32 numbones;
    i32 boneindex;
    i32 numbonecontrollers;
    i32 bonecontrollerindex;
    i32 numhitboxes;
    i32 hitboxindex;
    i32 numseq;
    i32 seqindex;
    i32 numseqgroups;
    i32 seqgroupindex;
    i32 numtextures;
    i32 textureindex;
    i32 texturedataindex;
    i32 numskinref;
    i32 numskinfamilies;
    i32 skinindex;
    i32 numbodyparts;
    i32 bodypartindex;
    i32 numattachments;
    i32 attachmentindex;
} sh_studio_header_t;

typedef struct sh_studio_iface_s
{
    i32 version;
    i32 (*studio_draw_model)(i32 flags);
    i32 (*studio_draw_player)(i32 flags, sh_entity_state_t *pplayer);
} sh_studio_iface_t;

typedef struct sh_studio_api_s
{
    void *mem_calloc;
    void *cache_check;
    void *load_cache_file;
    void *mod_for_name;
    void *(*mod_extradata)(void *mod);
    void *get_model_by_index;
    sh_cl_entity_t *(*get_current_entity)(void);
    void *player_info;
    void *get_player_state;
    void *get_view_entity;
    void *get_times;
    void *get_cvar;
    void *get_view_info;
    void *get_chrome_sprite;
    void *get_model_counters;
    void *get_alias_scale;
    f32 ****(*studio_get_bone_transform)(void);
    f32 ****(*studio_get_light_transform)(void);
    f32 ***(*studio_get_alias_transform)(void);
    f32 ***(*studio_get_rotation_matrix)(void);
    void (*studio_setup_model)(i32 bodypart, void **ppbodypart, void **ppsubmodel);
} sh_studio_api_t;

typedef struct sh_weapon_data_s
{
    i32 weapon_id;
    i32 clip;
    f32 next_primary_attack;
    f32 next_secondary_attack;
    f32 idle_time;
    i32 in_reload;
    i32 in_special_reload;
    f32 next_reload;
    f32 pump_time;
    f32 reload_time;
    f32 aimed_damage;
    f32 next_aim_bonus;
    i32 in_zoom;
    i32 weapon_state;
    i32 iuser1;
    i32 iuser2;
    i32 iuser3;
    i32 iuser4;
    f32 fuser1;
    f32 fuser2;
    f32 fuser3;
    f32 fuser4;
} sh_weapon_data_t;

#define sh_max_weapons 64

typedef char sh_weapon_data_size_check[(sizeof(sh_weapon_data_t) == 0x58) ? 1 : -1];


typedef struct sh_globalvars_s
{
    f32 time;
    f32 frametime;
    f32 force_retouch;
    char mapname[32];
    f32 deathmatch;
    f32 coop;
    f32 teamplay;
    f32 serverflags;
    f32 found_secrets;
    vec3_t v_forward;
    vec3_t v_up;
    vec3_t v_right;
    f32 trace_allsolid;
    f32 trace_startsolid;
    f32 trace_fraction;
    vec3_t trace_endpos;
    vec3_t trace_plane_normal;
    f32 trace_plane_dist;
    void *trace_ent;
    f32 trace_inopen;
    f32 trace_inwater;
    i32 trace_hitgroup;
    i32 trace_flags;
    i32 msg_entity;
    void *cd;
    f32 *host_frametime;
} sh_globalvars_t;



typedef struct sh_spr_rect_s
{
    i32 left;
    i32 right;
    i32 top;
    i32 bottom;
} sh_spr_rect_t;

typedef struct sh_client_sprite_s
{
    char name[64];
    char sprite[64];
    i32 hspr;
    i32 res;
    sh_spr_rect_t rc;
} sh_client_sprite_t;

typedef struct sh_spr_list_fn_s
{
    sh_client_sprite_t *(*get_list)(const char *filename, i32 *count);
} sh_spr_list_fn_t;

typedef struct sh_engfuncs_s
{
    i32 (*pfnSPR_Load)(const char *name);
    i32 (*pfnSPR_Frames)(i32 spr);
    i32 (*pfnSPR_Height)(i32 spr, i32 frame);
    i32 (*pfnSPR_Width)(i32 spr, i32 frame);
    void (*pfnSPR_Set)(i32 spr, i32 r, i32 g, i32 b);
    void (*pfnSPR_Draw)(i32 frame, i32 x, i32 y,
                        const sh_spr_rect_t *rect);
    void (*pfnSPR_DrawHoles)(i32 frame, i32 x, i32 y,
                             const sh_spr_rect_t *rect);
    void (*pfnSPR_DrawAdditive)(i32 frame, i32 x, i32 y,
                                const sh_spr_rect_t *rect);
    void (*pfnSPR_EnableScissor)(i32 x, i32 y, i32 width, i32 height);
    void (*pfnSPR_DisableScissor)(void);
    sh_client_sprite_t *(*pfnSPR_GetList)(const char *filename, i32 *count);
    void *pfnFillRGBA;

    i32 (*pfnGetScreenInfo)(sh_screeninfo_t *info);
    void *pad_034[5];

    i32 (*pfnHookUserMsg)(const char *name, i32 (*cb)(const char *, i32, i32));
    i32 (*pfnServerCmd)(const char *cmd);
    i32 (*pfnClientCmd)(const char *cmd);
    void (*pfnGetPlayerInfo)(i32 index, sh_hud_player_info_t *info);
    void (*pfnPlaySoundByName)(const char *name, f32 volume);
    void (*pfnPlaySoundByIndex)(i32 index, f32 volume);

    void *pad_060[3];
    i32 (*pfnDrawConsoleString)(i32 x, i32 y, const char *str);
    void *pad_070;
    i32 (*pfnDrawConsoleStringLen)(const char *str, i32 *width, i32 *height);
    void (*pfnConsolePrint)(const char *str);

    void *pad_07C[3];

    void (*pfnGetViewAngles)(f32 *out);
    void (*pfnSetViewAngles)(f32 *in);

    void *pad_090;
    void (*pfnCvar_SetValue)(const char *name, f32 value);
    void *pad_098;
    const char *(*pfnCmd_Argv)(i32 index);
    void (*pfnCon_Printf)(const char *fmt, ...);
    void (*pfnCon_DPrintf)(const char *fmt, ...);

    void *pad_0A8[6];

    void (*pfnKey_Event)(i32 key, i32 down);
    void *pad_0C4[2];

    i32 (*pfnGetLocalPlayer)(void);
    void *(*pfnGetViewModel)(void);
    void *(*pfnGetEntityByIndex)(i32 index);

    void *pad_0D8[9];

    void *pad_0FC[4];

    f32 (*pfnRandomFloat)(f32 lo, f32 hi);
    i32 (*pfnRandomLong)(i32 lo, i32 hi);

    void *pad_114[3];

    void *(*pfnGetCvarPointer)(const char *name);

    void *pad_124[9];

    void *p_tri_api;
    void *p_efx_api;
    void *p_event_api;
    void *p_demo_api;
    void *p_net_api;
    void *p_voice_tweak;

    void *pad_160[28];

    char *(*pfnPlayerInfo_ValueForKey)(i32 index, const char *key);

    void *pad_1D4[4];

    void (*pfnCvar_Set)(const char *name, const char *value);

    void *pad_1E8[12];

    i32 (*pfnFilteredClientCmd)(const char *cmd);
} sh_engfuncs_t;

typedef char sh_engfuncs_size_check[(sizeof(sh_engfuncs_t) == 0x21C) ? 1 : -1];

extern sh_triapi_t *g_tri_api;
extern sh_engfuncs_t *g_engfuncs;


extern sh_triapi_t *g_tri_api;
extern sh_engfuncs_t *g_engfuncs;
extern sh_playermove_t *g_pmove;
extern sh_globalvars_t *g_globals;
extern void **g_entity_list;
extern sh_cl_entity_t *(*g_get_entity)(i32 index);
extern i32 g_local_player;
extern void *g_module_client;
extern void *g_module_hw;
extern void **g_pmove_slot;
extern void **g_engfuncs_slot;

extern i32 g_cfg_local_team;
void sh_set_trace_fn(i32 (*fn)(vec3_t, vec3_t, i32, i32));
extern i32 (*g_trace_fn)(vec3_t, vec3_t, i32, i32);
void sh_sdk_resolve(void);
qbool sh_sdk_ready(void);
sh_cl_entity_t *sh_get_entity(i32 index);
void sh_sdk_set_entity_fn(sh_cl_entity_t *(*fn)(i32));

typedef struct sh_tempent_s
{
    i32 flags;
    f32 die;
    f32 frame_max;
    f32 x;
    f32 y;
    f32 z;
    f32 fade_speed;
    f32 bounce_factor;
    i32 hit_sound;
    void *hit_callback;
    void *callback;
    struct sh_tempent_s *next;
    i32 priority;
    i16 client_index;
    f32 tent_offset[3];
    sh_cl_entity_t entity;
} sh_tempent_t;

typedef char sh_tempent_offset_check[
    (((char *)&((sh_tempent_t *)0)->entity - (char *)0) == 0x44) ? 1 : -1];

typedef void(__cdecl *sh_tempent_update_fn)(f64 frametime, f64 client_time,
                                            f64 cl_gravity,
                                            sh_tempent_t **ppfree,
                                            sh_tempent_t **ppactive,
                                            i32 (*add_visible)(sh_cl_entity_t *),
                                            void (*play_sound)(sh_tempent_t *,
                                                               f32));
