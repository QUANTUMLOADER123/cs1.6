#pragma once

namespace killsounds
{
    using play_t = bool (__stdcall*)(const wchar_t*, void*, unsigned long);
    using saytext_t = int (__thiscall*)(void* self, int name, int size, int buffer);

    inline constexpr auto count = 3;
    inline constexpr const char* names[count] = { "neverlose", "rust headshot", "skeet" };

    inline play_t play_sound = nullptr;
    inline saytext_t saytext_original = nullptr;

    auto initialize() -> bool;
    auto install_hook() -> bool;
    auto play(int index) -> bool;
    auto on_kill(void* self, int name, int size, int buffer) -> int;
    auto __fastcall on_kill_detour(void* self, int unused, int name, int size, int buffer) -> int;
}
