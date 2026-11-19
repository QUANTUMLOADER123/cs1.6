#pragma once

#include <cstdint>

#include <cstrike1_5/enums.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/interfaces/engine/engine.hpp>

class c_weapon
{
public:
    c_weapon() = default;
    explicit c_weapon(uint8_t* weapon) : base(weapon) {}

    auto is_valid() const -> bool { return base != nullptr; }

    auto get_id() const -> int { return base ? *reinterpret_cast<int*>(base + 172) : 0; }

    auto is_silenced() const -> bool
    {
        const weapon_data_t* weapon_data = get_weapon_data();
        if (!weapon_data)
            return false;

        if (get_id() == weapon_m4a1 && weapon_data->m_iWeaponState & (1 << 2))
            return true;

        return get_id() == weapon_usp && weapon_data->m_iWeaponState & (1 << 0);
    }

    auto is_in_burst() const -> bool
    {
        const weapon_data_t* weapon_data = get_weapon_data();
        if (!weapon_data)
            return false;

        return get_id() == weapon_glock18 && weapon_data->m_iWeaponState & (1 << 1);
    }

    uint8_t* base = nullptr;

private:
    auto get_weapon_data() const -> const weapon_data_t*
    {
        const int id = get_id();
        if (id <= 0 || id >= 64)
            return nullptr;

        client_state_t* state = engine::client_state();
        if (!state)
            return nullptr;

        const local_state_t& prediction = state->predicted_frames[state->parsecountmod & 63];
        if (prediction.weapondata[id].m_iId != id)
            return nullptr;

        return &prediction.weapondata[id];
    }
};