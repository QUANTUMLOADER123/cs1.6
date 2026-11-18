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

#define sh_max_bbox 32
#define sh_bone_history 8

typedef struct sh_bone_hist_s
{
    vec3_t pos[sh_bone_count];
    f32 time;
    qbool valid;
} sh_bone_hist_t;

typedef struct sh_studio_bbox_s
{
    i32 bone;
    f32 bbmin[3];
    f32 bbmax[3];
} sh_studio_bbox_t;

typedef struct sh_bbox_snap_s
{
    i32 bone;
    f32 bbmin[3];
    f32 bbmax[3];
} sh_bbox_snap_t;

typedef struct sh_bone_snapshot_s
{
    i32 index;
    i32 count;
    qbool valid;
    f32 born;
    i32 map[sh_bone_count];
    const void *model;
    vec3_t pos[sh_bone_count];
    f32 mat[sh_bone_count][3][4];
    i32 bbox_count;
    sh_bbox_snap_t bbox[sh_max_bbox];
    i32 hist_head;
    sh_bone_hist_t hist[sh_bone_history];
} sh_bone_snapshot_t;

typedef i32 (*sh_bones_draw_orig_fn)(i32 flags, sh_entity_state_t *pplayer);

void sh_bones_studio_set(sh_studio_api_t *api);
void sh_bones_hook_iface(sh_studio_iface_t *iface);
void sh_bones_capture(i32 index);
const sh_bone_snapshot_t *sh_bones_get(i32 index);
void sh_bones_clear(void);
qbool sh_bones_world_to_screen(const sh_bone_snapshot_t *snap, i32 bone, vec2_t *out);
i32 sh_bones_bbox_corners(const sh_bone_snapshot_t *snap, i32 box, vec3_t out[8]);
qbool sh_bones_segment_hitbox(const sh_bone_snapshot_t *snap, i32 box, const vec3_t *start, const vec3_t *end, f32 *out_frac);
qbool sh_bones_backtrack_vec(i32 index, i32 bone, i32 back, vec3_t *out);
i32 sh_bones_backtrack_frames(i32 index);
#define sh_hist_mask 63
#define sh_hist_max_rec 64

qbool sh_backtrack_lerp(i32 ent_index, f32 target_time, vec3_t *out_origin,
                        vec3_t *out_angles, qbool *extrap);
f32 sh_backtrack_time(i32 ent_index, i32 frames_back);
