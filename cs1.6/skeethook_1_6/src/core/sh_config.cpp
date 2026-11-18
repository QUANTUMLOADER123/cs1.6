#include "../../include/sh_config.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_scanner.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>

sh_config g_cfg;
i32 g_cfg_local_team;
i32 (*g_trace_fn)(vec3_t, vec3_t, i32, i32);

void sh_config_defaults(void)
{
    for (u32 i = 0; i < sizeof(g_cfg); ++i)
        ((u8 *)&g_cfg)[i] = 0;

    g_cfg.aimbot.enabled = (qbool)0;
    g_cfg.aimbot.silent = (qbool)0;
    g_cfg.aimbot.auto_fire = (qbool)0;
    g_cfg.aimbot.auto_wall = (qbool)1;
    g_cfg.aimbot.visible_check = (qbool)1;
    g_cfg.aimbot.fov = 8.0f;
    g_cfg.aimbot.smooth = 4.0f;
    g_cfg.aimbot.hitbox = 0;
    g_cfg.aimbot.auto_scope = (qbool)0;
    g_cfg.aimbot.multipoint = 0;
    g_cfg.aimbot.backtrack = 0;
    g_cfg.aimbot.priority = 0;
    g_cfg.aimbot.min_damage = 25.0f;
    g_cfg.aimbot.key = 0;
    g_cfg.aimbot.double_tap = (qbool)0;
    g_cfg.aimbot.penetration = (qbool)1;
    g_cfg.aimbot.auto_stop = (qbool)0;
    g_cfg.aimbot.auto_stop_duck = (qbool)0;

    g_cfg.movement.autostrafe = (qbool)1;
    g_cfg.movement.quick_stop = (qbool)0;
    g_cfg.movement.bhop = (qbool)1;
    g_cfg.movement.speedhack = (qbool)0;
    g_cfg.movement.speedhack_value = 1.5f;
    g_cfg.movement.air_stuck = (qbool)0;
    g_cfg.movement.air_stuck_intensity = 1.0f;
    g_cfg.movement.strafe_hack = (qbool)0;
    g_cfg.movement.strafe_hack_max = 1000.0f;
    g_cfg.movement.strafe_hack_boost = 1.0f;
    g_cfg.movement.ground_strafe = (qbool)0;
    g_cfg.movement.ground_strafe_max = 700.0f;
    g_cfg.movement.auto_jump = (qbool)1;
    g_cfg.movement.super_toss = (qbool)0;
    g_cfg.movement.tick_control = (qbool)0;
    g_cfg.movement.tick_ms = 10.0f;
    g_cfg.movement.super_toss_power = 50.0f;

    g_cfg.visuals.nightmode = (qbool)0;
    g_cfg.visuals.col_world = (col4_t){ 30, 35, 40, 255 };
    g_cfg.visuals.col_sky = (col4_t){ 15, 20, 30, 255 };
    g_cfg.visuals.col_light = (col4_t){ 180, 180, 180, 255 };
    g_cfg.visuals.tracer = (qbool)0;
    g_cfg.visuals.col_tracer = (col4_t){ 255, 80, 80, 200 };
    g_cfg.visuals.impact = (qbool)0;
    g_cfg.visuals.col_impact = (col4_t){ 255, 255, 255, 200 };
    g_cfg.visuals.thirdperson = (qbool)0;
    g_cfg.visuals.thirdperson_dist = 150.0f;
    g_cfg.visuals.spectator_list = (qbool)0;
    g_cfg.visuals.flat_textures = (qbool)0;
    g_cfg.visuals.hit_sound = (qbool)0;
    g_cfg.visuals.hit_sound_volume = 1.0f;
    g_cfg.visuals.bullet_impact_sound = (qbool)0;

    g_cfg.antiaim.enabled = (qbool)0;
    g_cfg.antiaim.pitch_mode = 0;
    g_cfg.antiaim.yaw_mode = 0;
    g_cfg.antiaim.yaw_base = 0.0f;
    g_cfg.antiaim.yaw_jitter = 25.0f;
    g_cfg.antiaim.desync = (qbool)1;
    g_cfg.antiaim.desync_amount = 58.0f;
    g_cfg.antiaim.desync_ticks = 2.0f;
    g_cfg.antiaim.roll = 0.0f;
    g_cfg.antiaim.fake_lag = (qbool)0;
    g_cfg.antiaim.fake_lag_ticks = 6;
    g_cfg.antiaim.fake_duck = (qbool)0;
    g_cfg.antiaim.micro_move = (qbool)0;
    g_cfg.antiaim.micro_move_speed = 1.0f;
    g_cfg.antiaim.spin = (qbool)0;
    g_cfg.antiaim.spin_speed = 1.0f;

    g_cfg.visuals.enabled = (qbool)1;
    g_cfg.visuals.box = (qbool)1;
    g_cfg.visuals.box_outline = (qbool)1;
    g_cfg.visuals.name = (qbool)1;
    g_cfg.visuals.health = (qbool)1;
    g_cfg.visuals.health_bar = (qbool)1;
    g_cfg.visuals.armor = (qbool)0;
    g_cfg.visuals.weapon = (qbool)1;
    g_cfg.visuals.weapon_icon = (qbool)0;
    g_cfg.visuals.ammo = (qbool)0;
    g_cfg.visuals.distance = (qbool)1;
    g_cfg.visuals.skeleton = (qbool)0;
    g_cfg.visuals.head_dot = (qbool)1;
    g_cfg.visuals.snaplines = (qbool)0;
    g_cfg.visuals.chams = (qbool)0;
    g_cfg.visuals.chams_xqz = (qbool)1;
    g_cfg.visuals.sound_esp = (qbool)0;
    g_cfg.visuals.draw_fakes = (qbool)1;
    g_cfg.visuals.penetration_info = (qbool)0;
    g_cfg.visuals.draw_history = (qbool)0;
    g_cfg.visuals.damage_info = (qbool)0;
    g_cfg.visuals.damage_hitbox = (qbool)1;
    g_cfg.visuals.hitboxes = (qbool)0;
    g_cfg.visuals.col_hitbox = (col4_t)SH_COL_RGBA(255, 255, 255, 160);
    g_cfg.visuals.col_hitbox_head = (col4_t)SH_COL_RGBA(255, 80, 80, 200);
    g_cfg.visuals.hitbox_vis_check = (qbool)0;
    g_cfg.visuals.hitbox_scan = (qbool)0;
    g_cfg.visuals.esp_color_mode = 0;
    g_cfg.visuals.fov_enabled = (qbool)0;
    g_cfg.visuals.fov_value = 90.0f;
    g_cfg.visuals.aspect_ratio = (qbool)0;
    g_cfg.visuals.aspect_value = 1.333f;
    g_cfg.visuals.viewmodel_offset = (qbool)0;
    g_cfg.visuals.viewmodel_x = 0.0f;
    g_cfg.visuals.viewmodel_y = 0.0f;
    g_cfg.visuals.viewmodel_z = 0.0f;
    g_cfg.visuals.viewmodel_fov = (qbool)0;
    g_cfg.visuals.viewmodel_fov_value = 90.0f;
    g_cfg.visuals.no_bob = (qbool)0;
    g_cfg.visuals.fog = (qbool)0;
    g_cfg.visuals.entity_esp = (qbool)0;
    g_cfg.visuals.smoke_edit = (qbool)0;
    g_cfg.visuals.lightmap_mode = (qbool)0;
    g_cfg.visuals.lightmap_r = 0.45f;
    g_cfg.visuals.lightmap_g = 0.52f;
    g_cfg.visuals.lightmap_b = 0.85f;
    g_cfg.visuals.smoke_alpha = 0.45f;
    g_cfg.visuals.smoke_r = 170;
    g_cfg.visuals.smoke_g = 170;
    g_cfg.visuals.smoke_b = 170;
    g_cfg.visuals.fog_density = 0.005f;
    g_cfg.visuals.fog_mode = 0;
    g_cfg.visuals.fog_r = 140;
    g_cfg.visuals.fog_g = 153;
    g_cfg.visuals.fog_b = 173;
    g_cfg.visuals.hitmarker = (qbool)1;
    g_cfg.visuals.indicators = (qbool)1;
    g_cfg.visuals.hitmarker_damage = (qbool)1;
    g_cfg.visuals.model_changer = (qbool)0;
    g_cfg.visuals.model_index = 0;
    g_cfg.visuals.col_chams_vis = (col4_t)SH_COL_RGBA(120, 255, 120, 180);
    g_cfg.visuals.col_chams_xqz = (col4_t)SH_COL_RGBA(255, 120, 120, 180);
    g_cfg.visuals.chams_team_color = (qbool)1;
    g_cfg.visuals.glow_amount = 60.0f;
    g_cfg.visuals.glow = (qbool)0;
    g_cfg.visuals.team_check = (qbool)1;
    g_cfg.visuals.visible_only = (qbool)0;
    g_cfg.visuals.max_distance = 4096.0f;
    g_cfg.visuals.box_style = 1;
    g_cfg.visuals.col_visible = (col4_t)SH_COL_RGBA(120, 255, 120, 255);
    g_cfg.visuals.col_hidden = (col4_t)SH_COL_RGBA(255, 90, 90, 255);
    g_cfg.visuals.col_name = (col4_t)SH_COL_RGBA(235, 235, 235, 255);
    g_cfg.visuals.col_health = (col4_t)SH_COL_RGBA(120, 255, 120, 255);
    g_cfg.visuals.col_armor = (col4_t)SH_COL_RGBA(80, 160, 255, 255);
    g_cfg.visuals.col_weapon = (col4_t)SH_COL_RGBA(200, 200, 200, 255);
    g_cfg.visuals.col_skeleton = (col4_t)SH_COL_RGBA(255, 255, 255, 220);

    g_cfg.misc.edge_jump = (qbool)0;
    g_cfg.misc.fast_ladder = (qbool)0;
    g_cfg.misc.no_fall_damage = (qbool)0;
    g_cfg.misc.grenade_predict = (qbool)0;
    g_cfg.misc.grenade_radius = (qbool)1;
    g_cfg.misc.enemy_fire_indicator = (qbool)1;
    g_cfg.misc.hideshot = (qbool)0;
    g_cfg.misc.auto_pistol = (qbool)0;
    g_cfg.misc.knife_bot = (qbool)0;
    g_cfg.misc.fast_reload = (qbool)0;
    g_cfg.misc.no_recoil = (qbool)0;
    g_cfg.misc.no_recoil_effect = (qbool)0;
    g_cfg.misc.hide_hud = (qbool)0;
    g_cfg.misc.hide_hud_weapons = (qbool)0;
    g_cfg.misc.hide_hud_crosshair = (qbool)1;
    g_cfg.misc.hide_hud_health = (qbool)0;
    g_cfg.misc.hide_hud_timer = (qbool)0;
    g_cfg.misc.hide_hud_money = (qbool)0;
    g_cfg.misc.no_spread = (qbool)0;
    g_cfg.misc.no_flash = (qbool)0;
    g_cfg.misc.no_smoke = (qbool)0;
    g_cfg.misc.no_scope = (qbool)0;
    g_cfg.misc.no_scope_borders = (qbool)0;
    g_cfg.misc.crosshair = (qbool)1;
    g_cfg.misc.radar_hack = (qbool)0;
    g_cfg.misc.radar_rotate = (qbool)1;
    g_cfg.misc.radar_height = (qbool)1;

    g_cfg.esp.text_scale = 1.0f;
    g_cfg.esp.font = 0;
    g_cfg.esp.show_local = (qbool)0;

    g_cfg.ready = (qbool)1;
}

static void sh_config_path(char *out, i32 cap)
{
    char dir[MAX_PATH];
    GetModuleFileNameA(0, dir, MAX_PATH);

    i32 i = 0;
    i32 last = -1;
    while (dir[i] && i < MAX_PATH)
    {
        if (dir[i] == '\\')
            last = i;
        ++i;
    }
    if (last >= 0)
        dir[last + 1] = 0;

    snprintf(out, cap, "%s%s.cfg", dir, SH_NAME);
}

void sh_config_save(void)
{
    char path[MAX_PATH];
    sh_config_path(path, MAX_PATH);

    FILE *f = fopen(path, "wb");
    if (!f)
        return;

    u32 magic = 0x53484B31u;
    u32 size = (u32)sizeof(g_cfg);

    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&size, sizeof(size), 1, f);
    fwrite(&g_cfg, sizeof(g_cfg), 1, f);
    fclose(f);
}

void sh_config_load(void)
{
    char path[MAX_PATH];
    sh_config_path(path, MAX_PATH);

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        sh_config_defaults();
        return;
    }

    u32 magic = 0;
    u32 size = 0;

    if (fread(&magic, sizeof(magic), 1, f) != 1 ||
        fread(&size, sizeof(size), 1, f) != 1 ||
        magic != 0x53484B31u || size != (u32)sizeof(sh_config))
    {
        fclose(f);
        sh_config_defaults();
        sh_config_save();
        return;
    }

    sh_config tmp;
    size_t got = fread(&tmp, 1, sizeof(tmp), f);
    fclose(f);

    if (got != sizeof(tmp))
    {
        sh_config_defaults();
        return;
    }

    g_cfg = tmp;
    g_cfg.ready = (qbool)1;
}
