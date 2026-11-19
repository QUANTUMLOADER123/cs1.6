#pragma once

#include <cstdint>

#include <core/hooks.hpp>

namespace nightmode
{
    inline hook build_hook;
    inline int* lightmap_format = nullptr;
    inline void* rebuild_address = nullptr;

    auto initialize() -> bool;
    auto poll() -> void;
    auto __cdecl on_build_lightmap(int surface, uint8_t* lightmap, int stride) -> int;
}