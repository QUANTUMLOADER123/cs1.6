#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <imgui/imgui.h>

namespace sprites
{
    using gl_gen_textures_t = void(__stdcall*)(int, unsigned int*);
    using gl_bind_texture_t = void(__stdcall*)(int, unsigned int);
    using gl_tex_image_2d_t = void(__stdcall*)(int, int, int, int, int, int, int, int, const void*);
    using gl_tex_parameteri_t = void(__stdcall*)(int, int, int);

    struct sprite_header_t
    {
        int ident;
        int version;
        int type;
        int texformat;
        float boundingradius;
        int width;
        int height;
        int numframes;
        float beamlength;
        int synctype;
    };

    struct sprite_frame_t
    {
        int group;
        int origin[2];
        int width;
        int height;
        int paletteindex;
    };

    struct sprite_rect_t
    {
        std::string sprite;
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
    };

    struct atlas_entry_t
    {
        unsigned int texture = 0;
        int width = 0;
        int height = 0;
        int scale = 1;
    };

    inline gl_gen_textures_t gen_textures = nullptr;
    inline gl_bind_texture_t bind_texture = nullptr;
    inline gl_tex_image_2d_t tex_image_2d = nullptr;
    inline gl_tex_parameteri_t tex_parameteri = nullptr;

    inline std::string sprites_directory;
    inline std::string hud_path;

    inline std::unordered_map<std::string, sprite_rect_t> rects;
    inline std::unordered_map<std::string, atlas_entry_t> atlas_cache;

    inline std::unordered_set<std::string> missed_icons;
    inline std::unordered_set<std::string> shown_icons;

    auto initialize() -> bool;
    auto find_rect(const std::string& name) -> const sprite_rect_t*;
    auto atlas_for(const std::string& sprite_name) -> const atlas_entry_t*;
    auto draw(ImDrawList* draw_list, const char* weapon_name, const ImVec2& box_min, const ImVec2& box_max) -> bool;
}