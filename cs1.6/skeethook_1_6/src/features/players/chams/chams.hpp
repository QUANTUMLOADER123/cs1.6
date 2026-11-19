#pragma once

#include <core/hooks.hpp>
#include <cstrike1_5/math/vector.hpp>

namespace chams
{
    enum chams_material
    {
        chams_textured,
        chams_flat,
        chams_solid,
        chams_glow,
    };

    inline const char* material_names[] =
    {
        "textured",
        "flat",
        "solid",
        "glow",
    };

    using gl_disable_t = void(__stdcall*)(unsigned int);
    using gl_enable_t = void(__stdcall*)(unsigned int);
    using gl_is_enabled_t = unsigned char(__stdcall*)(unsigned int);
    using gl_color4f_t = void(__stdcall*)(float, float, float, float);
    using gl_blend_func_t = void(__stdcall*)(unsigned int, unsigned int);
    using gl_depth_func_t = void(__stdcall*)(unsigned int);
    using gl_depth_mask_t = void(__stdcall*)(unsigned char);

    inline gl_disable_t gl_disable = nullptr;
    inline gl_enable_t gl_enable = nullptr;
    inline gl_is_enabled_t gl_is_enabled = nullptr;
    inline gl_color4f_t gl_color4f = nullptr;
    inline gl_blend_func_t gl_blend_func = nullptr;
    inline gl_depth_func_t gl_depth_func = nullptr;
    inline gl_depth_mask_t gl_depth_mask = nullptr;

    inline float* material_alpha = nullptr;

    inline hook model_hook;
    inline hook light_hook;

    inline bool unloading = false;
    inline bool override_color = false;
    inline bool shade_rim = false;
    inline float override_rgb[3] = { 0.0f, 0.0f, 0.0f };
    inline vec3_t view_forward{};

    auto __cdecl on_draw_model() -> int;
    auto __cdecl on_light_vertex(int light, float* normal, float* base, float* out) -> int;

    auto initialize() -> bool;
    auto shutdown() -> void;
}
