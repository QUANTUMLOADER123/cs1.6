#include <cstdint>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/address.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/structs/pmtrace.hpp>

#include <cstrike1_5/interfaces/enginefuncs/enginefuncs.hpp>

namespace enginefuncs
{
    using point_contents_fn = int(__cdecl*)(const float*, int*);
    using trace_line_fn = pm_trace_t*(__cdecl*)(const float*, const float*, int, int, int);

    using index_from_trace_fn = int(__cdecl*)(pm_trace_t*);
    using set_up_prediction_fn = void(__cdecl*)(int, int);
    using push_states_fn = void(__cdecl*)();
    using pop_states_fn = void(__cdecl*)();
    using set_solid_players_fn = void(__cdecl*)(int);
    using set_trace_hull_fn = void(__cdecl*)(int);
    using player_trace_fn = void(__cdecl*)(const float*, const float*, int, int, pm_trace_t*);

    auto slot(int index) -> void*
    {
        return *reinterpret_cast<void**>(table + index * 4);
    }

    auto event_api() -> uint8_t*
    {
        return *reinterpret_cast<uint8_t**>(table + 84 * 4);
    }

    auto event_slot(int byte_index) -> void*
    {
        return *reinterpret_cast<void**>(event_api() + byte_index);
    }
}

auto enginefuncs::initialize() -> bool
{
    uint8_t* match = memory::find_pattern(modules::client, patterns::engine_funcs);
    if (!match)
        return false;

    table = memory::global_address(match, 1);

    return table && event_api();
}

auto enginefuncs::point_contents(const vec3_t& point) -> int
{
    return reinterpret_cast<point_contents_fn>(slot(57))(&point.x, nullptr);
}

auto enginefuncs::trace_line(const vec3_t& start, const vec3_t& end, int flags, int usehull, int ignore_entity) -> pm_trace_t*
{
    return reinterpret_cast<trace_line_fn>(slot(59))(&start.x, &end.x, flags, usehull, ignore_entity);
}

auto enginefuncs::index_from_trace(pm_trace_t* trace) -> int
{
    return reinterpret_cast<index_from_trace_fn>(event_slot(32))(trace);
}

auto enginefuncs::set_trace_hull(int hull) -> void
{
    reinterpret_cast<set_trace_hull_fn>(event_slot(56))(hull);
}

auto enginefuncs::push_states() -> void
{
    reinterpret_cast<push_states_fn>(event_slot(44))();
}

auto enginefuncs::pop_states() -> void
{
    reinterpret_cast<pop_states_fn>(event_slot(48))();
}

auto enginefuncs::set_up_prediction(int do_prediction, int include_local) -> void
{
    reinterpret_cast<set_up_prediction_fn>(event_slot(40))(do_prediction, include_local);
}

auto enginefuncs::player_trace(const vec3_t& start, const vec3_t& end, int flags, int ignore_entity) -> pm_trace_t
{
    pm_trace_t trace = {};
    reinterpret_cast<player_trace_fn>(event_slot(60))(&start.x, &end.x, flags, ignore_entity, &trace);
    return trace;
}
