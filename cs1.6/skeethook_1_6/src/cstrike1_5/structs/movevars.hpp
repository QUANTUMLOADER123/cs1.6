#pragma once

#include <cstddef>
#include <cstdint>

struct movevars_t
{
    float gravity;
    float stopspeed;
    float maxspeed;
    float spectatormaxspeed;
    float accelerate;
    float airaccelerate;
    float wateraccelerate;
    float friction;
    float edgefriction;
    float waterfriction;
    float entgravity;
    float bounce;
    float stepsize;
    float maxvelocity;
    float zmax;
    float waveHeight;
    int32_t footsteps;
    char skyName[32];
    float rollangle;
    float rollspeed;
    float skycolor_r;
    float skycolor_g;
    float skycolor_b;
    float skyvec_x;
    float skyvec_y;
    float skyvec_z;
};

static_assert(offsetof(movevars_t, maxspeed) == 8);
static_assert(offsetof(movevars_t, airaccelerate) == 20);
static_assert(offsetof(movevars_t, skyName) == 68);
