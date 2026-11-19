#pragma once

namespace console
{
    auto create() -> bool;
    auto log(const char* format, ...) -> void;
    auto destroy() -> void;
}