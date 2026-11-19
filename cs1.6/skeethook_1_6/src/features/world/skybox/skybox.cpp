#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <crt/crt.h>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/address.hpp>
#include <memory/scanner.hpp>
#include <features/vars.hpp>

#include <features/world/skybox/skybox.hpp>

namespace skybox
{
    auto initialize() -> bool
    {
        static bool initialized = false;
        if (initialized)
            return true;

        uint8_t* draw_match = memory::find_pattern(modules::renderer, patterns::draw_skybox);
        uint8_t* flag_match = memory::find_pattern(modules::renderer, patterns::environment_flag);
        uint8_t* color_match = memory::find_pattern(modules::renderer, patterns::environment_color);
        uint8_t* load_match = memory::find_pattern(modules::renderer, patterns::load_skys);
        uint8_t* name_match = memory::find_pattern(modules::renderer, patterns::load_skys_name);
        if (!draw_match || !flag_match || !color_match || !load_match || !name_match)
            return false;

        enabled_flag = reinterpret_cast<int*>(memory::global_address(flag_match, 1));
        color = reinterpret_cast<float*>(memory::global_address(color_match, 10));
        reload_flag = reinterpret_cast<int*>(memory::global_address(load_match, 7));
        name_buffer = reinterpret_cast<char*>(memory::global_address(name_match, 2));
        load_address = load_match;

        draw_hook.create(draw_match, reinterpret_cast<void*>(on_draw_skybox));
        refresh();
        initialized = true;

        return true;
    }

    static auto face_bit(const char* face) -> int
    {
        if (CRT::StringCompareI(face, "rt") == 0)
            return 1;
        if (CRT::StringCompareI(face, "bk") == 0)
            return 2;
        if (CRT::StringCompareI(face, "lf") == 0)
            return 4;
        if (CRT::StringCompareI(face, "ft") == 0)
            return 8;
        if (CRT::StringCompareI(face, "up") == 0)
            return 16;
        if (CRT::StringCompareI(face, "dn") == 0)
            return 32;
        return 0;
    }

    static auto add_face(std::vector<sky_entry_t>& entries, const char* name, int bit) -> void
    {
        for (sky_entry_t& entry : entries)
        {
            if (CRT::StringCompareI(entry.name, name) == 0)
            {
                entry.faces |= bit;
                return;
            }
        }

        sky_entry_t entry = {};
        CRT::StringCopyN(entry.name, name, 31);
        entry.name[31] = '\0';
        entry.faces = bit;
        entries.push_back(entry);
    }

    static auto scan_directory(std::vector<sky_entry_t>& entries, const char* directory) -> void
    {
        char query[260] = {};
        const int written = snprintf(query, sizeof(query), "%s\\*.*", directory);
        if (written <= 0 || written >= static_cast<int>(sizeof(query)))
            return;

        WIN32_FIND_DATAA data = {};
        HANDLE find = FindFirstFileA(query, &data);
        if (find == INVALID_HANDLE_VALUE)
            return;

        do
        {
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            char* extension = CRT::StringCharR(data.cFileName, '.');
            if (!extension || extension == data.cFileName)
                continue;

            const size_t stem = static_cast<size_t>(extension - data.cFileName);
            if (stem <= 2)
                continue;

            char face[3] = { data.cFileName[stem - 2], data.cFileName[stem - 1], '\0' };
            const int bit = face_bit(face);
            if (!bit)
                continue;

            char name[32] = {};
            const size_t count = stem - 2;
            if (count == 0 || count >= sizeof(name))
                continue;

            CRT::StringCopyN(name, data.cFileName, count);
            name[count] = '\0';
            add_face(entries, name, bit);
        } while (FindNextFileA(find, &data));

        FindClose(find);
    }

    auto refresh() -> void
    {
        names.clear();

        char root[260] = {};
        HMODULE image = static_cast<HMODULE>(memory::get_module(nullptr));
        if (!image || !GetModuleFileNameA(image, root, sizeof(root)))
            return;

        char* slash = CRT::StringCharR(root, '\\');
        if (!slash)
            return;

        *slash = '\0';

        std::vector<sky_entry_t> entries;
        char directory[260] = {};
        const char* folders[] = { "valve\\gfx\\env", "cstrike\\gfx\\env", "cstrike_downloads\\gfx\\env" };
        for (const char* folder : folders)
        {
            const int written = snprintf(directory, sizeof(directory), "%s\\%s", root, folder);
            if (written <= 0 || written >= static_cast<int>(sizeof(directory)))
                continue;

            scan_directory(entries, directory);
        }

        for (const sky_entry_t& entry : entries)
        {
            if (entry.faces != 63 || !entry.name[0])
                continue;

            names.emplace_back(entry.name);
        }

        std::sort(names.begin(), names.end(), [](const std::string& left, const std::string& right)
        {
            return CRT::StringCompareI(left.c_str(), right.c_str()) < 0;
        });
    }

    auto selected() -> int
    {
        for (int index = 0; index < static_cast<int>(names.size()); index++)
        {
            if (CRT::StringCompareI(names[index].c_str(), vars::skybox.name) == 0)
                return index;
        }

        return -1;
    }

    auto select(int index) -> void
    {
        if (index < 0 || index >= static_cast<int>(names.size()))
            return;

        CRT::StringCopyN(vars::skybox.name, names[index].c_str(), 31);
        vars::skybox.name[31] = '\0';
    }

    auto combo_item(void* user, int index) -> const char*
    {
        if (index < 0 || index >= static_cast<int>(names.size()))
            return "";

        return names[index].c_str();
    }

    static auto apply_name(const char* name) -> void
    {
        CRT::StringCopyN(name_buffer, name, 31);
        name_buffer[31] = '\0';
        *reload_flag = 1;
        reinterpret_cast<int(*)()>(load_address)();
    }

    auto poll() -> void
    {
        if (!name_buffer || !reload_flag || !load_address)
            return;

        static char original[32] = {};
        static bool stored = false;
        static char applied[32] = {};
        static bool applied_changer = false;
        static char seen[32] = {};
        static bool seen_changer = false;
        static bool first = true;

        const bool changer = vars::skybox.changer;
        char current[32] = {};
        CRT::StringCopyN(current, vars::skybox.name, 31);
        current[31] = '\0';

        if (first)
        {
            first = false;
            applied_changer = changer;
            seen_changer = changer;
            CRT::StringCopyN(applied, current, 31);
            CRT::StringCopyN(seen, current, 31);
            return;
        }

        const bool steady = changer == seen_changer && CRT::StringCompare(current, seen) == 0;
        seen_changer = changer;
        CRT::StringCopyN(seen, current, 31);
        seen[31] = '\0';

        if (!steady)
            return;

        if (changer == applied_changer && CRT::StringCompare(current, applied) == 0)
            return;

        if (changer && current[0])
        {
            if (!stored)
            {
                CRT::StringCopyN(original, name_buffer, 31);
                original[31] = '\0';
                stored = true;
            }

            apply_name(current);
        }
        else if (stored)
        {
            apply_name(original);
            stored = false;
        }

        applied_changer = changer;
        CRT::StringCopyN(applied, current, 31);
        applied[31] = '\0';
    }

    auto __cdecl on_draw_skybox() -> int
    {
        auto original = draw_hook.get_original<decltype(&on_draw_skybox)>();
        if (!vars::skybox.enabled)
            return original();

        const int saved_flag = *enabled_flag;
        const float saved_color[3] = { color[0], color[1], color[2] };
        const float saved_scale = color[3];

        *enabled_flag = 1;
        color[0] = vars::skybox.color[0];
        color[1] = vars::skybox.color[1];
        color[2] = vars::skybox.color[2];
        color[3] = 1.0f;

        const int result = original();

        *enabled_flag = saved_flag;
        color[0] = saved_color[0];
        color[1] = saved_color[1];
        color[2] = saved_color[2];
        color[3] = saved_scale;

        return result;
    }
}
