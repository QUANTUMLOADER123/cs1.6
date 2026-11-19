#pragma once

#include <cstdint>
#include <cstddef>

#include <cstrike1_5/math/vector.hpp>

enum e_hitgroup : int32_t
{
    hitgroup_head = 1,
    hitgroup_chest = 2,
    hitgroup_stomach = 3,
    hitgroup_left_arm = 4,
    hitgroup_right_arm = 5,
    hitgroup_left_leg = 6,
    hitgroup_right_leg = 7,
    hitgroup_weapon = 8,
};

inline auto hitgroup_name(int group) -> const char*
{
    switch (group)
    {
    case hitgroup_head: return "head";
    case hitgroup_chest: return "chest";
    case hitgroup_stomach: return "stomach";
    case hitgroup_left_arm: return "left arm";
    case hitgroup_right_arm: return "right arm";
    case hitgroup_left_leg: return "left leg";
    case hitgroup_right_leg: return "right leg";
    case hitgroup_weapon: return "weapon";
    default: return "?";
    }
}

struct mstudiobone_t
{
    char name[32];
    int parent;
    int flags;
    int bonecontroller[6];
    float value[6];
    float scale[6];
};

static_assert(sizeof(mstudiobone_t) == 112);
static_assert(offsetof(mstudiobone_t, name) == 0);
static_assert(offsetof(mstudiobone_t, parent) == 32);

struct mstudiobbox_t
{
    int bone;
    int group;
    vec3_t bbmin;
    vec3_t bbmax;
};

static_assert(sizeof(mstudiobbox_t) == 32);
static_assert(offsetof(mstudiobbox_t, group) == 4);
static_assert(offsetof(mstudiobbox_t, bbmin) == 8);
static_assert(offsetof(mstudiobbox_t, bbmax) == 20);

struct mstudioseqdesc_t
{
    char label[32];
    float fps;
    int flags;
    int activity;
    int actweight;
    int numevents;
    int eventindex;
    int numframes;
    int numpivots;
    int pivotindex;
    int motiontype;
    int motionbone;
    vec3_t linearmovement;
    int automoveposindex;
    int automoveangleindex;
    vec3_t bbmin;
    vec3_t bbmax;
    int numblends;
    int animindex;
    int blendtype[2];
    float blendstart[2];
    float blendend[2];
    int blendparent;
    int seqgroup;
    int entrynode;
    int exitnode;
    int nodeflags;
    int nextseq;
};

static_assert(sizeof(mstudioseqdesc_t) == 176);
static_assert(offsetof(mstudioseqdesc_t, label) == 0);

struct bone_matrix_t
{
    float value[3][4];

    auto origin() const -> vec3_t { return vec3_t(value[0][3], value[1][3], value[2][3]); }

    auto transform(const vec3_t& local) const -> vec3_t
    {
        return vec3_t(
            value[0][0] * local.x + value[0][1] * local.y + value[0][2] * local.z + value[0][3],
            value[1][0] * local.x + value[1][1] * local.y + value[1][2] * local.z + value[1][3],
            value[2][0] * local.x + value[2][1] * local.y + value[2][2] * local.z + value[2][3]);
    }
};

static_assert(sizeof(bone_matrix_t) == 48);

struct studiohdr_t
{
    int id;
    int version;
    char name[64];
    int length;
    vec3_t eyeposition;
    vec3_t mins;
    vec3_t maxs;
    vec3_t bbmin;
    vec3_t bbmax;
    int flags;
    int numbones;
    int boneindex;
    int numbonecontrollers;
    int bonecontrollerindex;
    int numhitboxes;
    int hitboxindex;
    int numseq;
    int seqindex;
    int numseqgroups;
    int seqgroupindex;
    int numtextures;
    int textureindex;
    int texturedataindex;
    int numskinref;
    int numskinfamilies;
    int skinindex;
    int numbodyparts;
    int bodypartindex;
    int numattachments;
    int attachmentindex;
    int soundtable;
    int soundindex;
    int soundgroups;
    int soundgroupindex;
    int numtransitions;
    int transitionindex;
};

static_assert(offsetof(studiohdr_t, numbones) == 140);
static_assert(offsetof(studiohdr_t, boneindex) == 144);
static_assert(offsetof(studiohdr_t, numhitboxes) == 156);
static_assert(offsetof(studiohdr_t, hitboxindex) == 160);
static_assert(offsetof(studiohdr_t, numseq) == 164);
static_assert(offsetof(studiohdr_t, seqindex) == 168);
static_assert(offsetof(studiohdr_t, numbodyparts) == 204);
static_assert(offsetof(studiohdr_t, bodypartindex) == 208);
static_assert(offsetof(studiohdr_t, numattachments) == 212);
