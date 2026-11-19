#pragma once

#include <core/hooks.hpp>

namespace removals
{
    inline hook flash_hook;
    inline hook smoke_hook;
    inline hook scope_overlay_hook;
    inline hook scope_overlay_draw_hook;

    inline const char* items[] =
    {
        "flashbang",
        "smoke",
        "remove scope",
        "decals",
        "visual recoil",
        "landing bob",
    };

    inline constexpr int remove_scope = 2;
    inline constexpr int visual_recoil = 4;
    inline constexpr int landing_bob = 5;

    inline int* scoped_fov = nullptr;

    auto initialize() -> bool;
    auto poll() -> void;
    auto render() -> void;
    auto __fastcall on_flash(void* self, void* edx_unused, float* flash_color, int third_field) -> int;
    auto __cdecl on_smoke(int a1, int a2, int a3, float a4, float a5, float a6, char a7, char a8, char a9) -> char;
}