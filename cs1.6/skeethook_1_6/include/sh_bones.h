#pragma once

#include "sh_types.h"
#include "sh_sdk.h"

#define sh_max_bones 128
#define sh_max_bone_players 33

enum sh_bone_id
{
    sh_bone_head = 0,
    sh_bone_neck = 1,
    sh_bone_spine3 = 2,
    sh_bone_spine2 = 3,
    sh_bone_spine1 = 4,
    sh_bone_pelvis = 5,
    sh_bone_l_clavicle = 6,
    sh_bone_l_upperarm = 7,
    sh_bone_l_forearm = 8,
    sh_bone_l_hand = 9,
    sh_bone_r_clavicle = 10,
    sh_bone_r_upperarm = 11,
    sh_bone_r_forearm = 12,
    sh_bone_r_hand = 13,
    sh_bone_l_thigh = 14,
    sh_bone_l_calf = 15,
    sh_bone_l_foot = 16,
    sh_bone_r_thigh = 17,
    sh_bone_r_calf = 18,
    sh_bone_r_foot = 19,
    sh_bone_count = 20
};

typedef struct sh_bone_snapshot_s
{
    i32 index;
    i32 count;
    qbool valid;
    f32 born;
    i32 map[sh_bone_count];
    const void *model;
    vec3_t pos[sh_bone_count];
} sh_bone_snapshot_t;

typedef i32 (*sh_bones_draw_orig_fn)(i32 flags, sh_entity_state_t *pplayer);

void sh_bones_studio_set(sh_studio_api_t *api);
void sh_bones_hook_iface(sh_studio_iface_t *iface);
void sh_bones_capture(i32 index);
const sh_bone_snapshot_t *sh_bones_get(i32 index);
void sh_bones_clear(void);
qbool sh_bones_world_to_screen(const sh_bone_snapshot_t *snap, i32 bone, vec2_t *out);
