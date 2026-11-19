#pragma once

#include <render/menu_widgets.hpp>

namespace menu
{
    void render();

    inline bool opened = false;
    inline int selected_tab = 0;
    inline int selected_subtab = 0;

    inline char config_name[64] = {};
    inline bool config_scanned = false;
}