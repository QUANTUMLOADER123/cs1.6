#pragma once

#include <cstrike1_5/structs/cvar.hpp>

namespace engine_cvars
{
    auto initialize() -> bool;
    auto find(const char* name) -> cvar_t*;
    auto get_float(const char* name) -> float;
    auto get_int(const char* name) -> int;
    auto get_string(const char* name) -> const char*;
    auto execute(const char* command) -> void;
}