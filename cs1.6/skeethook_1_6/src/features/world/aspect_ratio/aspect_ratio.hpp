#pragma once

namespace aspect_ratio
{
    auto initialize() -> bool;
    auto poll() -> void;
    auto shutdown() -> void;
    auto __stdcall on_glfrustum(double left, double right, double bottom, double top, double z_near, double z_far) -> void;
}