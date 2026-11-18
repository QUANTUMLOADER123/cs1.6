#pragma once

#include "sh_types.h"

#define SH_MAX_ENTITIES 256

enum sh_key
{
    sh_key_insert = 0x2D,
    sh_key_delete = 0x2E,
    sh_key_home = 0x24,
    sh_key_end = 0x23,
    sh_key_f1 = 0x70,
    sh_key_f2 = 0x71,
    sh_key_mouse1 = 0x01,
    sh_key_mouse2 = 0x02,
    sh_key_mouse3 = 0x04,
    sh_key_mouse4 = 0x05,
    sh_key_mouse5 = 0x06,
    sh_key_shift = 0x10,
    sh_key_ctrl = 0x11,
    sh_key_alt = 0x12,
    sh_key_space = 0x20,
    sh_key_tab = 0x09
};

typedef struct sh_aimbot_cfg
{
    qbool enabled;
    qbool silent;
    qbool auto_fire;
    qbool auto_wall;
    qbool visible_check;
    qbool smoke_check;
    f32 fov;
    f32 smooth;
    i32 hitbox;
    qbool auto_scope;
    i32 multipoint;
    i32 backtrack;
    i32 priority;
    f32 min_damage;
    i32 key;
    qbool double_tap;
    qbool penetration;
    qbool auto_stop;
    qbool auto_stop_duck;
} sh_aimbot_cfg;

typedef struct sh_antiaim_cfg
{
    qbool enabled;
    i32 pitch_mode;
    i32 yaw_mode;
    f32 yaw_base;
    f32 yaw_jitter;
    qbool desync;
    f32 desync_ticks;
    f32 desync_amount;
    qbool fake_duck;
    qbool micro_move;
    f32 micro_move_speed;
    qbool spin;
    f32 spin_speed;
    f32 roll;
    qbool fake_lag;
    i32 fake_lag_ticks;
} sh_antiaim_cfg;

typedef struct sh_visuals_cfg
{
    qbool enabled;
    qbool box;
    qbool box_outline;
    qbool name;
    qbool health;
    qbool health_bar;
    qbool armor;
    qbool weapon;
    qbool weapon_icon;
    qbool ammo;
    qbool distance;
    qbool skeleton;
    qbool head_dot;
    qbool snaplines;
    qbool chams;
    qbool chams_team_color;
    qbool chams_xqz;
    qbool sound_esp;
    qbool draw_fakes;
    qbool penetration_info;
    qbool draw_history;
    qbool damage_info;
    qbool damage_hitbox;
    qbool hitboxes;
    col4_t col_hitbox;
    col4_t col_hitbox_head;
    qbool hitbox_vis_check;
    qbool hitbox_scan;
    i32 esp_color_mode;
    qbool fov_enabled;
    f32 fov_value;
    qbool aspect_ratio;
    f32 aspect_value;
    qbool viewmodel_offset;
    f32 viewmodel_x;
    f32 viewmodel_y;
    f32 viewmodel_z;
    qbool viewmodel_fov;
    f32 viewmodel_fov_value;
    qbool no_bob;
    qbool fog;
    f32 fog_density;
    i32 fog_mode;
    qbool hitmarker;
    qbool indicators;
    qbool hitmarker_damage;
    qbool model_changer;
    i32 model_index;
    col4_t col_chams_vis;
    col4_t col_chams_xqz;
    qbool glow;
    f32 glow_amount;
    qbool team_check;
    qbool visible_only;
    f32 max_distance;
    i32 box_style;
    col4_t col_visible;
    col4_t col_hidden;
    col4_t col_name;
    col4_t col_health;
    col4_t col_armor;
    col4_t col_weapon;
    col4_t col_skeleton;
    qbool nightmode;
    qbool skybox_change;
    i32 skybox_index;
    qbool world_color;
    col4_t col_world;
    qbool sky_color;
    col4_t col_sky;
    qbool light_color;
    col4_t col_light;
    qbool tracer;
    col4_t col_tracer;
    qbool impact;
    col4_t col_impact;
    qbool thirdperson;
    f32 thirdperson_dist;
    qbool spectator_list;
    qbool flat_textures;
    qbool hit_sound;
    f32 hit_sound_volume;
    qbool bullet_impact_sound;
} sh_visuals_cfg;

typedef struct sh_misc_cfg
{
    qbool edge_jump;
    qbool fast_ladder;
    qbool no_fall_damage;
    qbool grenade_predict;
    qbool grenade_radius;
    qbool enemy_fire_indicator;
    qbool hideshot;
    qbool auto_pistol;
    qbool knife_bot;
    qbool fast_reload;
    qbool no_recoil;
    qbool no_recoil_effect;
    qbool hide_hud;
    qbool hide_hud_weapons;
    qbool hide_hud_crosshair;
    qbool hide_hud_health;
    qbool hide_hud_timer;
    qbool hide_hud_money;
    qbool no_spread;
    qbool no_flash;
    qbool no_smoke;
    qbool no_scope;
    qbool no_scope_borders;
    qbool crosshair;
    qbool radar_hack;
    qbool radar_rotate;
    qbool radar_height;
} sh_misc_cfg;

typedef struct sh_movement_cfg
{
    qbool autostrafe;
    qbool quick_stop;
    qbool bhop;
    qbool auto_jump;
    qbool super_toss;
    qbool tick_control;
    f32 tick_ms;
    f32 super_toss_power;
} sh_movement_cfg;

typedef struct sh_esp_shared
{
    f32 text_scale;
    i32 font;
    qbool show_local;
} sh_esp_shared;

typedef struct sh_config
{
    qbool ready;
    qbool menu_open;
    sh_aimbot_cfg aimbot;
    sh_antiaim_cfg antiaim;
    sh_visuals_cfg visuals;
    sh_misc_cfg misc;
    sh_movement_cfg movement;
    sh_esp_shared esp;
} sh_config;

extern sh_config g_cfg;

void sh_config_load(void);
void sh_config_save(void);
void sh_config_defaults(void);
