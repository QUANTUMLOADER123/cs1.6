#include <cstdint>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/address.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/structs/cl_entity.hpp>
#include <cstrike1_5/structs/player_info.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/structs/bsp.hpp>

#include <cstrike1_5/interfaces/engine/engine.hpp>

auto engine::initialize() -> bool
{
    uint8_t* client_match = memory::find_pattern(modules::engine, patterns::client);
    uint8_t* entities_match = memory::find_pattern(modules::engine, patterns::entities);
    uint8_t* team_matcher = memory::find_pattern(modules::client, patterns::team_names);
    if (!client_match || !entities_match || !team_matcher)
        return false;

    client_state_global = reinterpret_cast<client_state_t**>(memory::global_address(client_match, 2));
    entity_list_global = reinterpret_cast<void**>(memory::global_address(entities_match, 2));

    team_name_storage = reinterpret_cast<const char*>(memory::global_address(team_matcher, 3));

    return client_state_global && entity_list_global && team_name_storage;
}

auto engine::client_state() -> client_state_t*
{
    return client_state_global ? *client_state_global : nullptr;
}

auto engine::entities() -> cl_entity_t*
{
    if (!entity_list_global)
        return nullptr;

    void* entity_list = *entity_list_global;
    if (!entity_list)
        return nullptr;

    return *reinterpret_cast<cl_entity_t**>(entity_list);
}

auto engine::local_player_index() -> int
{
    client_state_t* state = client_state();
    return state ? state->viewentity : 0;
}

auto engine::max_clients() -> int
{
    client_state_t* state = client_state();
    return state ? state->maxclients : 0;
}

auto engine::entity(int entity_index) -> cl_entity_t*
{
    cl_entity_t* entity_list = entities();
    if (!entity_list || entity_index < 0)
        return nullptr;

    return &entity_list[entity_index];
}

auto engine::player_info(int entity_index) -> player_info_t*
{
    client_state_t* state = client_state();
    if (!state || entity_index <= 0 || entity_index > state->maxclients)
        return nullptr;

    return &state->players[entity_index - 1];
}

auto engine::player_team(int entity_index) -> int
{
    if (!team_name_storage || entity_index <= 0 || entity_index > 64)
        return 0;

    const char* team_name = team_name_storage + 116 * entity_index;
    if (team_name[0] == 'C')
        return 2;

    if (team_name[0] == 'T')
        return 1;

    return 0;
}

auto engine::model_name(int model_index) -> const char*
{
    client_state_t* state = client_state();
    if (!state || model_index <= 0 || model_index >= state->model_precache_count)
        return nullptr;

    model_t* model = reinterpret_cast<model_t*>(state->model_precache[model_index]);
    return model ? model->name : nullptr;
}

auto engine::local_eye() -> vec3_t
{
    cl_entity_t* local = entity(local_player_index());
    client_state_t* state = client_state();

    if (!local || !state)
        return vec3_t();

    return local->origin + state->viewheight;
}
