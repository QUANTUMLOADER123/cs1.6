#include <cstdint>
#include <imgui/imgui_internal.h>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/math/angle.hpp>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/structs/cl_entity.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/structs/studio.hpp>
#include <cstrike1_5/structs/cvar.hpp>
#include <cstrike1_5/interfaces/engine/engine.hpp>
#include <cstrike1_5/interfaces/globals/globals.hpp>
#include <cstrike1_5/interfaces/cvars/engine_cvars.hpp>
#include <cstrike1_5/interfaces/tracers/tracers.hpp>
#include <cstrike1_5/interfaces/studio/studio.hpp>
#include <features/vars.hpp>
#include <render/render.hpp>
#include <features/ragebot/ragebot.hpp>

#include <array>
#include <vector>
#include <algorithm>
#include <utility>

#include <features/world/bullet_tracers/bullet_tracers.hpp>

#include <imgui/imgui.h>

namespace bullet_tracers
{
    static auto own_head_center(vec3_t& center) -> bool
    {
        const int local_index = engine::local_player_index();

        studiohdr_t* studio_header = studio::header(local_index);
        const mstudiobbox_t* hitboxes = studio::hitbox_list(studio_header);
        if (!hitboxes)
            return false;

        for (int index = 0; index < studio_header->numhitboxes; index++)
        {
            const mstudiobbox_t& hitbox = hitboxes[index];
            if (hitbox.group != hitgroup_head)
                continue;

            const bone_matrix_t* matrix = studio::bone(local_index, hitbox.bone);
            if (!matrix)
                return false;

            center = matrix->transform((hitbox.bbmin + hitbox.bbmax) * 0.5f);
            return true;
        }

        return false;
    }

    static auto tracer_start(vec3_t& start) -> bool
    {
        if (own_head_center(start))
            return true;

        const int local_index = engine::local_player_index();
        cl_entity_t* entity = engine::entity(local_index);
        client_state_t* state = engine::client_state();
        if (!entity || !state)
            return false;

        start = entity->origin + state->viewheight;
        return true;
    }

    auto angle_basis(const vec3_t& angles, vec3_t& forward, vec3_t& right, vec3_t& up) -> void
    {
        math::angle_vectors(angles, forward, right, up);
    }

    static auto apply_tracer_color(const float* color) -> void
    {
        cvar_t* red = engine_cvars::find("tracerred");
        cvar_t* green = engine_cvars::find("tracergreen");
        cvar_t* blue = engine_cvars::find("tracerblue");
        cvar_t* alpha = engine_cvars::find("traceralpha");

        if (red)
            red->value = color[0];

        if (green)
            green->value = color[1];

        if (blue)
            blue->value = color[2];

        if (alpha)
            alpha->value = color[3];
    }

    static auto apply_tracer_speed(const float speed, const float length) -> void
    {
        cvar_t* tracer_speed = engine_cvars::find("tracerspeed");
        cvar_t* tracer_length = engine_cvars::find("tracerlength");

        if (tracer_speed)
            tracer_speed->value = speed;

        if (tracer_length)
            tracer_length->value = length;
    }

    static auto __cdecl on_fire_bullets(uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5, uintptr_t p6, uintptr_t p7, uintptr_t p8, uintptr_t p9, uintptr_t p10, uintptr_t p11, uintptr_t p12, uintptr_t p13) -> void
    {
        auto original = fire_bullets_hook.get_original<decltype(&on_fire_bullets)>();

        own_fire_active = p1 == static_cast<uintptr_t>(engine::local_player_index());

        if (own_fire_active && ragebot::shot_pending && globals::tick_count() - ragebot::shot_tick < 3)
        {
            vec3_t forward;
            vec3_t right;
            vec3_t up;

            angle_basis(ragebot::shot_angles, forward, right, up);

            *reinterpret_cast<vec3_t*>(p2) = forward;
            *reinterpret_cast<vec3_t*>(p7) = forward;
            *reinterpret_cast<vec3_t*>(p3) = right;
            *reinterpret_cast<vec3_t*>(p4) = up;
        }

        original(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);
        own_fire_active = false;
    }

    static auto __cdecl on_bullet_impact(uintptr_t trace_data, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5, uintptr_t p6, uintptr_t p7, uintptr_t p8) -> void
    {
        auto original = bullet_impact_hook.get_original<decltype(&on_bullet_impact)>();

        if (own_fire_active)
        {
            const vec3_t impact = *reinterpret_cast<vec3_t*>(trace_data + 0x14);

            if (impact.is_valid())
            {
                vec3_t start;
                const bool has_start = tracer_start(start);

                if (vars::bullet_impacts.enabled && impacts.size() < 128)
                    impacts.push_back({ impact, globals::time() });

                if (has_start && vars::bullet_tracers.enabled)
                {
                    const vec3_t span = impact - start;

                    if (span.length() > 1.0f)
                    {
                        apply_tracer_color(vars::bullet_tracers.color);
                        apply_tracer_speed(vars::bullet_tracers.speed, vars::bullet_tracers.length);
                        engine_tracers::add(start, span.normalized(), vars::bullet_tracers.life, vars::bullet_tracers.color_index, vars::bullet_tracers.speed, 0, 0);
                    }
                }
            }
        }

        original(trace_data, p2, p3, p4, p5, p6, p7, p8);
    }

    auto initialize() -> bool
    {
        static bool initialized = false;
        if (initialized)
            return true;

        void* fire_address = memory::find_pattern(modules::client, patterns::fire_bullets);
        void* impact_address = memory::find_pattern(modules::client, patterns::bullet_impact);
        if (!fire_address || !impact_address)
            return false;

        fire_bullets_hook.create(fire_address, reinterpret_cast<void*>(on_fire_bullets));
        bullet_impact_hook.create(impact_address, reinterpret_cast<void*>(on_bullet_impact));
        initialized = true;

        return true;
    }

    auto shutdown() -> void
    {
        impacts.clear();
    }

    auto render() -> void
    {
        const double now = globals::time();
        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

        if (vars::bullet_impacts.enabled)
        {
            static constexpr std::array<std::array<uint8_t, 3>, 12> cube_triangles
            { {
                { 0, 1, 3 }, { 0, 3, 2 },
                { 4, 5, 7 }, { 4, 7, 6 },
                { 0, 2, 6 }, { 0, 6, 4 },
                { 1, 3, 7 }, { 1, 7, 5 },
                { 2, 3, 7 }, { 2, 7, 6 },
                { 0, 1, 5 }, { 0, 5, 4 }
            } };

            static constexpr std::array<std::pair<uint8_t, uint8_t>, 12> cube_edges
            { {
                { 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },
                { 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 },
                { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
            } };

            for (auto iter = impacts.begin(); iter != impacts.end();)
            {
                const float age = static_cast<float>(now - iter->created);
                if (age > 3.0f)
                {
                    iter = impacts.erase(iter);
                    continue;
                }

                const float half = 2.0f;
                vec2_t screen[8] = {};
                bool valid = true;

                for (int index = 0; index < 8 && valid; index++)
                {
                    const vec3_t corner = vec3_t(
                        iter->point.x + (index & 1 ? half : -half),
                        iter->point.y + (index & 2 ? half : -half),
                        iter->point.z + (index & 4 ? half : -half));

                    valid = render::wts(corner, screen[index]);
                }

                if (!valid)
                {
                    iter++;
                    continue;
                }

                const float alpha = vars::bullet_impacts.color[3] * (1.0f - age / 3.0f);
                const uint8_t bright = static_cast<uint8_t>(std::clamp(alpha * 255.0f, 0.0f, 255.0f));
                const uint8_t fill_alpha = static_cast<uint8_t>(std::clamp(bright * 0.35f, 0.0f, 255.0f));

                const ImU32 fill_color = IM_COL32(
                    static_cast<int>(vars::bullet_impacts.color[0] * 255.0f),
                    static_cast<int>(vars::bullet_impacts.color[1] * 255.0f),
                    static_cast<int>(vars::bullet_impacts.color[2] * 255.0f),
                    fill_alpha);

                const ImU32 edge_color = IM_COL32(
                    static_cast<int>(vars::bullet_impacts.color[0] * 255.0f),
                    static_cast<int>(vars::bullet_impacts.color[1] * 255.0f),
                    static_cast<int>(vars::bullet_impacts.color[2] * 255.0f),
                    bright);

                for (const std::array<uint8_t, 3>& triangle : cube_triangles)
                    draw_list->AddTriangleFilled(ImVec2(screen[triangle[0]].x, screen[triangle[0]].y), ImVec2(screen[triangle[1]].x, screen[triangle[1]].y), ImVec2(screen[triangle[2]].x, screen[triangle[2]].y), fill_color);

                for (const std::pair<uint8_t, uint8_t>& edge : cube_edges)
                    draw_list->AddLine(ImVec2(screen[edge.first].x, screen[edge.first].y), ImVec2(screen[edge.second].x, screen[edge.second].y), edge_color, 1.0f);

                iter++;
            }
        }
    }
}
