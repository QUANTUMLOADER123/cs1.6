#pragma once

#include <algorithm>
#include <cmath>

#include <cstrike1_5/enums.hpp>
#include <cstrike1_5/classes/c_weapon.hpp>
#include <cstrike1_5/interfaces/weapons/weapons.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/structs/playermove.hpp>

namespace spread
{
    struct state_t
    {
        int weapon_id = 0;
        int shots = 0;
        int flags = 0;
        float speed = 0.0f;
        float time = 0.0f;
        float value = 0.0f;
        float last_shot = 0.0f;
        float broken_time = 0.0f;
        float idle_time = 0.0f;
        bool firing = false;
    };

    inline state_t current;

    inline auto deploy_value(int weapon_id) -> float
    {
        switch (weapon_id)
        {
        case weapon_deagle: return 0.9f;
        case weapon_fiveseven: return 0.92f;
        case weapon_elite: return 0.88f;
        case weapon_glock18: return 0.9f;
        case weapon_usp: return 0.92f;
        case weapon_p228: return 0.9f;
        case weapon_aug: return 0.3f;
        case weapon_mac10: return 0.15f;
        case weapon_p90: return 0.15f;
        case weapon_ak47: return 0.2f;
        case weapon_sg552: return 0.2f;
        case weapon_m4a1: return 0.2f;
        case weapon_m249: return 0.2f;
        case weapon_tmp: return 0.2f;
        case weapon_g3sg1: return 0.2f;
        case weapon_sg550: return 0.2f;
        default: return 0.0f;
        }
    }

    inline auto is_secondary_weapon(int weapon_id) -> bool
    {
        switch (weapon_id)
        {
        case weapon_p228:
        case weapon_usp:
        case weapon_deagle:
        case weapon_elite:
        case weapon_fiveseven:
        case weapon_glock18:
            return true;
        default:
            return false;
        }
    }

    inline auto decay(float scale, float interval, float maximum, float minimum) -> void
    {
        if (current.last_shot == 0.0f)
            return;

        current.value = std::clamp(current.value - scale * (interval - (current.time - current.last_shot)), minimum, maximum);
    }

    inline auto clamp_cubed(float divisor, float base, float maximum) -> void
    {
        const float shots = static_cast<float>(current.shots);

        current.value = std::min(shots * shots * shots / divisor + base, maximum);
    }

    inline auto clamp_squared(float divisor, float base, float maximum) -> void
    {
        const float shots = static_cast<float>(current.shots);

        current.value = std::min(shots * shots / divisor + base, maximum);
    }

    inline auto reset(int weapon_id, float time) -> void
    {
        current.weapon_id = weapon_id;
        current.shots = 0;
        current.last_shot = 0.0f;
        current.idle_time = time;
        current.firing = true;
        current.value = deploy_value(weapon_id);
    }

    inline auto fired(float time, bool burst) -> void
    {
        switch (current.weapon_id)
        {
        case weapon_deagle:
            decay(0.35f, 0.4f, 0.9f, 0.55f);
            current.shots += 1;
            current.last_shot = time;
            return;
        case weapon_elite:
            decay(0.275f, 0.325f, 0.88f, 0.55f);
            current.shots += 1;
            current.last_shot = time;
            return;
        case weapon_fiveseven:
            decay(0.25f, 0.275f, 0.92f, 0.725f);
            current.shots += 1;
            current.last_shot = time;
            return;
        case weapon_p228:
            decay(0.3f, 0.325f, 0.9f, 0.6f);
            current.shots += 1;
            current.last_shot = time;
            return;
        case weapon_usp:
            decay(0.275f, 0.3f, 0.92f, 0.6f);
            current.shots += 1;
            current.last_shot = time;
            return;
        case weapon_glock18:
            if (!burst)
                current.shots += 1;

            decay(0.275f, 0.325f, 0.9f, 0.6f);
            current.last_shot = time;
            return;
        case weapon_g3sg1:
            if (current.broken_time != 0.0f)
                current.value = std::min(0.55f + 0.3f * (time - current.broken_time), 0.98f);

            current.shots += 1;
            current.broken_time = time;
            current.firing = true;
            return;
        case weapon_sg550:
            if (current.broken_time != 0.0f)
                current.value = std::min(0.65f + 0.35f * (time - current.broken_time), 0.98f);

            current.shots += 1;
            current.broken_time = time;
            current.firing = true;
            return;
        case weapon_ak47:
            current.shots += 1;
            current.firing = true;
            clamp_cubed(200.0f, 0.35f, 1.25f);
            current.last_shot = time;
            return;
        case weapon_galil:
            current.shots += 1;
            current.firing = true;
            clamp_cubed(200.0f, 0.35f, 1.25f);
            current.last_shot = time;
            return;
        case weapon_famas:
            current.shots += 1;
            current.firing = true;
            clamp_cubed(215.0f, 0.3f, 1.0f);
            current.last_shot = time;
            return;
        case weapon_aug:
            current.shots += 1;
            current.firing = true;
            clamp_cubed(215.0f, 0.3f, 1.0f);
            current.last_shot = time;
            return;
        case weapon_sg552:
            current.shots += 1;
            current.firing = true;
            clamp_cubed(220.0f, 0.3f, 1.0f);
            current.last_shot = time;
            return;
        case weapon_m4a1:
            current.shots += 1;
            current.firing = true;
            clamp_cubed(220.0f, 0.3f, 1.0f);
            current.last_shot = time;
            return;
        case weapon_m249:
            current.shots += 1;
            current.firing = true;
            clamp_cubed(175.0f, 0.4f, 0.9f);
            current.last_shot = time;
            return;
        case weapon_mp5navy:
            current.shots += 1;
            current.firing = true;
            clamp_cubed(220.1f, 0.45f, 0.75f);
            current.last_shot = time;
            return;
        case weapon_mac10:
            current.shots += 1;
            current.firing = true;
            clamp_cubed(200.0f, 0.6f, 1.65f);
            current.last_shot = time;
            return;
        case weapon_tmp:
            current.shots += 1;
            current.firing = true;
            clamp_cubed(200.0f, 0.55f, 1.4f);
            current.last_shot = time;
            return;
        case weapon_p90:
            current.shots += 1;
            current.firing = true;
            clamp_squared(175.0f, 0.45f, 1.0f);
            current.last_shot = time;
            return;
        case weapon_ump45:
            current.shots += 1;
            current.firing = true;
            clamp_squared(210.0f, 0.5f, 1.0f);
            current.last_shot = time;
            return;
        case weapon_awp:
        case weapon_scout:
            current.shots += 1;
            current.last_shot = time;
            current.firing = true;
            return;
        }
    }

    inline auto idle(float time) -> void
    {
        if (current.firing)
        {
            current.firing = false;
            current.shots = std::min(current.shots, 15);
            current.idle_time = time + 0.4f;
        }

        if (is_secondary_weapon(current.weapon_id))
        {
            current.shots = 0;
            return;
        }

        if (current.shots <= 0 || current.idle_time > time)
            return;

        current.idle_time += 0.0225f;
        current.shots -= 1;
    }

    inline auto can_attack(const weapon_data_t& weapon_data) -> bool
    {
        return weapon_data.m_flNextPrimaryAttack <= 0.0f;
    }

    inline auto poll(const local_state_t& state, const usercmd_t& cmd, bool runfuncs, float time, const playermove_t* move) -> void
    {
        if (!runfuncs)
            return;

        current.flags = state.client.flags;
        current.speed = move ? move->velocity.length_2d() : 0.0f;
        current.time = time;

        const int held_id = current.weapon_id;

        if (held_id > 0 && held_id < 64)
        {
            const weapon_data_t& weapon_data = state.weapondata[held_id];

            if ((cmd.buttons & in_attack) && can_attack(weapon_data))
                fired(time, weapons::local().is_in_burst());
            else if (!(cmd.buttons & (in_attack | in_attack2)))
                idle(time);
        }

        const int weapon_id = state.client.m_iId;

        if (weapon_id != held_id)
            reset(weapon_id, time);

        if (weapon_id > 0 && weapon_id < 64 && state.weapondata[weapon_id].m_fInReload)
        {
            reset(weapon_id, time);
            current.firing = false;
        }
    }

    inline auto pistol_radius(int weapon_id, bool silenced, bool burst, bool on_ground, bool ducking, float speed, float value) -> float
    {
        const float inverse = 1.0f - value;

        if (!on_ground)
        {
            switch (weapon_id)
            {
            case weapon_elite: return 1.3f * inverse;
            case weapon_glock18: return burst ? 1.2f * inverse : inverse;
            case weapon_usp: return silenced ? 1.3f * inverse : 1.2f * inverse;
            default: return 1.5f * inverse;
            }
        }

        if (speed != 0.0f)
        {
            switch (weapon_id)
            {
            case weapon_deagle: return 0.25f * inverse;
            case weapon_elite: return 0.175f * inverse;
            case weapon_glock18: return burst ? 0.185f * inverse : 0.165f * inverse;
            case weapon_usp: return silenced ? 0.25f * inverse : 0.225f * inverse;
            default: return 0.255f * inverse;
            }
        }

        if (ducking)
        {
            switch (weapon_id)
            {
            case weapon_deagle: return 0.115f * inverse;
            case weapon_elite: return 0.08f * inverse;
            case weapon_glock18: return burst ? 0.095f * inverse : 0.075f * inverse;
            case weapon_usp: return silenced ? 0.125f * inverse : 0.08f * inverse;
            default: return 0.075f * inverse;
            }
        }

        switch (weapon_id)
        {
        case weapon_deagle: return 0.13f * inverse;
        case weapon_elite: return 0.1f * inverse;
        case weapon_glock18: return burst ? 0.3f * inverse : 0.1f * inverse;
        case weapon_usp: return silenced ? 0.15f * inverse : 0.1f * inverse;
        default: return 0.15f * inverse;
        }
    }

    inline auto automatic_radius(int weapon_id, bool silenced, bool on_ground, float speed, float value) -> float
    {
        if (!on_ground)
        {
            switch (weapon_id)
            {
            case weapon_mp5navy: return value * 0.2f;
            case weapon_mac10: return value * 0.375f;
            case weapon_ump45: return value * 0.24f;
            case weapon_tmp: return value * 0.25f;
            case weapon_p90: return value * 0.3f;
            case weapon_famas: return value * 0.4f + 0.04f;
            case weapon_galil: return value * 0.4f + 0.03f;
            case weapon_m249: return value * 0.5f + 0.045f;
            case weapon_ak47: return value * 0.4f + 0.04f;
            default: return value * 0.4f + 0.035f;
            }
        }

        switch (weapon_id)
        {
        case weapon_mp5navy: return value * 0.04f;
        case weapon_mac10: return value * 0.03f;
        case weapon_ump45: return value * 0.04f;
        case weapon_tmp: return value * 0.03f;
        default: break;
        }

        if (weapon_id == weapon_p90)
            return speed > 170.0f ? value * 0.115f : value * 0.045f;

        if (speed > 140.0f)
        {
            switch (weapon_id)
            {
            case weapon_famas: return value * 0.07f + 0.03f;
            case weapon_m249: return value * 0.095f + 0.045f;
            case weapon_galil:
            case weapon_ak47: return value * 0.07f + 0.04f;
            default: return value * 0.07f + 0.035f;
            }
        }

        switch (weapon_id)
        {
        case weapon_galil: return value * 0.0375f;
        case weapon_m249: return value * 0.03f;
        case weapon_m4a1: return silenced ? value * 0.025f : value * 0.02f;
        case weapon_ak47: return value * 0.0275f;
        default: return value * 0.02f;
        }
    }

    inline auto sniper_radius(int weapon_id, bool scoped, bool on_ground, bool ducking, float speed, float value) -> float
    {
        const float inverse = 1.0f - value;

        float spread = 0.0f;

        if (!on_ground)
        {
            if (weapon_id == weapon_awp)
                spread = 0.85f;
            else if (weapon_id == weapon_scout)
                spread = 0.2f;
            else
                spread = 0.45f * inverse;
        }
        else if (weapon_id == weapon_awp)
            spread = speed < 10.0f ? (ducking ? 0.0f : 0.001f) : (speed < 140.0f ? 0.1f : 0.25f);
        else if (weapon_id == weapon_scout)
            spread = speed < 170.0f ? (ducking ? 0.0f : 0.007f) : 0.075f;
        else if (speed != 0.0f)
            spread = 0.15f;
        else if (ducking)
            spread = (weapon_id == weapon_sg550 ? 0.04f : 0.035f) * inverse;
        else
            spread = (weapon_id == weapon_sg550 ? 0.05f : 0.055f) * inverse;

        if (scoped)
            return spread;

        return spread + (weapon_id == weapon_awp ? 0.08f : 0.025f);
    }

    inline auto radius(const c_weapon& local, bool scoped) -> float
    {
        const int weapon_id = local.get_id();

        switch (weapon_id)
        {
        case weapon_p228:
        case weapon_fiveseven:
        case weapon_deagle:
        case weapon_elite:
        case weapon_glock18:
        case weapon_usp:
            return pistol_radius(weapon_id, local.is_silenced(), local.is_in_burst(), (current.flags & 0x200) != 0, (current.flags & 0x4000) != 0, current.speed, current.value);
        case weapon_galil:
        case weapon_famas:
        case weapon_ak47:
        case weapon_sg552:
        case weapon_aug:
        case weapon_m249:
        case weapon_m4a1:
        case weapon_mp5navy:
        case weapon_mac10:
        case weapon_ump45:
        case weapon_tmp:
        case weapon_p90:
            return automatic_radius(weapon_id, local.is_silenced(), (current.flags & 0x200) != 0, current.speed, current.value);
        case weapon_scout:
        case weapon_awp:
        case weapon_g3sg1:
        case weapon_sg550:
            return sniper_radius(weapon_id, scoped, (current.flags & 0x200) != 0, (current.flags & 0x4000) != 0, current.speed, current.value);
        default:
            return 0.0f;
        }
    }
}