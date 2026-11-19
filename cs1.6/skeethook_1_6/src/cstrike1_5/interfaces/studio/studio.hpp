#pragma once

#include <cstrike1_5/structs/cl_entity.hpp>
#include <cstrike1_5/structs/entity_state.hpp>
#include <cstrike1_5/structs/studio.hpp>

namespace studio
{
    inline cl_entity_t** current_entity_global = nullptr;
    inline studiohdr_t** studiohdr_global = nullptr;
    inline bone_matrix_t* bone_matrices = nullptr;

    inline uint8_t* studio_api_global = nullptr;
    inline uint8_t* studio_interface = nullptr;
    inline int (__cdecl* draw_player_original)(int flags, entity_state_t* pplayer) = nullptr;

    inline studiohdr_t* headers[33] = {};
    inline bone_matrix_t bones[33][128] = {};
    inline int bone_counts[33] = {};
    inline int frame_bone_counts[33] = {};
    inline bool captured[33] = {};
    inline int frame_counter = 0;
    inline int last_visible_frame[33] = {};

    auto initialize() -> bool;

    auto set_draw_player(int (__cdecl* detour)(int flags, entity_state_t* pplayer)) -> bool;
    auto restore_draw_player() -> void;

    auto invert_rotation(const bone_matrix_t* matrix, float inverse[3][3]) -> bool;
    auto to_local(const float inverse[3][3], const vec3_t& origin, const vec3_t& point) -> vec3_t;
    auto segment_vs_hitbox(const bone_matrix_t* matrix, const mstudiobbox_t& hitbox, const vec3_t& start, const vec3_t& end, float& fraction) -> bool;

    auto begin_frame() -> void;
    auto capture() -> void;

    auto current_entity() -> cl_entity_t*;
    auto has_model(int entity_index) -> bool;
    auto header(int entity_index) -> studiohdr_t*;
    auto bone_list(studiohdr_t* header) -> mstudiobone_t*;
    auto hitbox_list(studiohdr_t* header) -> mstudiobbox_t*;
    auto bone_count(int entity_index) -> int;
    auto bone(int entity_index, int bone_index) -> const bone_matrix_t*;
    auto hitbox_center(int entity_index, const mstudiobbox_t& hitbox) -> vec3_t;
    auto hitbox_bounds(int entity_index, vec3_t& world_min, vec3_t& world_max) -> bool;
    auto hitbox_hit(int entity_index, const vec3_t& start, const vec3_t& end, vec3_t& point) -> bool;
    auto hitbox_trace(int entity_index, const vec3_t& start, const vec3_t& end, float& fraction, int& hitgroup) -> bool;
}