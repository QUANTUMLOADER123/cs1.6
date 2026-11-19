#pragma once

#include <cstddef>
#include <cstdint>

#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/structs/entity_state.hpp>

struct position_history_t
{
    float animtime;
    vec3_t origin;
    vec3_t angles;
};

struct mouth_t
{
    uint8_t mouthopen;
    uint8_t sndcount;
    int sndavg;
};

struct latchedvars_t
{
    float prevanimtime;
    float sequencetime;
    uint8_t prevseqblending[2];
    vec3_t prevorigin;
    vec3_t prevangles;
    int prevsequence;
    float prevframe;
    uint8_t prevcontroller[4];
    uint8_t prevblending[2];
};

struct colorVec
{
    unsigned int r, g, b, a;
};

struct cl_entity_t
{
    int index;
    int player;
    entity_state_t baseline;
    entity_state_t prevstate;
    entity_state_t curstate;
    int current_position;
    position_history_t ph[64];
    mouth_t mouth;
    latchedvars_t latched;
    float lastmove;
    vec3_t origin;
    vec3_t angles;
    float attachment[4][3];
    int trivial_accept;
    void* model;
    void* efrag;
    void* topnode;
    float syncbase;
    int visframe;
    colorVec cvFloorColor;
};

static_assert(sizeof(cl_entity_t) == 3000);
static_assert(offsetof(cl_entity_t, player) == 4);
static_assert(offsetof(cl_entity_t, baseline) == 8);
static_assert(offsetof(cl_entity_t, prevstate) == 348);
static_assert(offsetof(cl_entity_t, curstate) == 688);
static_assert(offsetof(cl_entity_t, current_position) == 1028);
static_assert(offsetof(cl_entity_t, origin) == 2888);
static_assert(offsetof(cl_entity_t, angles) == 2900);
static_assert(offsetof(cl_entity_t, model) == 2964);
