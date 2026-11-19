#include <cstdint>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/address.hpp>
#include <memory/scanner.hpp>
#include <features/vars.hpp>

#include <features/world/aspect_ratio/aspect_ratio.hpp>

namespace aspect_ratio
{
    using frustum_fn = void(__stdcall*)(double, double, double, double, double, double);

    void** frustum_slot = nullptr;
    frustum_fn original_frustum = nullptr;

    auto initialize() -> bool
    {
        uint8_t* match = memory::find_pattern(modules::renderer, patterns::camera_frustum);
        if (!match)
            return false;

        frustum_slot = reinterpret_cast<void**>(memory::global_address(match, 0x6F));
        return frustum_slot != nullptr;
    }

    auto poll() -> void
    {
        if (!frustum_slot)
            return;

        void* stored = *frustum_slot;
        if (!stored)
            return;

        if (!vars::aspect_ratio.enabled)
        {
            if (original_frustum && stored != reinterpret_cast<void*>(original_frustum))
                *frustum_slot = reinterpret_cast<void*>(original_frustum);
            return;
        }

        if (!original_frustum)
            original_frustum = reinterpret_cast<frustum_fn>(stored);

        if (stored != reinterpret_cast<void*>(on_glfrustum))
            *frustum_slot = reinterpret_cast<void*>(on_glfrustum);
    }

    auto shutdown() -> void
    {
        if (!frustum_slot || !original_frustum)
            return;

        if (*frustum_slot == reinterpret_cast<void*>(on_glfrustum))
            *frustum_slot = reinterpret_cast<void*>(original_frustum);
    }

    auto __stdcall on_glfrustum(double left, double right, double bottom, double top, double z_near, double z_far) -> void
    {
        if (!original_frustum)
            return;

        if (vars::aspect_ratio.enabled)
        {
            const double horizontal = right - left;
            const double vertical = top - bottom;
            if (horizontal > 0.0 && vertical > 0.0)
            {
                const double scale = static_cast<double>(vars::aspect_ratio.aspect) * vertical / horizontal;
                left *= scale;
                right *= scale;
            }
        }

        original_frustum(left, right, bottom, top, z_near, z_far);
    }
}