#pragma once

#include "sh_types.h"
#include "sh_sdk.h"

typedef struct sh_rage_target_s
{
    i32 index;
    f32 distance;
    f32 damage;
    vec3_t aim_point;
    i32 hitgroup;
    qbool through_wall;
} sh_rage_target_t;

typedef struct sh_rage_result_s
{
    qbool found;
    sh_rage_target_t target;
    i32 candidates;
} sh_rage_result_t;

void sh_rage_scan(sh_rage_result_t *out);
qbool sh_rage_should_fire(const sh_rage_result_t *r);
void sh_rage_apply(sh_usercmd_t *cmd, const sh_rage_result_t *r);
qbool sh_rage_double_tap(sh_usercmd_t *cmd, f32 next_attack_time, f32 now);
void sh_rage_apply_recoil(sh_usercmd_t *cmd);

void sh_rage_run(sh_usercmd_t *cmd);
