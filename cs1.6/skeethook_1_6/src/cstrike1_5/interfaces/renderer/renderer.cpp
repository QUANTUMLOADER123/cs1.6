#include <cstdint>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/address.hpp>
#include <memory/scanner.hpp>

#include <cstrike1_5/interfaces/renderer/renderer.hpp>

auto renderer::initialize() -> bool
{
    uint8_t* world_transform_match = memory::find_pattern(modules::renderer, patterns::world_transform);
    uint8_t* screen_match = memory::find_pattern(modules::renderer, patterns::screen);
    uint8_t* viewport_match = memory::find_pattern(modules::renderer, patterns::viewport);
    if (!world_transform_match || !screen_match || !viewport_match)
        return false;

    world_matrix = reinterpret_cast<const float*>(memory::global_address(world_transform_match, 0xC));

    screen_origin_x = reinterpret_cast<const int*>(memory::global_address(screen_match, 2));
    screen_width = reinterpret_cast<const int*>(memory::global_address(screen_match, 7));
    screen_origin_y = reinterpret_cast<const int*>(memory::global_address(screen_match, 0xD));
    screen_height = reinterpret_cast<const int*>(memory::global_address(screen_match, 0x15));

    viewport_y = reinterpret_cast<const int*>(memory::global_address(viewport_match, 2));
    viewport_x = reinterpret_cast<const int*>(memory::global_address(viewport_match, 0xC));

    return true;
}

auto renderer::world_to_screen(const vec3_t& world, vec2_t& screen) -> bool
{
    if (!world_matrix)
        return false;

    const float clip_x = world_matrix[0] * world.x + world_matrix[4] * world.y + world_matrix[8] * world.z + world_matrix[12];
    const float clip_y = world_matrix[1] * world.x + world_matrix[5] * world.y + world_matrix[9] * world.z + world_matrix[13];
    const float clip_w = world_matrix[3] * world.x + world_matrix[7] * world.y + world_matrix[11] * world.z + world_matrix[15];

    if (clip_w <= 0.0f)
        return false;

    const float inverse_w = 1.0f / clip_w;

    screen.x = static_cast<float>(*viewport_x + *screen_origin_x) + (1.0f + clip_x * inverse_w) * 0.5f * static_cast<float>(*screen_width);
    screen.y = static_cast<float>(*viewport_y + *screen_origin_y) + (1.0f - clip_y * inverse_w) * 0.5f * static_cast<float>(*screen_height);

    return true;
}
