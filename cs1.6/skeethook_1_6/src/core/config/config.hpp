#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <features/vars.hpp>

namespace config
{
    struct blob_t
    {
        uint32_t magic;
        uint32_t version;
        vars::esp_t esp;
        vars::chams_t chams;
        vars::misc_t misc;
        vars::third_person_t third_person;
        vars::nightmode_t nightmode;
        vars::skybox_t skybox;
        vars::aspect_ratio_t aspect_ratio;
        vars::viewmodel_t viewmodel;
        vars::fov_changer_t fov_changer;
        vars::antihit_t antihit;
        vars::removals_t removals;
        vars::model_changer_t model_changer;
        vars::bullet_tracers_t bullet_tracers;
        vars::bullet_impacts_t bullet_impacts;
        vars::ragebot_t ragebot;
        vars::legitbot_t legitbot;
    };

    auto directory() -> const char*;
    auto refresh() -> void;
    auto exists(const char* name) -> bool;
    auto save(const char* name) -> bool;
    auto load(const char* name) -> bool;
    auto remove(const char* name) -> bool;
    auto open_directory() -> void;
    auto names() -> const std::vector<std::string>&;
    auto selected() -> int&;
}
