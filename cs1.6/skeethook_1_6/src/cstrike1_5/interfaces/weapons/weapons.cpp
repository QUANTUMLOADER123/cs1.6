#include <cstdint>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/address.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/classes/c_weapon.hpp>

#include <cstrike1_5/interfaces/weapons/weapons.hpp>

static uint8_t** local_weapon_global = nullptr;

auto weapons::initialize() -> bool
{
    uint8_t* match = memory::find_pattern(modules::client, patterns::local_weapon);
    if (!match)
        return false;

    local_weapon_global = reinterpret_cast<uint8_t**>(memory::global_address(match, 2));
    return local_weapon_global != nullptr;
}

auto weapons::local() -> c_weapon
{
    if (!local_weapon_global)
        return c_weapon();

    return c_weapon(*local_weapon_global);
}
