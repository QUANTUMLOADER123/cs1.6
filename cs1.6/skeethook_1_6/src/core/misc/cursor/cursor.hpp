#pragma once

namespace cursor
{
    auto poll() -> void;

    /* доступен ли SDL2-путь управления курсором */
    auto usable() -> bool;
}