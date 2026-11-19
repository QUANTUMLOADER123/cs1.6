#pragma once

#include <imgui/imgui.h>

#include <pe32/pe32.h>
#include <cstrike1_5/math/vector.hpp>

extern ImFont* tahoma_font;

namespace render
{
    bool initialize(HDC device_context);
    void shutdown();
    void draw(HDC device_context);
    void apply_style();
    bool wts(const vec3_t& world, vec2_t& screen);

    inline HWND window = nullptr;
    inline int width = 0;
    inline int height = 0;
}
