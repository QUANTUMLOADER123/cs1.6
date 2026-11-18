#pragma once

#include "sh_types.h"
#include "sh_sdk.h"

void sh_movement_run(sh_usercmd_t *cmd);

void sh_shots_on_attack(void);
void sh_shots_reset(void);
i32  sh_shots_count(void);
i32  sh_local_weapon_id(void);

extern i32 g_shots_fired;

void sh_fall_predict(sh_playermove_t *pm);
f32 sh_fall_damage_predicted(void);
qbool sh_fall_warning(f32 *damage);

void sh_speedhack_apply(sh_usercmd_t *cmd, sh_playermove_t *pm);
void sh_super_toss(sh_usercmd_t *cmd, sh_playermove_t *pm);

void sh_autostrafe(sh_usercmd_t *cmd, sh_playermove_t *pm);
void sh_quick_stop(sh_usercmd_t *cmd, sh_playermove_t *pm);
void sh_bhop(sh_usercmd_t *cmd, sh_playermove_t *pm);
void sh_auto_jump_hold(sh_usercmd_t *cmd, sh_playermove_t *pm);
f32  sh_strafe_omega(f32 wishspeed);
f32  sh_friction_drop(f32 speed, f32 frametime);
