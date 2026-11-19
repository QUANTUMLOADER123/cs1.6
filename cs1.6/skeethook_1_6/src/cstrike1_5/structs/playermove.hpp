#pragma once

#include <cstddef>
#include <cstdint>

#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/structs/movevars.hpp>

struct playermove_t
{
    uint8_t reserved_0[92];
    vec3_t velocity;
    uint8_t reserved_1[220 - 104];
    int32_t movetype;
    int32_t onground;
    int32_t waterlevel;
    uint8_t reserved_2[500 - 232];
    float maxspeed;
    uint8_t reserved_3[324848 - 504];
    movevars_t* movevars;
};

static_assert(offsetof(playermove_t, velocity) == 92);
static_assert(offsetof(playermove_t, movetype) == 220);
static_assert(offsetof(playermove_t, onground) == 224);
static_assert(offsetof(playermove_t, waterlevel) == 228);
static_assert(offsetof(playermove_t, maxspeed) == 500);
static_assert(offsetof(playermove_t, movevars) == 324848);
