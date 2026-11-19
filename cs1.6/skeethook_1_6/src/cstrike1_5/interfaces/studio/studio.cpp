#include <cstdint>
#include <cmath>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/address.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/structs/cl_entity.hpp>
#include <cstrike1_5/structs/entity_state.hpp>
#include <cstrike1_5/structs/studio.hpp>

#include <cstrike1_5/interfaces/studio/studio.hpp>

namespace studio
{
    auto invert_rotation(const bone_matrix_t* matrix, float inverse[3][3]) -> bool
    {
        const float a = matrix->value[0][0], b = matrix->value[0][1], c = matrix->value[0][2];
        const float d = matrix->value[1][0], e = matrix->value[1][1], f = matrix->value[1][2];
        const float g = matrix->value[2][0], h = matrix->value[2][1], i = matrix->value[2][2];

        const float determinant = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
        if (std::fabs(determinant) < 1e-6f)
            return false;

        const float scale = 1.0f / determinant;
        inverse[0][0] = (e * i - f * h) * scale;
        inverse[0][1] = (c * h - b * i) * scale;
        inverse[0][2] = (b * f - c * e) * scale;
        inverse[1][0] = (f * g - d * i) * scale;
        inverse[1][1] = (a * i - c * g) * scale;
        inverse[1][2] = (c * d - a * f) * scale;
        inverse[2][0] = (d * h - e * g) * scale;
        inverse[2][1] = (b * g - a * h) * scale;
        inverse[2][2] = (a * e - b * d) * scale;

        return true;
    }

    auto to_local(const float inverse[3][3], const vec3_t& origin, const vec3_t& point) -> vec3_t
    {
        const vec3_t offset = point - origin;
        return vec3_t(
            inverse[0][0] * offset.x + inverse[0][1] * offset.y + inverse[0][2] * offset.z,
            inverse[1][0] * offset.x + inverse[1][1] * offset.y + inverse[1][2] * offset.z,
            inverse[2][0] * offset.x + inverse[2][1] * offset.y + inverse[2][2] * offset.z);
    }

    auto segment_vs_hitbox(const bone_matrix_t* matrix, const mstudiobbox_t& hitbox, const vec3_t& start, const vec3_t& end, float& fraction) -> bool
    {
        float inverse[3][3];
        if (!invert_rotation(matrix, inverse))
            return false;

        const vec3_t origin = matrix->origin();
        const vec3_t local_start = to_local(inverse, origin, start);
        const vec3_t local_end = to_local(inverse, origin, end);
        const vec3_t direction = local_end - local_start;

        const float bounds_min[3] = { hitbox.bbmin.x, hitbox.bbmin.y, hitbox.bbmin.z };
        const float bounds_max[3] = { hitbox.bbmax.x, hitbox.bbmax.y, hitbox.bbmax.z };
        const float ray_origin[3] = { local_start.x, local_start.y, local_start.z };
        const float ray_direction[3] = { direction.x, direction.y, direction.z };

        float entry = 0.0f;
        float exit = 1.0f;

        for (int axis = 0; axis < 3; axis++)
        {
            if (std::fabs(ray_direction[axis]) < 1e-6f)
            {
                if (ray_origin[axis] < bounds_min[axis] || ray_origin[axis] > bounds_max[axis])
                    return false;

                continue;
            }

            const float inverse_direction = 1.0f / ray_direction[axis];
            float first = (bounds_min[axis] - ray_origin[axis]) * inverse_direction;
            float second = (bounds_max[axis] - ray_origin[axis]) * inverse_direction;
            if (first > second)
                std::swap(first, second);

            entry = first > entry ? first : entry;
            exit = second < exit ? second : exit;

            if (entry > exit)
                return false;
        }

        fraction = entry;
        return true;
    }

    auto initialize() -> bool
    {
        static bool initialized = false;
        if (initialized)
            return true;

        uint8_t* globals_match = memory::find_pattern(modules::renderer, patterns::bone_globals);
        uint8_t* matrices_match = memory::find_pattern(modules::renderer, patterns::bone_matrices);
        uint8_t* api_match = memory::find_pattern(modules::renderer, patterns::studio_draw_player);
        if (!globals_match || !matrices_match || !api_match)
            return false;

        current_entity_global = reinterpret_cast<cl_entity_t**>(memory::global_address(globals_match, 7));
        studiohdr_global = reinterpret_cast<studiohdr_t**>(memory::global_address(globals_match, 0xD));
        bone_matrices = reinterpret_cast<bone_matrix_t*>(memory::global_address(matrices_match, 1));
        if (!current_entity_global || !studiohdr_global || !bone_matrices)
            return false;

        studio_api_global = memory::global_address(api_match, 2);
        if (!studio_api_global)
            return false;

        studio_interface = *reinterpret_cast<uint8_t**>(studio_api_global);
        if (!studio_interface)
            return false;

        initialized = true;

        return true;
    }

    auto set_draw_player(int (__cdecl* detour)(int flags, entity_state_t* pplayer)) -> bool
    {
        if (!studio_interface)
            return false;

        auto function_slot = reinterpret_cast<int(__cdecl**)(int, entity_state_t*)>(studio_interface + 8);
        if (!function_slot || !*function_slot)
            return false;

        draw_player_original = *function_slot;
        *function_slot = detour;

        return true;
    }

    auto restore_draw_player() -> void
    {
        if (!studio_interface || !draw_player_original)
            return;

        *reinterpret_cast<int(__cdecl**)(int, entity_state_t*)>(studio_interface + 8) = draw_player_original;
        draw_player_original = nullptr;
    }

    auto begin_frame() -> void
    {
        frame_counter++;

        for (int index = 0; index < 33; index++)
            frame_bone_counts[index] = 0;
    }

    auto capture() -> void
    {
        cl_entity_t* entity = current_entity_global ? *current_entity_global : nullptr;
        studiohdr_t* studio_header = studiohdr_global ? *studiohdr_global : nullptr;

        if (!entity || !studio_header || !entity->player || entity->index <= 0 || entity->index > 32)
            return;

        const int count = studio_header->numbones < 128 ? studio_header->numbones : 128;
        if (captured[entity->index] && count <= frame_bone_counts[entity->index])
            return;

        captured[entity->index] = true;
        last_visible_frame[entity->index] = frame_counter;
        frame_bone_counts[entity->index] = count;

        for (int index = 0; index < count; index++)
            bones[entity->index][index] = bone_matrices[index];

        headers[entity->index] = studio_header;
        bone_counts[entity->index] = count;
    }

    auto current_entity() -> cl_entity_t*
    {
        return current_entity_global ? *current_entity_global : nullptr;
    }

    auto has_model(int entity_index) -> bool
    {
        if (entity_index <= 0 || entity_index > 32)
            return false;

        return captured[entity_index] && frame_counter - last_visible_frame[entity_index] <= 1;
    }

    auto header(int entity_index) -> studiohdr_t*
    {
        if (entity_index <= 0 || entity_index > 32)
            return nullptr;

        return headers[entity_index];
    }

    auto bone_list(studiohdr_t* header) -> mstudiobone_t*
    {
        if (!header)
            return nullptr;

        return reinterpret_cast<mstudiobone_t*>(reinterpret_cast<uint8_t*>(header) + header->boneindex);
    }

    auto hitbox_list(studiohdr_t* header) -> mstudiobbox_t*
    {
        if (!header || header->numhitboxes <= 0)
            return nullptr;

        return reinterpret_cast<mstudiobbox_t*>(reinterpret_cast<uint8_t*>(header) + header->hitboxindex);
    }

    auto bone_count(int entity_index) -> int
    {
        if (entity_index <= 0 || entity_index > 32)
            return 0;

        return bone_counts[entity_index];
    }

    auto bone(int entity_index, int bone_index) -> const bone_matrix_t*
    {
        if (entity_index <= 0 || entity_index > 32 || bone_index < 0 || bone_index >= bone_counts[entity_index])
            return nullptr;

        return &bones[entity_index][bone_index];
    }

    auto hitbox_center(int entity_index, const mstudiobbox_t& hitbox) -> vec3_t
    {
        const bone_matrix_t* matrix = bone(entity_index, hitbox.bone);
        if (!matrix)
            return vec3_t();

        return matrix->transform((hitbox.bbmin + hitbox.bbmax) * 0.5f);
    }

    auto hitbox_bounds(int entity_index, vec3_t& world_min, vec3_t& world_max) -> bool
    {
        studiohdr_t* studio_header = header(entity_index);
        const mstudiobbox_t* hitboxes = hitbox_list(studio_header);
        if (!hitboxes)
            return false;

        bool found = false;

        for (int index = 0; index < studio_header->numhitboxes; index++)
        {
            const mstudiobbox_t& hitbox = hitboxes[index];
            const bone_matrix_t* matrix = bone(entity_index, hitbox.bone);
            if (!matrix)
                continue;

            for (int corner = 0; corner < 8; corner++)
            {
                const vec3_t local((corner & 1) ? hitbox.bbmax.x : hitbox.bbmin.x, (corner & 2) ? hitbox.bbmax.y : hitbox.bbmin.y, (corner & 4) ? hitbox.bbmax.z : hitbox.bbmin.z);
                const vec3_t world = matrix->transform(local);

                if (!found)
                {
                    world_min = world;
                    world_max = world;
                    found = true;
                    continue;
                }

                world_min.x = world.x < world_min.x ? world.x : world_min.x;
                world_min.y = world.y < world_min.y ? world.y : world_min.y;
                world_min.z = world.z < world_min.z ? world.z : world_min.z;

                world_max.x = world.x > world_max.x ? world.x : world_max.x;
                world_max.y = world.y > world_max.y ? world.y : world_max.y;
                world_max.z = world.z > world_max.z ? world.z : world_max.z;
            }
        }

        return found;
    }

    auto hitbox_hit(int entity_index, const vec3_t& start, const vec3_t& end, vec3_t& point) -> bool
    {
        studiohdr_t* studio_header = header(entity_index);
        const mstudiobbox_t* hitboxes = hitbox_list(studio_header);
        if (!hitboxes)
            return false;

        bool found = false;
        float nearest = 1.0f;

        for (int index = 0; index < studio_header->numhitboxes; index++)
        {
            const mstudiobbox_t& hitbox = hitboxes[index];
            const bone_matrix_t* matrix = bone(entity_index, hitbox.bone);
            if (!matrix)
                continue;

            float fraction;
            if (!segment_vs_hitbox(matrix, hitbox, start, end, fraction) || fraction >= nearest)
                continue;

            nearest = fraction;
            found = true;
        }

        if (!found)
            return false;

        point = start + (end - start) * nearest;
        return true;
    }

    auto hitbox_trace(int entity_index, const vec3_t& start, const vec3_t& end, float& fraction, int& hitgroup) -> bool
    {
        studiohdr_t* studio_header = header(entity_index);
        const mstudiobbox_t* hitboxes = hitbox_list(studio_header);
        if (!hitboxes)
            return false;

        bool found = false;
        float nearest = 1.0f;
        int nearest_group = 0;

        for (int index = 0; index < studio_header->numhitboxes; index++)
        {
            const mstudiobbox_t& hitbox = hitboxes[index];
            const bone_matrix_t* matrix = bone(entity_index, hitbox.bone);
            if (!matrix)
                continue;

            float local_fraction;
            if (!segment_vs_hitbox(matrix, hitbox, start, end, local_fraction) || local_fraction >= nearest)
                continue;

            nearest = local_fraction;
            nearest_group = hitbox.group;
            found = true;
        }

        if (!found)
            return false;

        fraction = nearest;
        hitgroup = nearest_group;
        return true;
    }
}