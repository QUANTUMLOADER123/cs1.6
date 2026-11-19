#pragma once

#include <cstrike1_5/classes/c_base_player.hpp>
#include <cstrike1_5/structs/player_info.hpp>

class c_player : public c_base_player
{
public:
    c_player() = default;
    c_player(cl_entity_t* entity, player_info_t* player_info) : c_base_player(entity), info(player_info) {}

    auto get_name() -> const char* { return info ? info->name : ""; }

    player_info_t* info = nullptr;
};