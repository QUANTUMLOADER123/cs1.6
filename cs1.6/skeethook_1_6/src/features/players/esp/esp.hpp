#pragma once

#include <imgui/imgui.h>

#include <cstrike1_5/classes/c_player.hpp>
#include <cstrike1_5/structs/cl_entity.hpp>

namespace esp
{
    auto draw() -> void;
    auto name(ImDrawList* draw_list, c_player* player, const ImVec2& box_min, const ImVec2& box_max) -> void;
    auto weapon(ImDrawList* draw_list, c_player* player, const ImVec2& box_min, const ImVec2& box_max) -> void;
    auto distance(ImDrawList* draw_list, float meters, const ImVec2& box_min, const ImVec2& box_max, float y) -> void;
    auto corners(ImDrawList* draw_list, const ImVec2& box_min, const ImVec2& box_max) -> void;
    auto skeleton(ImDrawList* draw_list, cl_entity_t* entity) -> void;
}
