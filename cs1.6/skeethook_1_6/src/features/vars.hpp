#pragma once

namespace vars
{
    struct esp_t
    {
        bool enabled = true;
        bool box = true;
        bool corners = false;
        bool name = true;
        bool weapon = false;
        bool weapon_icons = false;
        bool distance = false;
        bool skeleton = false;
        bool only_visible = false;
        bool enemies_only = true;
    };

    struct chams_style_t
    {
        bool enabled = false;
        int material = 0;
        bool ignore_z = false;
        float visible_color[4] = { 1.0f, 0.5f, 0.0f, 1.0f };
        float occluded_color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    };

    struct chams_t
    {
        bool enabled = false;
        int material = 0;
        bool ignore_z = false;
        bool enemies_only = true;
        float visible_color[4] = { 1.0f, 0.5f, 0.0f, 1.0f };
        float occluded_color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
        chams_style_t local;
        chams_style_t hands;
    };

    struct killsound_t
    {
        bool enabled = false;
        int sound = 0;
    };

    struct misc_t
    {
        bool bunny_hop = false;
        bool autostrafe = false;
        bool quick_stop = false;
        bool ground_strafe = false;
        bool jumpbug = false;
        bool texturebug = false;
        float texturebug_upmove = -12.7f;
        int autostrafe_speed = 3;
        killsound_t killsound;
    };

    struct third_person_t
    {
        bool enabled = false;
        bool active = false;
        float distance = 150.0f;
    };

    struct nightmode_t
    {
        bool enabled = false;
        float color[3] = { 0.15f, 0.15f, 0.25f };
    };

    struct skybox_t
    {
        bool enabled = false;
        float color[3] = { 1.0f, 1.0f, 1.0f };
        bool changer = false;
        char name[32] = {};
    };

    struct aspect_ratio_t
    {
        bool enabled = false;
        float aspect = 4.0f / 3.0f;
    };

    struct viewmodel_t
    {
        bool enabled = false;
        float fov = 75.0f;
        float offset_x = 0.0f;
        float offset_y = 0.0f;
        float offset_z = 0.0f;
    };

    struct fov_changer_t
    {
        bool enabled = false;
        float fov = 90.0f;
    };

    struct antihit_t
    {
        bool enabled = false;
        bool fake_lag = false;
        int fake_lag_ticks = 6;
        int fake_lag_type = 1;
        int fake_lag_move = 0;
        float fake_lag_variance = 100.0f;
        bool fake_lag_while_shooting = false;
        int type = 0;
        float yaw = 0.0f;
        float pitch = 0.0f;
        float jitter_amount = 1.0f;
        float spin_speed = 15.0f;
    };

    struct removals_t
    {
        bool toggles[6] = { false, false, false, false, false, false };
        bool custom_scope = false;
        float custom_scope_size = 1.0f;
    };

    struct model_changer_t
    {
        bool enabled = false;
        int type = 0;
        int model = 0;
        char path[260] = {};
    };

    struct bullet_tracers_t
    {
        bool enabled = false;
        int color_index = 4;
        float life = 0.8f;
        float speed = 6000.0f;
        float length = 0.8f;
        float color[4] = { 1.0f, 0.85f, 0.15f, 1.0f };
    };

    struct bullet_impacts_t
    {
        bool enabled = false;
        float color[4] = { 0.2f, 0.5f, 1.0f, 1.0f };
    };

    struct ragebot_t
    {
        bool enabled = false;
        float hit_chance = 30.0f;
        float minimum_damage = 25.0f;
        bool hitbox[8] = { true, true, true, true, true, true, true, false };
        bool autostop[4] = {};
        bool autoscope = false;
        bool penetration = false;
        bool backtrack = false;
        float backtrack_ms = 200.0f;
        bool nospread = false;
    };

    struct legitbot_t
    {
        bool enabled = false;
        int activation = 0;
        bool hitbox[8] = { true, true, true, true, true, true, true, false };
        float fov = 30.0f;
        float smooth = 5.0f;
        bool draw_fov = false;
        bool rcs = false;
    };

    struct anim_breakers_t
    {
        bool toggles[2] = { false, false };
    };

    inline esp_t esp;
    inline chams_t chams;
    inline misc_t misc;
    inline third_person_t third_person;
    inline nightmode_t nightmode;
    inline skybox_t skybox;
    inline aspect_ratio_t aspect_ratio;
    inline viewmodel_t viewmodel;
    inline fov_changer_t fov_changer;
    inline antihit_t antihit;
    inline removals_t removals;
    inline model_changer_t model_changer;
    inline bullet_tracers_t bullet_tracers;
    inline bullet_impacts_t bullet_impacts;
    inline ragebot_t ragebot;
    inline legitbot_t legitbot;
    inline anim_breakers_t anim_breakers;
}
