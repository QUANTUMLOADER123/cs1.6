#include <cstdint>
#include <imgui/imgui_internal.h>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/structs/cvar.hpp>
#include <cstrike1_5/interfaces/cvars/engine_cvars.hpp>
#include <features/vars.hpp>
#include <render/render.hpp>

#include <features/world/removals/removals.hpp>

#include <imgui/imgui.h>

namespace removals
{
    static auto __fastcall on_scope_overlay(void* self, void* edx_unused, float fl_time) -> int
    {
        auto original = scope_overlay_hook.get_original<decltype(&on_scope_overlay)>();

        if (vars::removals.toggles[removals::remove_scope])
            return 1;

        return original(self, edx_unused, fl_time);
    }

    static auto __cdecl on_scope_overlay_draw(int a1, int a2) -> void
    {
        auto original = scope_overlay_draw_hook.get_original<decltype(&on_scope_overlay_draw)>();

        if (vars::removals.toggles[removals::remove_scope])
            return;

        original(a1, a2);
    }

    auto initialize() -> bool
    {
        static bool flash_initialized = false;
        static bool smoke_initialized = false;
        static bool scope_overlay_initialized = false;
        static bool scope_overlay_draw_initialized = false;
        static bool scoped_initialized = false;

        if (!flash_initialized)
        {
            void* flash_address = memory::find_pattern(modules::mp, patterns::no_flash);
            if (!flash_address)
                return false;

            flash_hook.create(flash_address, reinterpret_cast<void*>(on_flash));
            flash_initialized = true;
        }

        if (!smoke_initialized)
        {
            void* smoke_address = memory::find_pattern(modules::client, patterns::no_smoke);
            if (!smoke_address)
                return false;

            smoke_hook.create(smoke_address, reinterpret_cast<void*>(on_smoke));
            smoke_initialized = true;
        }

        if (!scope_overlay_initialized)
        {
            void* scope_address = memory::find_pattern(modules::client, patterns::no_scope);
            if (!scope_address)
                return false;

            scope_overlay_hook.create(scope_address, reinterpret_cast<void*>(on_scope_overlay));
            scope_overlay_initialized = true;
        }

        if (!scope_overlay_draw_initialized)
        {
            void* scope_draw_address = memory::find_pattern(modules::renderer, patterns::no_scope_overlay);
            if (!scope_draw_address)
                return false;

            scope_overlay_draw_hook.create(scope_draw_address, reinterpret_cast<void*>(on_scope_overlay_draw));
            scope_overlay_draw_initialized = true;
        }

        if (!scoped_initialized)
        {
            uint8_t* match = memory::find_pattern(modules::client, patterns::scoped_fov);
            if (!match)
                return false;

            scoped_fov = reinterpret_cast<int*>(*reinterpret_cast<int*>(match + 2));
            scoped_initialized = true;
        }

        return true;
    }

    auto poll() -> void
    {
        static cvar_t* decals = nullptr;
        if (!decals)
            decals = engine_cvars::find("r_decals");
        if (!decals)
            return;

        static float original = 4096.0f;
        static bool stored = false;

        if (vars::removals.toggles[3])
        {
            if (!stored)
            {
                original = decals->value;
                stored = true;
            }

            decals->value = 0.0f;
            return;
        }

        if (stored)
        {
            decals->value = original;
            stored = false;
        }
    }

    auto render() -> void
    {
        if (!vars::removals.custom_scope || !scoped_fov || *scoped_fov >= 90)
            return;

        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        ImU32 color = IM_COL32(0, 0, 0, 255);
        float thickness = vars::removals.custom_scope_size;
        float center_x = static_cast<float>(render::width) * 0.5f;
        float center_y = static_cast<float>(render::height) * 0.5f;

        draw_list->AddLine(ImVec2(center_x, 0.0f), ImVec2(center_x, static_cast<float>(render::height)), color, thickness);
        draw_list->AddLine(ImVec2(0.0f, center_y), ImVec2(static_cast<float>(render::width), center_y), color, thickness);
    }

    auto __fastcall on_flash(void* self, void* edx_unused, float* flash_color, int third_field) -> int
    {
        auto original = flash_hook.get_original<decltype(&on_flash)>();

        if (vars::removals.toggles[0])
            return 0;

        return original(self, edx_unused, flash_color, third_field);
    }

    auto __cdecl on_smoke(int a1, int a2, int a3, float a4, float a5, float a6, char a7, char a8, char a9) -> char
    {
        auto original = smoke_hook.get_original<decltype(&on_smoke)>();

        if (vars::removals.toggles[1])
            return 0;

        return original(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    }
}