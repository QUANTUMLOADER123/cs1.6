#pragma once

#include "sh_types.h"
#include "sh_config.h"
#include "sh_sdk.h"

typedef struct sh_player_s
{
    void *fake_ent;
    sh_cl_entity_t *ent;
    i32 index;
    sh_hud_player_info_t info;
    char name[32];
    i32 team;
    i32 health;
    i32 armor;
    i32 weapon_id;
    char weapon_name[32];
    f32 distance;
    qbool visible;
    vec3_t origin;
    vec3_t head;
    vec3_t mins;
    vec3_t maxs;
} sh_player_t;

typedef struct sh_target_s
{
    qbool valid;
    sh_player_t player;
    i32 hitbox;
    f32 fov;
    f32 distance;
    vec3_t point;
} sh_target_t;

enum sh_hitbox
{
    sh_hitbox_head = 0,
    sh_hitbox_neck,
    sh_hitbox_chest,
    sh_hitbox_stomach,
    sh_hitbox_pelvis,
    sh_hitbox_legs
};

void sh_players_collect(void);
i32 sh_players_count(void);
sh_player_t *sh_player_get(i32 index);
sh_player_t *sh_player_local(void);
void sh_players_resolve_fakes(void);

qbool sh_player_visible(const sh_player_t *p);
void sh_smoke_add(const vec3_t *origin);
void sh_smoke_tick(f32 now);
qbool sh_player_behind_smoke(const sh_player_t *p);

void sh_aim_run(sh_usercmd_t *cmd);
void sh_aim_select_target(sh_target_t *out);
void sh_aim_lock_angles(sh_target_t *target, vec3_t *angles);

qbool sh_trace_line(const vec3_t *start, const vec3_t *end, vec3_t *hitsh);
qbool sh_trace_fraction(const vec3_t *start, const vec3_t *end, f32 *out_hit);
i32 sh_trace_material(const vec3_t *start, const vec3_t *end);

void sh_antiaim_run(sh_usercmd_t *cmd);
void sh_antiaim_apply_pitch(vec3_t *angles, sh_usercmd_t *cmd, f32 frametime);
void sh_antiaim_apply_yaw(vec3_t *angles, sh_usercmd_t *cmd, f32 frametime);

void sh_misc_run(sh_usercmd_t *cmd);

void sh_events_reset(void);
void sh_events_on_fire(i32 entindex, const vec3_t *origin, i32 event_id);
i32  sh_events_fire_count(i32 entindex);
f32  sh_events_last_fire(i32 entindex);
qbool sh_events_is_firing(i32 entindex, f32 window);
void sh_misc_on_post_run(sh_usercmd_t *cmd, i32 runfuncs, f64 time);

void sh_esp_run(void);
void sh_esp_draw_player(const sh_player_t *p);

void sh_hud_run(void);

#define sh_msg_deathmsg 0
#define sh_msg_scoreattrib 1

typedef struct sh_killfeed_s
{
    char killer[32];
    char victim[32];
    char weapon[32];
    qbool headshot;
    f32 time;
} sh_killfeed_t;

typedef struct sh_weapon_list_s
{
    char name[32];
    i32 id;
    i32 slot;
    i32 clip;
    i32 clip_max;
    i32 ammo_type;
    qbool valid;
} sh_weapon_list_t;

typedef struct sh_msg_state_s
{
    sh_weapon_list_t weapons[64];
    i32 weapon_count;
    i32 cur_weapon_id;
    i32 cur_weapon_clip;
    u8 attrib[SH_MAX_ENTITIES];
    i32 money[SH_MAX_ENTITIES];
    sh_killfeed_t feed[8];
    i32 feed_head;
    i32 round_seconds;
} sh_msg_state_t;

extern sh_msg_state_t g_msgs;

void sh_msgs_reset(void);
void sh_msgs_register(void);

void sh_msg_weapon_list(const char *name, i32 id, i32 slot, i32 clip,
                        i32 clip_max, i32 ammo_type);
void sh_msg_cur_weapon(i32 state, i32 id, i32 clip);

const sh_weapon_list_t *sh_weapon_get(i32 id);
const char *sh_weapon_icon(i32 id);
void sh_msg_score_attrib(i32 index, u8 attrib);
void sh_msg_round_time(i32 seconds);
i32 sh_player_is_dead(i32 index);

void sh_menu_draw(void);
