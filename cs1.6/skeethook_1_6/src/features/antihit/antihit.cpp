#include <cstdint>
#include <cmath>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/math/angle.hpp>
#include <cstrike1_5/enums.hpp>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/structs/playermove.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <core/hooks.hpp>
#include <features/vars.hpp>
#include <features/ragebot/ragebot.hpp>
#include <features/misc/misc.hpp>

#include <features/antihit/antihit.hpp>

namespace antihit
{
    auto jitter_offset(int step, int sides, float amount) -> float
    {
        if (sides == 5)
        {
            const float half = amount * 0.5f;
            switch (step)
            {
            case 1: return -half;
            case 2: return 0.0f;
            case 3: return half;
            case 4: return amount;
            default: return -amount;
            }
        }

        switch (step)
        {
        case 1: return 0.0f;
        case 2: return amount;
        default: return -amount;
        }
    }

    auto random_jitter(float amount) -> float
    {
        jitter_random = jitter_random * 1664525u + 1013904223u;
        const float random = static_cast<float>((jitter_random >> 8) & 0xFFFFFF) * (1.0f / 16777215.0f);
        return (random * 2.0f - 1.0f) * amount;
    }

    auto calculate_fake_angles() -> vec3_t
    {
        float fake_yaw = std::fmodf(real_angles.y + vars::antihit.yaw, 360.0f);

        if (vars::antihit.type == 1)
        {
            jitter_side = !jitter_side;
            fake_yaw = std::fmodf(real_angles.y + vars::antihit.yaw + (jitter_side ? vars::antihit.jitter_amount : -vars::antihit.jitter_amount), 360.0f);
        }
        else if (vars::antihit.type == 2)
        {
            spin_angle = std::fmodf(spin_angle + vars::antihit.spin_speed, 360.0f);
            fake_yaw = spin_angle;
        }
        else if (vars::antihit.type == 3)
        {
            jitter_step = (jitter_step + 1) % 3;
            fake_yaw = std::fmodf(real_angles.y + vars::antihit.yaw + jitter_offset(jitter_step, 3, vars::antihit.jitter_amount), 360.0f);
        }
        else if (vars::antihit.type == 4)
        {
            jitter_step = (jitter_step + 1) % 5;
            fake_yaw = std::fmodf(real_angles.y + vars::antihit.yaw + jitter_offset(jitter_step, 5, vars::antihit.jitter_amount), 360.0f);
        }
        else if (vars::antihit.type == 5)
        {
            swing_phase = std::fmodf(swing_phase + 12.0f, 360.0f);
            fake_yaw = std::fmodf(real_angles.y + vars::antihit.yaw + vars::antihit.jitter_amount * std::sinf(math::to_radians(swing_phase)), 360.0f);
        }
        else if (vars::antihit.type == 6)
        {
            fake_yaw = std::fmodf(real_angles.y + vars::antihit.yaw + random_jitter(vars::antihit.jitter_amount), 360.0f);
        }

        return vec3_t(vars::antihit.pitch, fake_yaw, real_angles.z);
    }

    auto send_packet(bool value) -> void
    {
        if (!send_packet_operand)
            return;

        if (value)
            *send_packet_operand = send_packet_backup;
        else
            *send_packet_operand = reinterpret_cast<uintptr_t>(&fake_lag_block);
    }

    auto initialize() -> bool
    {
        static bool initialized = false;
        if (initialized)
            return true;

        uint8_t* hook_address = memory::find_pattern(modules::renderer, patterns::antihit_camera_hook);
        uint8_t* camera_ref = memory::find_pattern(modules::renderer, patterns::antihit_camera_angles);
        uint8_t* view_ref = memory::find_pattern(modules::renderer, patterns::antihit_view_angles);
        if (!hook_address || !camera_ref || !view_ref)
            return false;

        view_angles_address = *reinterpret_cast<vec3_t**>(view_ref + 2);
        camera_angles_address = *reinterpret_cast<vec3_t**>(camera_ref + 2);

        camera_hook.create(hook_address, reinterpret_cast<void*>(on_camera_calc));

        uint8_t* send_packet_ref = memory::find_pattern(modules::renderer, patterns::fake_lag_global_time);
        if (!send_packet_ref)
            return false;

        send_packet_operand = reinterpret_cast<uintptr_t*>(send_packet_ref + 0x1b + 2);
        send_packet_backup = *send_packet_operand;

        VirtualProtect(send_packet_operand, sizeof(uintptr_t), PAGE_EXECUTE_READWRITE, &send_packet_protection);

        initialized = true;
        return true;
    }

    auto shutdown() -> void
    {
        if (!send_packet_operand)
            return;

        *send_packet_operand = send_packet_backup;
        VirtualProtect(send_packet_operand, sizeof(uintptr_t), send_packet_protection, &send_packet_protection);
    }

    auto on_create_move(float frametime, usercmd_t* cmd) -> void
    {
        real_angles = cmd->viewangles;

        if (!vars::antihit.enabled)
        {
            fake_angles = real_angles;
            return;
        }
        if (cmd->buttons & in_use)
        {
            fake_angles = real_angles;
            return;
        }

        fake_angles = calculate_fake_angles();
        cmd->viewangles = fake_angles;

        misc::movement_fix(cmd, fake_angles.y - real_angles.y);
    }

    auto fake_lag(float frametime, usercmd_t* cmd) -> void
    {
        if (!vars::antihit.fake_lag)
        {
            fake_lag_counter = 0;
            fake_lag_good = 0;
            send_packet(true);
            return;
        }

        const playermove_t* player_move = hooks::player_move();
        if (!player_move)
            return;

        const float speed = player_move->velocity.length_2d();
        const bool in_air = player_move->onground == -1;
        const bool is_attacking = (cmd->buttons & in_attack) && ragebot::can_shoot();

        bool should_fake_lag = !is_attacking;
        if (vars::antihit.fake_lag_while_shooting && is_attacking)
            should_fake_lag = true;

        if (vars::antihit.fake_lag_move == 1 && speed > 0.0f)
            should_fake_lag = false;
        else if (vars::antihit.fake_lag_move == 2 && speed <= 0.0f)
            should_fake_lag = false;
        else if (vars::antihit.fake_lag_move == 3 && !in_air)
            should_fake_lag = false;

        if (!should_fake_lag)
        {
            fake_lag_counter = 0;
            fake_lag_good = 0;
            send_packet(true);
            return;
        }

        const int limit = vars::antihit.fake_lag_ticks;
        const float send_window = static_cast<float>(limit) * vars::antihit.fake_lag_variance / 100.0f;

        if (vars::antihit.fake_lag_type == 1)
        {
            if (fake_lag_counter < limit)
            {
                send_packet(false);
                ++fake_lag_counter;
            }
            else if (static_cast<float>(++fake_lag_good) > send_window)
            {
                fake_lag_counter = 0;
                fake_lag_good = 0;
            }
        }
        else if (vars::antihit.fake_lag_type == 2)
        {
            ++fake_lag_counter;
            if (fake_lag_counter > 0)
            {
                send_packet(false);
            }
            if (fake_lag_counter > limit)
                fake_lag_counter = -1;
        }
        else if (vars::antihit.fake_lag_type == 3)
        {
            if (fake_lag_jitter)
            {
                send_packet(false);
            }
            fake_lag_jitter = !fake_lag_jitter;
        }
        else if (vars::antihit.fake_lag_type == 4)
        {
            const float travel = speed * frametime;
            if (travel < 64.0f && speed > 0.05f)
            {
                int need_choke = static_cast<int>(64.0f / travel);
                if (need_choke > limit)
                    need_choke = limit;

                if (fake_lag_counter < need_choke)
                {
                    send_packet(false);
                    ++fake_lag_counter;
                }
                else
                {
                    fake_lag_counter = 0;
                    send_packet(true);
                }
            }
            else
            {
                fake_lag_counter = 0;
                fake_lag_good = 0;
                send_packet(true);
            }
        }
    }

    auto __cdecl on_camera_calc() -> int
    {
        auto original = camera_hook.get_original<decltype(&on_camera_calc)>();

        if (!vars::antihit.enabled)
            return original();

        *view_angles_address = fake_angles;

        int result = original();

        *camera_angles_address = real_angles;

        return result;
    }
}