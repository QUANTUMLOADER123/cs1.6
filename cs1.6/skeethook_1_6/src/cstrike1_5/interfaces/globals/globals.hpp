#pragma once

#include <cstrike1_5/structs/client_state.hpp>

namespace globals
{
    static client_state_t** client_state_global = nullptr;

    auto initialize() -> bool;
    auto time() -> double;
    auto old_time() -> double;
    auto frame_time() -> float;
    auto tick_interval() -> double;
    auto tick_count() -> int;
}