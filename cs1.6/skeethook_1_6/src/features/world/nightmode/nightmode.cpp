#include <cstdint>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/address.hpp>
#include <memory/scanner.hpp>
#include <features/vars.hpp>

#include <features/world/nightmode/nightmode.hpp>

namespace nightmode
{
    auto initialize() -> bool
    {
        static bool initialized = false;
        if (initialized)
            return true;

        uint8_t* build_match = memory::find_pattern(modules::renderer, patterns::build_lightmap);
        uint8_t* format_match = memory::find_pattern(modules::renderer, patterns::lightmap_format);
        uint8_t* rebuild_match = memory::find_pattern(modules::renderer, patterns::rebuild_lightmaps);
        if (!build_match || !format_match || !rebuild_match)
            return false;

        lightmap_format = reinterpret_cast<int*>(memory::global_address(format_match, 1));
        rebuild_address = rebuild_match;

        build_hook.create(build_match, reinterpret_cast<void*>(on_build_lightmap));
        initialized = true;

        return true;
    }

    auto poll() -> void
    {
        static bool first = true;
        static bool applied_enabled = false;
        static float applied_color[3] = { 0.0f, 0.0f, 0.0f };
        static bool seen_enabled = false;
        static float seen_color[3] = { 0.0f, 0.0f, 0.0f };

        const bool enabled = vars::nightmode.enabled;
        const float color[3] = { vars::nightmode.color[0], vars::nightmode.color[1], vars::nightmode.color[2] };

        if (first)
        {
            first = false;
            applied_enabled = enabled;
            seen_enabled = enabled;
            for (int i = 0; i < 3; ++i)
            {
                applied_color[i] = color[i];
                seen_color[i] = color[i];
            }
            return;
        }

        const bool steady = enabled == seen_enabled
            && color[0] == seen_color[0]
            && color[1] == seen_color[1]
            && color[2] == seen_color[2];

        seen_enabled = enabled;
        for (int i = 0; i < 3; ++i)
            seen_color[i] = color[i];

        if (!steady)
            return;

        const bool applied_same = enabled == applied_enabled
            && color[0] == applied_color[0]
            && color[1] == applied_color[1]
            && color[2] == applied_color[2];

        if (applied_same)
            return;

        applied_enabled = enabled;
        for (int i = 0; i < 3; ++i)
            applied_color[i] = color[i];

        reinterpret_cast<int(*)()>(rebuild_address)();
    }

    auto __cdecl on_build_lightmap(int surface, uint8_t* lightmap, int stride) -> int
    {
        auto original = build_hook.get_original<decltype(&on_build_lightmap)>();

        const int result = original(surface, lightmap, stride);

        if (!vars::nightmode.enabled || *lightmap_format != 6408)
            return result;

        const int width = (*reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(surface) + 24) >> 4) + 1;
        const int height = (*reinterpret_cast<int16_t*>(reinterpret_cast<uint8_t*>(surface) + 26) >> 4) + 1;

        const float night_color[3] = { vars::nightmode.color[0], vars::nightmode.color[1], vars::nightmode.color[2] };

        uint8_t* row = lightmap;
        for (int y = 0; y < height; ++y)
        {
            uint8_t* pixel = row;
            for (int x = 0; x < width; ++x)
            {
                for (int channel = 0; channel < 3; ++channel)
                {
                    const float value = static_cast<float>(pixel[channel]) * night_color[channel];
                    pixel[channel] = value >= 255.0f ? 255 : static_cast<uint8_t>(value);
                }
                pixel += 4;
            }
            row += stride;
        }

        return result;
    }
}