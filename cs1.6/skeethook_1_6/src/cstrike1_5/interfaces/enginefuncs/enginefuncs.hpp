#pragma once

#include <cstdint>

#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/structs/pmtrace.hpp>

namespace enginefuncs
{
    inline uint8_t* table = nullptr;

    auto initialize() -> bool;

    auto point_contents(const vec3_t& point) -> int;
    auto trace_line(const vec3_t& start, const vec3_t& end, int flags, int usehull, int ignore_entity) -> pm_trace_t*;
    auto index_from_trace(pm_trace_t* trace) -> int;
    auto set_trace_hull(int hull) -> void;
    auto push_states() -> void;
    auto pop_states() -> void;
    auto set_up_prediction(int do_prediction, int include_local) -> void;
    auto player_trace(const vec3_t& start, const vec3_t& end, int flags, int ignore_entity) -> pm_trace_t;
}
