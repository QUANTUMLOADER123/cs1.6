#pragma once

#include <cstddef>
#include <cstdint>

#include <cstrike1_5/math/vector.hpp>

struct pm_plane_t
{
    vec3_t normal;
    float dist;
};

struct pm_trace_t
{
    int32_t allsolid;
    int32_t startsolid;
    int32_t inopen;
    int32_t inwater;
    float fraction;
    vec3_t endpos;
    pm_plane_t plane;
    int32_t ent;
    vec3_t deltavelocity;
    int32_t hitgroup;
};

static_assert(offsetof(pm_trace_t, fraction) == 16);
static_assert(offsetof(pm_trace_t, endpos) == 20);
static_assert(offsetof(pm_trace_t, ent) == 48);
static_assert(offsetof(pm_trace_t, hitgroup) == 64);
