#pragma once

#include "sh_types.h"
#include "sh_sdk.h"

#define sh_max_bones 128

enum sh_bone_id
{
    sh_bone_pelvis = 0,
    sh_bone_spine = 1,
    sh_bone_spine1 = 2,
    sh_bone_neck = 3,
    sh_bone_head = 4,
    sh_bone_l_upperarm = 5,
    sh_bone_l_forearm = 6,
    sh_bone_l_hand = 7,
    sh_bone_r_upperarm = 8,
    sh_bone_r_forearm = 9,
    sh_bone_r_hand = 10,
    sh_bone_l_thigh = 11,
    sh_bone_l_calf = 12,
    sh_bone_l_foot = 13,
    sh_bone_r_thigh = 14,
    sh_bone_r_calf = 15,
    sh_bone_r_foot = 16
};

typedef struct sh_bone_snapshot_s
{
    i32 index;
    i32 count;
    qbool valid;
    f32 born;
    vec3_t pos[sh_max_bones];
} sh_bone_snapshot_t;

void sh_bones_studio_set(sh_studio_api_t *api);
void sh_bones_capture(i32 index);
const sh_bone_snapshot_t *sh_bones_get(i32 index);
void sh_bones_clear(void);
qbool sh_bones_world_to_screen(const sh_bone_snapshot_t *snap, i32 bone, vec2_t *out);
