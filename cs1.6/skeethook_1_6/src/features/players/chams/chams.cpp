#include <cstdint>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/address.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/structs/cl_entity.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/interfaces/engine/engine.hpp>
#include <cstrike1_5/interfaces/studio/studio.hpp>
#include <features/vars.hpp>

#include <features/players/chams/chams.hpp>

namespace chams
{
    static auto begin_override(const float* color, bool rim) -> float
    {
        const float previous = material_alpha ? *material_alpha : 1.0f;
        if (material_alpha)
            *material_alpha = color[3];

        gl_color4f(color[0], color[1], color[2], color[3]);
        override_rgb[0] = color[0];
        override_rgb[1] = color[1];
        override_rgb[2] = color[2];
        shade_rim = rim;
        override_color = true;
        return previous;
    }

    static auto end_override(float previous) -> void
    {
        override_color = false;
        shade_rim = false;
        if (material_alpha)
            *material_alpha = previous;
    }

    static auto draw_block(int(__cdecl* original)(), const float* color, int material, unsigned int depth_func, bool write_depth) -> int
    {
        const bool textured = gl_is_enabled(3553) != 0;
        const bool depth_tested = gl_is_enabled(2929) != 0;
        const bool blended = gl_is_enabled(3042) != 0;
        const bool untextured = material != chams_textured;
        const bool glow = material == chams_glow;

        gl_depth_mask(write_depth ? 1 : 0);
        gl_depth_func(depth_func);

        if (!depth_tested)
            gl_enable(2929);
        if (!blended)
            gl_enable(3042);

        if (untextured)
        {
            if (textured)
                gl_disable(3553);
        }
        else if (!textured)
            gl_enable(3553);

        gl_blend_func(770, glow ? 1 : 771);
        const float previous = begin_override(color, material == chams_solid || glow);
        const int result = original();
        end_override(previous);

        gl_blend_func(770, 771);
        gl_depth_func(515);
        gl_depth_mask(1);

        if (untextured)
        {
            if (textured)
                gl_enable(3553);
        }
        else if (!textured)
            gl_disable(3553);

        if (!blended)
            gl_disable(3042);
        if (!depth_tested)
            gl_disable(2929);

        return result;
    }

    auto initialize() -> bool
    {
        static bool initialized = false;
        if (initialized)
            return true;

        HMODULE opengl = GetModuleHandleA("opengl32.dll");
        if (!opengl)
            return false;

        gl_disable = reinterpret_cast<gl_disable_t>(GetProcAddress(opengl, "glDisable"));
        gl_enable = reinterpret_cast<gl_enable_t>(GetProcAddress(opengl, "glEnable"));
        gl_is_enabled = reinterpret_cast<gl_is_enabled_t>(GetProcAddress(opengl, "glIsEnabled"));
        gl_color4f = reinterpret_cast<gl_color4f_t>(GetProcAddress(opengl, "glColor4f"));
        gl_blend_func = reinterpret_cast<gl_blend_func_t>(GetProcAddress(opengl, "glBlendFunc"));
        gl_depth_func = reinterpret_cast<gl_depth_func_t>(GetProcAddress(opengl, "glDepthFunc"));
        gl_depth_mask = reinterpret_cast<gl_depth_mask_t>(GetProcAddress(opengl, "glDepthMask"));
        if (!gl_disable || !gl_enable || !gl_is_enabled || !gl_color4f || !gl_blend_func || !gl_depth_func || !gl_depth_mask)
            return false;

        uint8_t* model_match = memory::find_pattern(modules::renderer, patterns::chams_model);
        uint8_t* light_match = memory::find_pattern(modules::renderer, patterns::chams_light);
        uint8_t* alpha_match = memory::find_pattern(modules::renderer, patterns::chams_alpha);
        if (!model_match || !light_match || !alpha_match)
            return false;

        material_alpha = reinterpret_cast<float*>(memory::global_address(alpha_match, 34));

        model_hook.create(model_match, reinterpret_cast<void*>(on_draw_model));
        light_hook.create(light_match, reinterpret_cast<void*>(on_light_vertex));
        initialized = true;

        return true;
    }

    auto shutdown() -> void
    {
        unloading = true;
        override_color = false;
        shade_rim = false;
        vars::chams.enabled = false;

        model_hook.disable();
        light_hook.disable();
    }

    static auto draw_style(int(__cdecl* original)(), const vars::chams_style_t& style) -> int
    {
        int result = 0;

        if (style.ignore_z)
            result = draw_block(original, style.occluded_color, style.material, 519, 0);

        result = draw_block(original, style.visible_color, style.material, 515, 1);
        return result;
    }

    auto __cdecl on_draw_model() -> int
    {
        auto original = model_hook.get_original<decltype(&on_draw_model)>();
        if (!original || unloading)
            return original ? original() : 0;

        studio::capture();

        cl_entity_t* entity = studio::current_entity();
        if (!entity)
            return original();

        client_state_t* state = engine::client_state();
        const int local_index = engine::local_player_index();

        if (state && entity == &state->viewent)
        {
            if (!vars::chams.hands.enabled)
                return original();

            return draw_style(original, vars::chams.hands);
        }

        if (!entity->player)
            return original();

        if (entity->index == local_index)
        {
            if (!vars::chams.local.enabled)
                return original();

            return draw_style(original, vars::chams.local);
        }

        const int local_team = engine::player_team(local_index);
        const int entity_team = engine::player_team(entity->index);
        const bool chams_target = vars::chams.enabled && !(vars::chams.enemies_only && entity_team != 0 && entity_team == local_team);
        if (!chams_target)
            return original();

        int result = 0;

        if (vars::chams.ignore_z)
            result = draw_block(original, vars::chams.occluded_color, vars::chams.material, 519, 0);

        result = draw_block(original, vars::chams.visible_color, vars::chams.material, 515, 1);
        return result;
    }

    auto __cdecl on_light_vertex(int light, float* normal, float* base, float* out) -> int
    {
        auto original = light_hook.get_original<decltype(&on_light_vertex)>();
        if (!original || unloading)
            return original ? original(light, normal, base, out) : 0;

        const int result = original(light, normal, base, out);

        if (!override_color)
            return result;

        if (!shade_rim)
        {
            out[0] = override_rgb[0];
            out[1] = override_rgb[1];
            out[2] = override_rgb[2];
            return result;
        }

        float light_value = (out[0] + out[1] + out[2]) * (1.0f / 3.0f);
        if (light_value < 0.0f)
            light_value = 0.0f;
        if (light_value > 1.0f)
            light_value = 1.0f;

        float rim = 0.2f;
        if (normal && view_forward.is_valid())
        {
            vec3_t shaded_normal(normal[0], normal[1], normal[2]);
            shaded_normal.normalize();

            float facing = shaded_normal.x * view_forward.x + shaded_normal.y * view_forward.y + shaded_normal.z * view_forward.z;
            if (facing < 0.0f)
                facing = -facing;
            if (facing > 1.0f)
                facing = 1.0f;

            const float edge = 1.0f - facing;
            rim = edge * edge * edge;
        }

        float shade = 0.06f + light_value * 0.28f + rim * 0.82f;
        if (shade > 1.0f)
            shade = 1.0f;

        out[0] = override_rgb[0] * shade;
        out[1] = override_rgb[1] * shade;
        out[2] = override_rgb[2] * shade;
        return result;
    }
}
