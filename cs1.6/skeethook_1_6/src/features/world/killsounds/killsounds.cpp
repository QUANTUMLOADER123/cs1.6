#include <cstdint>
#include <minhook.h>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/interfaces/engine/engine.hpp>
#include <features/vars.hpp>
#include <core/console.hpp>

#include <features/world/killsounds/killsounds.hpp>
#include <features/world/killsounds/killsounds_data.hpp>

namespace killsounds
{
    auto initialize() -> bool
    {
        HMODULE winmm = LoadLibraryA("winmm.dll");

        if (!winmm)
        {
            console::log("[killsounds] winmm.dll not found");
            return false;
        }

        play_sound = reinterpret_cast<play_t>(GetProcAddress(winmm, "PlaySoundW"));

        if (!play_sound)
        {
            console::log("[killsounds] PlaySoundW not found");
            return false;
        }

        if (!install_hook())
            console::log("[killsounds] killfeed unavailable");

        return true;
    }

    auto install_hook() -> bool
    {
        void* address = memory::find_pattern(modules::client, patterns::killfeed_saytext);

        if (!address)
            return false;

        void* trampoline = nullptr;

        if (MH_CreateHook(address, reinterpret_cast<void*>(on_kill_detour), &trampoline) != MH_OK)
            return false;

        if (MH_EnableHook(address) != MH_OK)
        {
            MH_RemoveHook(address);
            return false;
        }

        saytext_original = reinterpret_cast<saytext_t>(trampoline);

        return true;
    }

    auto play(int index) -> bool
    {
        if (!play_sound || index < 0 || index >= count)
            return false;

        const unsigned char* sample[count] = { neverlose, headshot, skeet };

        return play_sound(reinterpret_cast<const wchar_t*>(sample[index]), nullptr, 0x0004 | 0x0001 | 0x0002);
    }

    auto on_kill(void* self, int name, int size, int buffer) -> int
    {
        const int result = saytext_original(self, name, size, buffer);

        if (!vars::misc.killsound.enabled || size < 3)
            return result;

        const auto* bytes = reinterpret_cast<const uint8_t*>(buffer);

        if (bytes[0] != engine::local_player_index())
            return result;

        console::log("[killsounds] victim=%d headshot=%d sound=%d", bytes[1], bytes[2], play(vars::misc.killsound.sound));

        return result;
    }

    auto __fastcall on_kill_detour(void* self, int unused, int name, int size, int buffer) -> int
    {
        return on_kill(self, name, size, buffer);
    }
}
