#pragma once

#include <cstddef>
#include <cstdint>

#include <cstrike1_5/math/vector.hpp>

struct color24
{
    uint8_t r, g, b;
};

struct entity_state_t
{
    int entity_type;
    int number;
    float msg_time;
    int messagenum;
    vec3_t origin;
    vec3_t angles;
    int modelindex;
    int sequence;
    float frame;
    int colormap;
    short skin;
    short solid;
    int effects;
    float scale;
    uint8_t eflags;
    int rendermode;
    int renderamt;
    color24 rendercolor;
    int renderfx;
    int movetype;
    float animtime;
    float framerate;
    int body;
    uint8_t controller[4];
    uint8_t blending[4];
    vec3_t velocity;
    vec3_t mins;
    vec3_t maxs;
    int aiment;
    int owner;
    float friction;
    float gravity;
    int team;
    int playerclass;
    int health;
    int spectator;
    int weaponmodel;
    int gaitsequence;
    vec3_t basevelocity;
    int usehull;
    int oldbuttons;
    int onground;
    int iStepLeft;
    float flFallVelocity;
    float fov;
    int weaponanim;
    vec3_t startpos;
    vec3_t endpos;
    float impacttime;
    float starttime;
    int iuser1;
    int iuser2;
    int iuser3;
    int iuser4;
    float fuser1;
    float fuser2;
    float fuser3;
    float fuser4;
    vec3_t vuser1;
    vec3_t vuser2;
    vec3_t vuser3;
    vec3_t vuser4;
};

static_assert(sizeof(entity_state_t) == 340);
static_assert(offsetof(entity_state_t, origin) == 16);
static_assert(offsetof(entity_state_t, modelindex) == 40);
static_assert(offsetof(entity_state_t, solid) == 58);
static_assert(offsetof(entity_state_t, team) == 164);
static_assert(offsetof(entity_state_t, health) == 172);
