#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/structs/ref_params.hpp>
#include <cstrike1_5/interfaces/engine/engine.hpp>
#include <cstrike1_5/interfaces/trace/trace.hpp>
#include <features/vars.hpp>

#include <features/world/third_person/third_person.hpp>

auto third_person::apply(ref_params_t* params) -> void
{
    if (!vars::third_person.enabled || !vars::third_person.active)
        return;

    const vec3_t eye = params->vieworg;
    const vec3_t desired = eye - params->forward * vars::third_person.distance;
    const trace_result_t obstruction = trace::line(eye, desired, engine::local_player_index());

    params->vieworg = obstruction.fraction < 1.0f ? obstruction.endpos : desired;
}
