#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>
#include <imgui/imgui_internal.h>
#include <core/console.hpp>
#include <render/sprites.hpp>

#include <cstdio>
#include <cstring>
#include <cmath>
#include <windows.h>

namespace sprites
{
    auto game_directory() -> void
    {
        HMODULE client_module = GetModuleHandleA("client.dll");
        if (!client_module)
            return;

        char buffer[MAX_PATH];
        if (!GetModuleFileNameA(client_module, buffer, MAX_PATH))
            return;

        for (int level = 0; level < 2 && buffer[0]; level++)
        {
            char* separator = strrchr(buffer, '\\');
            if (!separator)
                return;

            *separator = 0;
        }

        sprites_directory = std::string(buffer) + "\\sprites";
        hud_path = sprites_directory + "\\hud.txt";
    }

    auto parse_hud() -> bool
    {
        FILE* file = nullptr;
        fopen_s(&file, hud_path.c_str(), "r");
        if (!file)
            return false;

        char line[256];
        while (fgets(line, sizeof(line), file))
        {
            char name[64];
            char scale[16];
            char sprite_token[32];
            int x, y, w, h;

            if (sscanf_s(line, " %63s %15s %31s %d %d %d %d", name, 64, scale, 16, sprite_token, 32, &x, &y, &w, &h) != 7)
                continue;

            if (w <= 0 || h <= 0)
                continue;

            if ((name[0] == 'd' && name[1] == '_') || strcmp(name, "c4") == 0)
                rects[name] = sprite_rect_t{ sprite_token, x, y, w, h };
        }

        fclose(file);
        return !rects.empty();
    }

    auto read_file(const char* path, std::vector<uint8_t>& output) -> bool
    {
        FILE* file = nullptr;
        fopen_s(&file, path, "rb");
        if (!file)
            return false;

        fseek(file, 0, SEEK_END);
        const long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        if (size <= 0)
        {
            fclose(file);
            return false;
        }

        output.resize(size);
        const size_t read = fread(output.data(), 1, size, file);
        fclose(file);
        return read == static_cast<size_t>(size);
    }

    auto cubic_value(float p0, float p1, float p2, float p3, float t) -> float
    {
        const float t2 = t * t;
        const float t3 = t2 * t;
        return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    }

    auto sample_texel(const std::vector<uint8_t>& source, int source_w, int source_h, int x, int y, int channel) -> float
    {
        if (x < 0)
            x = 0;

        if (y < 0)
            y = 0;

        if (x >= source_w)
            x = source_w - 1;

        if (y >= source_h)
            y = source_h - 1;

        return source[(y * source_w + x) * 4 + channel];
    }

    auto resize_scanline(const std::vector<uint8_t>& source, int source_w, int source_h, std::vector<uint8_t>& output, int output_w) -> void
    {
        for (int line = 0; line < source_h; line++)
        {
            for (int x = 0; x < output_w; x++)
            {
                const float source_x = (x + 0.5f) / output_w * source_w - 0.5f;
                const int x1 = static_cast<int>(floorf(source_x));
                const float fx = source_x - x1;

                for (int channel = 0; channel < 4; channel++)
                {
                    const float p0 = sample_texel(source, source_w, source_h, x1 - 1, line, channel);
                    const float p1 = sample_texel(source, source_w, source_h, x1, line, channel);
                    const float p2 = sample_texel(source, source_w, source_h, x1 + 1, line, channel);
                    const float p3 = sample_texel(source, source_w, source_h, x1 + 2, line, channel);
                    const float value = cubic_value(p0, p1, p2, p3, fx);
                    output[(line * output_w + x) * 4 + channel] = static_cast<uint8_t>(value < 0.0f ? 0.0f : (value > 255.0f ? 255.0f : value));
                }
            }
        }
    }

    auto scale_atlas(const std::vector<uint8_t>& source, int source_w, int source_h, std::vector<uint8_t>& output, int factor) -> void
    {
        const int output_w = source_w * factor;
        const int output_h = source_h * factor;

        std::vector<uint8_t> intermediate(static_cast<size_t>(output_w) * source_h * 4);
        resize_scanline(source, source_w, source_h, intermediate, output_w);

        output.resize(static_cast<size_t>(output_w) * output_h * 4);
        for (int y = 0; y < output_h; y++)
        {
            const float source_y = (y + 0.5f) / output_h * source_h - 0.5f;
            const int y1 = static_cast<int>(floorf(source_y));
            const float fy = source_y - y1;

            const int line0 = y1 - 1 < 0 ? 0 : y1 - 1;
            const int line1 = y1 < 0 ? 0 : y1;
            const int line2 = y1 + 1 > source_h - 1 ? source_h - 1 : y1 + 1;
            const int line3 = y1 + 2 > source_h - 1 ? source_h - 1 : y1 + 2;

            for (int x = 0; x < output_w; x++)
            {
                for (int channel = 0; channel < 4; channel++)
                {
                    const float p0 = intermediate[(line0 * output_w + x) * 4 + channel];
                    const float p1 = intermediate[(line1 * output_w + x) * 4 + channel];
                    const float p2 = intermediate[(line2 * output_w + x) * 4 + channel];
                    const float p3 = intermediate[(line3 * output_w + x) * 4 + channel];
                    const float value = cubic_value(p0, p1, p2, p3, fy);
                    output[(y * output_w + x) * 4 + channel] = static_cast<uint8_t>(value < 0.0f ? 0.0f : (value > 255.0f ? 255.0f : value));
                }
            }
        }
    }

    auto load_atlas(const std::string& sprite_name, atlas_entry_t& entry) -> bool
    {
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s\\%s.spr", sprites_directory.c_str(), sprite_name.c_str());

        std::vector<uint8_t> file;
        if (!read_file(path, file))
        {
            snprintf(path, sizeof(path), "%s\\640%s.spr", sprites_directory.c_str(), sprite_name.c_str());
            if (!read_file(path, file))
            {
                console::log("[sprites] cannot open sprite atlas for %s", sprite_name.c_str());
                return false;
            }
        }

        if (file.size() < sizeof(sprite_header_t))
        {
            console::log("[sprites] %s too small (%d bytes)", path, static_cast<int>(file.size()));
            return false;
        }

        const sprite_header_t* header = reinterpret_cast<const sprite_header_t*>(file.data());
        console::log("[sprites] spr header %s v=%d type=%d tex=%d %dx%d frames=%d file=%d", sprite_name.c_str(), header->version, header->type, header->texformat, header->width, header->height, header->numframes, static_cast<int>(file.size()));

        if (header->ident != 0x50534449 || header->type == 1 || header->numframes < 1)
        {
            console::log("[sprites] bad sprite header %s (ident=%08X type=%d frames=%d)", path, header->ident, header->type, header->numframes);
            return false;
        }

        const int width = header->width;
        const int height = header->height;
        if (width <= 0 || height <= 0)
        {
            console::log("[sprites] %s bad frame size %dx%d", path, width, height);
            return false;
        }

        const int pixel_count = width * height;
        std::vector<uint8_t> rgba(static_cast<size_t>(pixel_count) * 4);

        if (header->texformat == 2)
        {
            const std::vector<uint8_t> pixels(file.begin() + sizeof(sprite_header_t) + sizeof(sprite_frame_t), file.end());
            if (pixels.size() < static_cast<size_t>(pixel_count * 4))
            {
                console::log("[sprites] %s rgba underrun pixels=%d need=%d", path, static_cast<int>(pixels.size()), pixel_count * 4);
                return false;
            }

            for (int index = 0; index < pixel_count; index++)
            {
                rgba[index * 4 + 0] = pixels[index * 4 + 0];
                rgba[index * 4 + 1] = pixels[index * 4 + 1];
                rgba[index * 4 + 2] = pixels[index * 4 + 2];
                rgba[index * 4 + 3] = pixels[index * 4 + 3];
            }
        }
        else
        {
            const uint8_t* palette = file.data() + sizeof(sprite_header_t);
            const uint8_t* indices = palette + 768 + 22;
            if (file.size() < sizeof(sprite_header_t) + 768 + 22 + static_cast<size_t>(pixel_count))
            {
                console::log("[sprites] %s indexed underrun file=%d need=%d", path, static_cast<int>(file.size()), sizeof(sprite_header_t) + 768 + pixel_count);
                return false;
            }

            for (int index = 0; index < pixel_count; index++)
            {
                const uint8_t color_index = indices[index];
                if (color_index == 0 || color_index == 255)
                {
                    rgba[index * 4 + 3] = 0;
                    continue;
                }

                rgba[index * 4 + 0] = palette[color_index * 3 + 0];
                rgba[index * 4 + 1] = palette[color_index * 3 + 1];
                rgba[index * 4 + 2] = palette[color_index * 3 + 2];
                rgba[index * 4 + 3] = 255;
            }
        }

        unsigned int texture = 0;
        gen_textures(1, &texture);
        if (!texture)
            return false;

        std::vector<uint8_t> scaled_rgba;
        const int scale_factor = 4;
        scale_atlas(rgba, width, height, scaled_rgba, scale_factor);

        bind_texture(3553, texture);
        tex_parameteri(3553, 10240, 9729);
        tex_parameteri(3553, 10241, 9729);
        tex_parameteri(3553, 10242, 33071);
        tex_parameteri(3553, 10243, 33071);
        tex_image_2d(3553, 0, 6408, width * scale_factor, height * scale_factor, 0, 6408, 5121, scaled_rgba.data());

        entry.texture = texture;
        entry.width = width * scale_factor;
        entry.height = height * scale_factor;
        entry.scale = scale_factor;
        console::log("[sprites] atlas %s loaded %dx%d", sprite_name.c_str(), width * scale_factor, height * scale_factor);
        return true;
    }

    auto initialize() -> bool
    {
        HMODULE opengl = GetModuleHandleA("opengl32.dll");
        if (!opengl)
            return false;

        gen_textures = reinterpret_cast<gl_gen_textures_t>(GetProcAddress(opengl, "glGenTextures"));
        bind_texture = reinterpret_cast<gl_bind_texture_t>(GetProcAddress(opengl, "glBindTexture"));
        tex_image_2d = reinterpret_cast<gl_tex_image_2d_t>(GetProcAddress(opengl, "glTexImage2D"));
        tex_parameteri = reinterpret_cast<gl_tex_parameteri_t>(GetProcAddress(opengl, "glTexParameteri"));
        if (!gen_textures || !bind_texture || !tex_image_2d || !tex_parameteri)
            return false;

        game_directory();
        const bool hud_loaded = parse_hud();
        console::log("[sprites] hud.txt %s (%d rects)", hud_loaded ? "ok" : "missing", static_cast<int>(rects.size()));
        return !sprites_directory.empty() && hud_loaded;
    }

    auto find_rect(const std::string& name) -> const sprite_rect_t*
    {
        std::unordered_map<std::string, sprite_rect_t>::iterator found = rects.find(name);
        if (found == rects.end())
            return nullptr;

        return &found->second;
    }

    auto atlas_for(const std::string& sprite_name) -> const atlas_entry_t*
    {
        std::unordered_map<std::string, atlas_entry_t>::iterator cached = atlas_cache.find(sprite_name);
        if (cached != atlas_cache.end())
            return &cached->second;

        atlas_entry_t entry;
        if (!load_atlas(sprite_name, entry))
        {
            atlas_cache[sprite_name] = entry;
            return nullptr;
        }

        return &atlas_cache.emplace(sprite_name, entry).first->second;
    }

    auto draw(ImDrawList* draw_list, const char* weapon_name, const ImVec2& box_min, const ImVec2& box_max) -> bool
    {
        if (!weapon_name || !weapon_name[0] || !gen_textures)
            return false;

        char name_buffer[32];
        snprintf(name_buffer, sizeof(name_buffer), "%s", weapon_name);

        const size_t name_length = strlen(name_buffer);
        if (name_length >= 4 && strcmp(name_buffer + name_length - 4, ".mdl") == 0)
            name_buffer[name_length - 4] = '\0';

        char lookup[64];
        if (strcmp(name_buffer, "hegrenade") == 0)
            snprintf(lookup, sizeof(lookup), "d_grenade");
        else
            snprintf(lookup, sizeof(lookup), "d_%s", name_buffer);

        const sprite_rect_t* rect = find_rect(lookup);
        if (!rect)
            rect = find_rect(name_buffer);

        if (!rect)
        {
            if (missed_icons.insert(lookup).second)
                console::log("[sprites] no hud rect for %s (model token '%s')", lookup, weapon_name);

            return false;
        }

        const atlas_entry_t* atlas = atlas_for(rect->sprite);
        if (!atlas || !atlas->texture)
            return false;

        if (shown_icons.insert(lookup).second)
            console::log("[sprites] draw %s atlas %dx%d rect %d,%d %dx%d", lookup, atlas->width, atlas->height, rect->x, rect->y, rect->w, rect->h);

        const float icon_height = 24.0f;
        const float icon_width = rect->w * (icon_height / rect->h);

        const float x = floorf(box_min.x + (box_max.x - box_min.x - icon_width) * 0.5f);
        const float y = floorf(box_max.y + 2.0f);

        const ImVec2 uv0(static_cast<float>(rect->x * atlas->scale) / atlas->width, static_cast<float>(rect->y * atlas->scale) / atlas->height);
        const ImVec2 uv1(static_cast<float>((rect->x + rect->w) * atlas->scale) / atlas->width, static_cast<float>((rect->y + rect->h) * atlas->scale) / atlas->height);

        draw_list->AddImage(static_cast<ImTextureID>(static_cast<uintptr_t>(atlas->texture)), ImVec2(x, y), ImVec2(x + icon_width, y + icon_height), uv0, uv1);
        return true;
    }
}