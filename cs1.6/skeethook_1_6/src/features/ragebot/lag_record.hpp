#pragma once

#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/structs/cl_entity.hpp>
#include <cstrike1_5/structs/cvar.hpp>

namespace lag_record
{
    inline constexpr int max_players = 33;
    inline constexpr int max_records = 64;

    struct record_t
    {
        vec3_t origin;
        vec3_t angles;
        float simulation_time;
    };

    inline record_t records[max_players][max_records] = {};
    inline record_t empty_record = {};
    inline int record_counts[max_players] = {};
    inline float current_time = 0.0f;

    inline cvar_t* cvar_max_unlag = nullptr;
    inline cvar_t* cvar_unlag_push = nullptr;

    auto resolve_cvars() -> void;
    auto available_window() -> float;
    auto reference_time() -> float;
    auto newest_stamp(cl_entity_t* entity) -> float;
    auto collect(int player_index, cl_entity_t* entity, float window) -> void;
    auto update() -> void;
    auto record_count(int player_index) -> int;
    auto get_record(int player_index, int slot) -> const record_t&;
}
