#pragma once

#include <cstrike1_5/structs/cl_entity.hpp>

class c_base_entity
{
public:
    c_base_entity() = default;
    explicit c_base_entity(cl_entity_t* entity) : base(entity) {}

    auto get_index() -> int { return base ? base->index : 0; }
    auto get_origin() -> vec3_t& { return base->origin; }
    auto get_angles() -> vec3_t& { return base->angles; }

    cl_entity_t* base = nullptr;
};
