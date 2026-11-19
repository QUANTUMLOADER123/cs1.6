#pragma once

#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/math/angle.hpp>
#include <cstrike1_5/structs/cl_entity.hpp>
#include <cstrike1_5/structs/studio.hpp>
#include <cstrike1_5/interfaces/engine/engine.hpp>
#include <cstrike1_5/interfaces/studio/studio.hpp>
#include <cstrike1_5/interfaces/trace/trace.hpp>
#include <cstrike1_5/interfaces/weapons/weapons.hpp>

class c_base_player
{
public:
    c_base_player() = default;
    explicit c_base_player(cl_entity_t* entity) : base(entity) {}

    auto get_index() -> int { return base ? base->index : 0; }
    auto is_player() -> bool { return base && base->player; }
    auto is_valid() -> bool { return base && base->player && base->origin.is_valid() && base->curstate.iuser1 == 0; }

    auto is_local_player() -> bool { return base && base->index == engine::local_player_index(); }
    auto get_team() -> int { return base ? engine::player_team(base->index) : 0; }

    auto is_enemy() -> bool
    {
        const int local_team = engine::player_team(engine::local_player_index());
        const int team = get_team();
        return team != 0 && team != local_team;
    }

    auto is_alive() -> bool
    {
        if (!base || base->curstate.iuser1 != 0)
            return false;

        return base->curstate.solid == solid_slidebox || base->curstate.solid == solid_bbox;
    }

    auto get_health() -> int { return base ? base->curstate.health : 0; }
    auto get_origin() -> vec3_t { return base ? base->origin : vec3_t(); }
    auto get_angles() -> vec3_t { return base ? base->angles : vec3_t(); }
    auto get_velocity() -> vec3_t { return base ? base->curstate.velocity : vec3_t(); }
    auto get_speed() -> float { return get_velocity().length_2d(); }
    auto get_flags() -> int { return base ? base->curstate.iuser1 : 0; }
    auto is_on_ground() -> bool { return base && base->curstate.onground != 0; }

    auto get_eye_position() -> vec3_t
    {
        if (!base)
            return vec3_t();

        client_state_t* state = engine::client_state();
        if (state && is_local_player())
            return base->origin + state->viewheight;

        return base->origin + vec3_t(0.0f, 0.0f, base->curstate.maxs.z);
    }

    auto get_aim_punch() -> vec3_t
    {
        client_state_t* state = engine::client_state();
        if (!state)
            return vec3_t();

        return state->predicted_frames[state->parsecountmod & 63].client.punchangle;
    }

    auto get_view_angles() -> vec3_t
    {
        client_state_t* state = engine::client_state();
        return state ? state->viewangles : vec3_t();
    }

    auto get_current_weapon() -> c_weapon { return weapons::local(); }
    auto get_current_weapon_id() -> int { return weapons::local().get_id(); }
    auto get_weapon_model_index() -> int { return base ? base->curstate.weaponmodel : 0; }

    auto get_hitbox_count() -> int
    {
        studiohdr_t* header = studio::header(get_index());
        return header ? header->numhitboxes : 0;
    }

    auto get_hitbox_center(int group) -> vec3_t
    {
        studiohdr_t* header = studio::header(get_index());
        if (!header)
            return vec3_t();

        const mstudiobbox_t* boxes = studio::hitbox_list(header);
        if (!boxes)
            return vec3_t();

        for (int index = 0; index < header->numhitboxes; index++)
            if (boxes[index].group == group)
                return studio::hitbox_center(get_index(), boxes[index]);

        return vec3_t();
    }

    auto get_hitbox_bounds(vec3_t& world_min, vec3_t& world_max) -> bool
    {
        return studio::hitbox_bounds(get_index(), world_min, world_max);
    }

    auto get_bone_count() -> int { return studio::bone_count(get_index()); }

    auto get_bone_position(int bone_index) -> vec3_t
    {
        const bone_matrix_t* matrix = studio::bone(get_index(), bone_index);
        return matrix ? matrix->origin() : vec3_t();
    }

    auto is_visible(const vec3_t& from) -> bool { return trace::visible(from, get_eye_position()); }
    auto get_distance(const vec3_t& point) -> float { return (get_origin() - point).length(); }
    auto get_angles_to(const vec3_t& point) -> vec3_t { return math::vector_angles(get_eye_position(), point); }

    cl_entity_t* base = nullptr;
};