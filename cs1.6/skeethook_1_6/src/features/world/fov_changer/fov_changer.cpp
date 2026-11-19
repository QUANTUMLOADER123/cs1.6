#include <cstdint>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/address.hpp>
#include <memory/scanner.hpp>
#include <features/vars.hpp>

#include <features/world/fov_changer/fov_changer.hpp>

namespace fov_changer
{
    auto initialize() -> bool
    {
        uint8_t* match = memory::find_pattern(modules::client_mini, patterns::fov_scaled_write);
        if (!match)
            return false;

        fov_scaled = reinterpret_cast<float*>(memory::global_address(match, 4));
        fov_previous = fov_scaled + 1;
        fov_current = fov_scaled + 2;
        fov_target = fov_scaled + 3;

        return true;
    }

    auto poll() -> void
    {
        if (!fov_target)
            return;

        if (!vars::fov_changer.enabled)
        {
            if (*fov_target == last_custom_fov)
            {
                *fov_target = 90.0f;
                *fov_current = 90.0f;
                *fov_previous = 90.0f;
                *fov_scaled = 1.0f;
            }

            return;
        }

        float custom_fov = vars::fov_changer.fov;
        if (custom_fov < 1.0f || custom_fov > 170.0f)
            custom_fov = 90.0f;

        if (*fov_target == custom_fov)
            return;

        *fov_target = custom_fov;
        *fov_current = custom_fov;
        *fov_previous = custom_fov;
        *fov_scaled = custom_fov / 90.0f;
        last_custom_fov = custom_fov;
    }
}