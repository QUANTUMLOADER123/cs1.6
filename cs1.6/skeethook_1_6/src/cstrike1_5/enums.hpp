#pragma once

#include <cstdint>

enum e_flags : int32_t
{
    fl_onground = (1 << 0),
    fl_ducking = (1 << 1),
    fl_waterjump = (1 << 2),
    fl_ontrain = (1 << 3),
    fl_intrain = (1 << 4),
    fl_frozen = (1 << 5),
    fl_atcontrols = (1 << 6),
    fl_client = (1 << 7),
    fl_fakeclient = (1 << 8),
    fl_inwater = (1 << 9),
};

enum e_buttons : int32_t
{
    in_attack = (1 << 0),
    in_jump = (1 << 1),
    in_duck = (1 << 2),
    in_forward = (1 << 3),
    in_back = (1 << 4),
    in_use = (1 << 5),
    in_cancel = (1 << 6),
    in_left = (1 << 7),
    in_right = (1 << 8),
    in_moveleft = (1 << 9),
    in_moveright = (1 << 10),
    in_attack2 = (1 << 11),
    in_run = (1 << 12),
    in_reload = (1 << 13),
    in_alt1 = (1 << 14),
    in_score = (1 << 15),
};

enum e_effects : int32_t
{
    ef_nodraw = (1 << 7),
};

enum e_gait : int32_t
{
    gait_stand = 1,
    gait_crouch_idle = 2,
    gait_walk = 3,
    gait_run = 4,
    gait_crouch_move = 5,
    gait_air = 6,
};

enum e_contents : int32_t
{
    contents_empty = -1,
    contents_solid = -2,
    contents_water = -3,
    contents_slime = -4,
    contents_lava = -5,
    contents_sky = -6,
    contents_origin = -7,
    contents_clip = -8,
    contents_current_0 = -9,
    contents_current_90 = -14,
    contents_translucent = -15,
};

enum e_move_type : int32_t
{
    movetype_none = 0,
    movetype_walk = 3,
    movetype_step = 4,
    movetype_fly = 5,
    movetype_toss = 6,
    movetype_push = 7,
    movetype_noclip = 8,
    movetype_flymissile = 9,
    movetype_bounce = 10,
};

enum e_solid : int32_t
{
    solid_not = 0,
    solid_trigger = 1,
    solid_bbox = 2,
    solid_slidebox = 3,
    solid_bsp = 4,
};

enum e_weapon_id : int32_t
{
    weapon_p228 = 1,
    weapon_glock = 2,
    weapon_scout = 3,
    weapon_hegrenade = 4,
    weapon_xm1014 = 5,
    weapon_c4 = 6,
    weapon_mac10 = 7,
    weapon_aug = 8,
    weapon_smokegrenade = 9,
    weapon_elite = 10,
    weapon_fiveseven = 11,
    weapon_ump45 = 12,
    weapon_sg550 = 13,
    weapon_galil = 14,
    weapon_famas = 15,
    weapon_usp = 16,
    weapon_glock18 = 17,
    weapon_awp = 18,
    weapon_mp5navy = 19,
    weapon_m249 = 20,
    weapon_m3 = 21,
    weapon_m4a1 = 22,
    weapon_tmp = 23,
    weapon_g3sg1 = 24,
    weapon_flashbang = 25,
    weapon_deagle = 26,
    weapon_sg552 = 27,
    weapon_ak47 = 28,
    weapon_knife = 29,
    weapon_p90 = 30,
};
