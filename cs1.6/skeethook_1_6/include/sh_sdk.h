#pragma once

#include "sh_types.h"

#define SH_MAX_PHYSENTS 600

#define sh_fl_onground 0x200
#define sh_fl_ducking 0x4
#define sh_fl_waterjump 0x800
#define sh_fl_frozen 0x4000

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
    qbool footsteps;
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

typedef struct sh_pmtrace_s
{
    qbool allsolid;
    qbool startsolid;
    qbool inopen;
    qbool inwater;
    f32 fraction;
    vec3_t endpos;
    struct
    {
        qbool allsolid;
        qbool startsolid;
        f32 fraction;
        vec3_t endpos;
        vec3_t normal;
        i32 hitgroup;
    } plane;
    i32 ent;
    i32 hitgroup;
} sh_pmtrace_t;

typedef sh_pmtrace_t (*sh_ptrace_fn)(vec3_t start, vec3_t end, i32 flags, i32 ignore_pe);
typedef i32 (*sh_pointcontents_fn)(vec3_t p, i32 *truecontents);
typedef const char *(*sh_tracetexture_fn)(i32 ground, vec3_t start, vec3_t end);

typedef struct sh_playermove_s
{
    i32 player_index;
    qbool server;
    qbool multiplayer;
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
    qbool in_duck;
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
    qbool dead;
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
    sh_physent_t moveents[SH_MAX_PHYSENTS];
    i32 numvisent;
    sh_physent_t visents[SH_MAX_PHYSENTS];
    sh_usercmd_t cmd;
    i32 numtouch;
    sh_pmtrace_t touchindex[SH_MAX_PHYSENTS];
    char physinfo[512];
    sh_movevars_t *movevars;
    f32 player_mins[4][3];
    f32 player_maxs[4][3];
} sh_playermove_t;

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
} sh_weapon_data_t;

typedef char sh_weapon_data_size_check[(sizeof(sh_weapon_data_t) == 0x3C) ? 1 : -1];


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

typedef struct sh_engfuncs_s
{
    void *pad_000[12];

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

    void *pad_114[13];

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
