#pragma once

namespace model_changer
{
    inline const char* type_names[] =
    {
        "game",
        "custom",
    };

    inline const char* model_names[] =
    {
        "urban",
        "terror",
        "leet",
        "arctic",
        "gsg9",
        "gign",
        "sas",
        "guerilla",
        "vip",
        "militia",
        "spetsnaz",
    };

    auto poll() -> void;
}
