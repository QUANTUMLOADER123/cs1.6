#pragma once

#include <cstrike1_5/structs/client_state.hpp>

namespace engine
{
    static client_state_t** client_state_global = nullptr;
    static void** entity_list_global = nullptr;
    static const char* team_name_storage = nullptr;

    auto initialize() -> bool;
    auto client_state() -> client_state_t*;
    auto entities() -> cl_entity_t*;
    auto local_player_index() -> int;
    auto max_clients() -> int;
    auto entity(int entity_index) -> cl_entity_t*;
    auto player_info(int entity_index) -> player_info_t*;
    auto player_team(int entity_index) -> int;
    auto model_name(int model_index) -> const char*;
    auto local_eye() -> vec3_t;
}
