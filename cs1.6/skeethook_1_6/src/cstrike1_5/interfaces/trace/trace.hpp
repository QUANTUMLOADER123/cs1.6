#pragma once

#include <cstrike1_5/math/vector.hpp>

struct trace_result_t
{
    bool allsolid = false;
    bool startsolid = false;
    bool inopen = false;
    bool inwater = false;
    float fraction = 1.0f;
    vec3_t endpos;
    vec3_t plane_normal;
    float plane_dist = 0.0f;
    int entity_index = 0;
    int hitgroup = 0;
};

namespace trace
{
    auto line(const vec3_t& start, const vec3_t& end, int ignore_entity = -1) -> trace_result_t;
    auto visible(const vec3_t& start, const vec3_t& end) -> bool;
    auto point_contents(const vec3_t& point) -> int;
    auto solid_travel(const vec3_t& origin, const vec3_t& direction, float maximum) -> float;
    auto shot(const vec3_t& start, const vec3_t& end, int ignore_entity) -> trace_result_t;
}
