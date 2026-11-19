#pragma once

#include <string>
#include <vector>

#include <core/hooks.hpp>

namespace skybox
{
    struct sky_entry_t
    {
        char name[32];
        int faces;
    };

    inline hook draw_hook;
    inline int* enabled_flag = nullptr;
    inline float* color = nullptr;
    inline int* reload_flag = nullptr;
    inline char* name_buffer = nullptr;
    inline void* load_address = nullptr;
    inline std::vector<std::string> names;

    auto initialize() -> bool;
    auto refresh() -> void;
    auto selected() -> int;
    auto select(int index) -> void;
    auto combo_item(void* user, int index) -> const char*;
    auto poll() -> void;
    auto __cdecl on_draw_skybox() -> int;
}
