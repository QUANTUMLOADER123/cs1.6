#pragma once

#include "sh_types.h"
#include "sh_config.h"
#include "sh_sdk.h"

typedef struct sh_player_s
{
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

qbool sh_player_visible(const sh_player_t *p);
qbool sh_player_behind_smoke(const sh_player_t *p);

void sh_aim_run(sh_usercmd_t *cmd);
void sh_aim_select_target(sh_target_t *out);
void sh_aim_lock_angles(sh_target_t *target, vec3_t *angles);

qbool sh_trace_line(const vec3_t *start, const vec3_t *end, vec3_t *hitsh);

void sh_antiaim_run(sh_usercmd_t *cmd);
void sh_antiaim_apply_pitch(vec3_t *angles, sh_usercmd_t *cmd, f32 frametime);
void sh_antiaim_apply_yaw(vec3_t *angles, sh_usercmd_t *cmd, f32 frametime);

void sh_misc_run(sh_usercmd_t *cmd);

void sh_esp_run(void);
void sh_esp_draw_player(const sh_player_t *p);

void sh_hud_run(void);

void sh_menu_draw(void);
