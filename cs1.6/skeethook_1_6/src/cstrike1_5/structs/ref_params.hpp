#pragma once

#include <cstddef>

#include <cstrike1_5/math/vector.hpp>

struct usercmd_t;
struct movevars_t;

struct ref_params_t
{
    vec3_t vieworg;
    vec3_t viewangles;
    vec3_t forward;
    vec3_t right;
    vec3_t up;
    float frametime;
    float time;
    int intermission;
    int paused;
    int spectator;
    int onground;
    int waterlevel;
    vec3_t simvel;
    vec3_t simorg;
    vec3_t viewheight;
    float idealpitch;
    vec3_t cl_viewangles;
    int health;
    vec3_t crosshairangle;
    float viewsize;
    vec3_t punchangle;
    int maxclients;
    int viewentity;
    int playernum;
    int max_entities;
    int demoplayback;
    int hardware;
    int smoothing;
    usercmd_t* cmd;
    movevars_t* movevars;
    int viewport[4];
    int next_view;
    int only_client_draw;
};

static_assert(sizeof(ref_params_t) == 232);
static_assert(offsetof(ref_params_t, intermission) == 68);
static_assert(offsetof(ref_params_t, cl_viewangles) == 128);
static_assert(offsetof(ref_params_t, punchangle) == 160);
static_assert(offsetof(ref_params_t, viewport) == 208);
