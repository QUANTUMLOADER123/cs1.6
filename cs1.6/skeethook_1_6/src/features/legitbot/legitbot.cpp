#include <cmath>
#include <algorithm>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/math/angle.hpp>
#include <cstrike1_5/enums.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/structs/studio.hpp>
#include <cstrike1_5/interfaces/engine/engine.hpp>
#include <cstrike1_5/interfaces/studio/studio.hpp>
#include <cstrike1_5/interfaces/trace/trace.hpp>
#include <cstrike1_5/classes/c_base_player.hpp>
#include <features/vars.hpp>
#include <render/render.hpp>
#include <features/misc/misc.hpp>

#include <features/legitbot/legitbot.hpp>

namespace legitbot
{
    auto eye_position() -> vec3_t
    {
        return c_base_player(engine::entity(engine::local_player_index())).get_eye_position();
    }

    auto acquire_target(const vec3_t& view_angles) -> aim_result_t
    {
        aim_result_t best;
        float best_delta = vars::legitbot.fov;

        const int local_index = engine::local_player_index();
        const int max_clients = engine::max_clients();

        c_base_player local(engine::entity(local_index));
        const vec3_t eye = local.get_eye_position();
        if (!eye.is_valid())
            return best;

        for (int index = 1; index <= max_clients; index++)
        {
            if (index == local_index)
                continue;

            c_base_player player(engine::entity(index));
            if (!player.base || !player.is_player() || !player.is_alive() || !player.is_enemy())
                continue;

            studiohdr_t* studio_header = studio::header(index);
            if (!studio_header || !studio::has_model(index))
                continue;

            const mstudiobbox_t* hitboxes = studio::hitbox_list(studio_header);
            if (!hitboxes)
                continue;

            for (int hitbox = 0; hitbox < studio_header->numhitboxes; hitbox++)
            {
                const mstudiobbox_t& hitbox_data = hitboxes[hitbox];
                if (hitbox_data.group < 1 || hitbox_data.group > 8 || !vars::legitbot.hitbox[hitbox_data.group - 1])
                    continue;

                const vec3_t point = studio::hitbox_center(index, hitbox_data);
                if (!point.is_valid())
                    continue;

                if (!trace::visible(eye, point))
                    continue;

                const vec3_t angles = math::vector_angles(eye, point);
                const float delta = math::angle_delta(view_angles, angles);
                if (delta >= best_delta)
                    continue;

                best_delta = delta;
                best.found = true;
                best.delta = delta;
                best.angles = angles;
            }
        }

        return best;
    }

    auto apply_smooth(const vec3_t& current, const vec3_t& target, vec3_t& out) -> void
    {
        const float factor = 1.0f / vars::legitbot.smooth;
        vec3_t delta = target - current;
        delta.x = math::normalize_angle(delta.x);
        delta.y = math::normalize_angle(delta.y);

        out = current + delta * factor;
        out.clamp();
    }

    auto on_create_move(float frametime, usercmd_t* cmd) -> void
    {
        if (!vars::legitbot.enabled)
            return;

        if (vars::legitbot.activation == 1 && !(cmd->buttons & in_attack))
            return;

        client_state_t* state = engine::client_state();
        if (!state)
            return;

        const vec3_t current = cmd->viewangles;
        const aim_result_t result = acquire_target(current);
        if (!result.found)
            return;

        vec3_t aiming = result.angles;
        if (vars::legitbot.rcs)
        {
            const vec3_t punch = c_base_player().get_aim_punch();
            aiming.x = std::clamp(aiming.x - punch.x, -89.0f, 89.0f);
            aiming.y = math::normalize_angle(aiming.y - punch.y);
        }

        vec3_t next;
        apply_smooth(current, aiming, next);

        state->viewangles = next;
        cmd->viewangles = next;
        misc::movement_fix(cmd, math::normalize_angle(next.y - current.y));
    }

    auto render() -> void
    {
        if (!vars::legitbot.enabled || !vars::legitbot.draw_fov)
            return;

        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        const float center_x = render::width * 0.5f;
        const float center_y = render::height * 0.5f;
        const float radius = std::tanf(math::to_radians(vars::legitbot.fov) * 0.5f) * (render::height * 0.5f);
        const ImU32 ring_color = IM_COL32(255, 255, 255, 140);

        draw_list->AddCircle(ImVec2(center_x, center_y), radius, ring_color, 64, 2.0f);
    }
}