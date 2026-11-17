#pragma once

#include "sh_types.h"
#include "sh_sdk.h"

enum sh_pitch_mode
{
    sh_pitch_none = 0,
    sh_pitch_down = 1,
    sh_pitch_up = 2,
    sh_pitch_fake_down = 3,
    sh_pitch_fake_up = 4,
    sh_pitch_spin = 5,
    sh_pitch_static = 6
};

enum sh_yaw_mode
{
    sh_yaw_none = 0,
    sh_yaw_backwards = 1,
    sh_yaw_static_left = 2,
    sh_yaw_static_right = 3,
    sh_yaw_left = 4,
    sh_yaw_right = 5,
    sh_yaw_spin = 6,
    sh_yaw_static = 7,
    sh_yaw_random = 8,
    sh_yaw_jitter = 9,
    sh_yaw_fake_sideways = 10,
    sh_yaw_fake_edge = 11
};

qbool sh_antiaim_can_run(sh_usercmd_t *cmd);
void  sh_antiaim_apply_pitch(sh_usercmd_t *cmd, f32 time);
void  sh_antiaim_apply_yaw(sh_usercmd_t *cmd, f32 time);
void  sh_antiaim_movement_fix(sh_usercmd_t *cmd, const vec3_t *old_angles);
void  sh_antiaim_run(sh_usercmd_t *cmd);
f32   sh_antiaim_max_delta(void);
