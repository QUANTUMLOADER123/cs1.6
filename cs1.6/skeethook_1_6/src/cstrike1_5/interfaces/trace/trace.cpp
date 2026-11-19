#include <algorithm>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/structs/pmtrace.hpp>
#include <cstrike1_5/interfaces/engine/engine.hpp>

#include <cstrike1_5/interfaces/enginefuncs/enginefuncs.hpp>
#include <cstrike1_5/interfaces/studio/studio.hpp>
#include <cstrike1_5/interfaces/trace/trace.hpp>

auto trace::point_contents(const vec3_t& point) -> int
{
    return enginefuncs::point_contents(point);
}

auto trace::line(const vec3_t& start, const vec3_t& end, int ignore_entity) -> trace_result_t
{
    trace_result_t result;
    result.endpos = end;

    pm_trace_t* engine_trace = enginefuncs::trace_line(start, end, 0, 2, ignore_entity);
    if (!engine_trace)
        return result;

    result.allsolid = engine_trace->allsolid != 0;
    result.startsolid = engine_trace->startsolid != 0;
    result.inopen = engine_trace->inopen != 0;
    result.inwater = engine_trace->inwater != 0;
    result.fraction = engine_trace->fraction;
    result.endpos = engine_trace->endpos;
    result.plane_normal = engine_trace->plane.normal;
    result.plane_dist = engine_trace->plane.dist;
    result.entity_index = engine_trace->ent;

    return result;
}

auto trace::visible(const vec3_t& start, const vec3_t& end) -> bool
{
    const trace_result_t result = line(start, end);
    return result.fraction >= 1.0f && !result.startsolid;
}

auto trace::solid_travel(const vec3_t& origin, const vec3_t& direction, float maximum) -> float
{
    const float scan = std::min(maximum, 128.0f);
    const vec3_t exit = origin + direction * scan;

    pm_trace_t* back_trace = enginefuncs::trace_line(exit, origin, 0, 2, -1);
    if (!back_trace || back_trace->fraction >= 1.0f || back_trace->startsolid)
        return 0.0f;

    return (back_trace->endpos - origin).length();
}

auto trace::shot(const vec3_t& start, const vec3_t& end, int ignore_entity) -> trace_result_t
{
    trace_result_t result = line(start, end, ignore_entity);

    const int max_clients = engine::max_clients();
    if (max_clients <= 0)
        return result;

    for (int entity_index = 1; entity_index <= max_clients; entity_index++)
    {
        if (entity_index == ignore_entity)
            continue;

        float fraction = 0.0f;
        int hitgroup = 0;

        if (!studio::hitbox_trace(entity_index, start, end, fraction, hitgroup) || fraction >= result.fraction)
            continue;

        result.entity_index = entity_index;
        result.hitgroup = hitgroup;
        result.fraction = fraction;
        result.endpos = start + (end - start) * fraction;
    }

    return result;
}
