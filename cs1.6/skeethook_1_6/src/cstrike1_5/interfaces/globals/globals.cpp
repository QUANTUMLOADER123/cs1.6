#include <cstdint>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/address.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/structs/client_state.hpp>

#include <cstrike1_5/interfaces/globals/globals.hpp>

auto globals::initialize() -> bool
{
    uint8_t* client_match = memory::find_pattern(modules::engine, patterns::client);
    if (!client_match)
        return false;

    client_state_global = reinterpret_cast<client_state_t**>(memory::global_address(client_match, 2));

    return client_state_global != nullptr;
}

auto globals::time() -> double
{
    client_state_t* state = client_state_global ? *client_state_global : nullptr;
    return state ? state->time : 0.0;
}

auto globals::old_time() -> double
{
    client_state_t* state = client_state_global ? *client_state_global : nullptr;
    return state ? state->oldtime : 0.0;
}

auto globals::frame_time() -> float
{
    return static_cast<float>(time() - old_time());
}

auto globals::tick_interval() -> double
{
    return 1.0 / 64.0;
}

auto globals::tick_count() -> int
{
    return static_cast<int>(time() / tick_interval());
}