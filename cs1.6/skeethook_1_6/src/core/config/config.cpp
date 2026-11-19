#include <cstdio>

#include <cstddef>
#include <string>
#include <vector>
#include <crt/crt.h>
#include <features/vars.hpp>

#include <core/config/config.hpp>

#include <windows.h>
#include <shellapi.h>

#define config_version 15

namespace config
{
    static const char* root = "C:\\cs";
    static std::vector<std::string> items;
    static int current = -1;

    static auto valid_name(const char* name) -> bool
    {
        if (!name || !name[0])
            return false;

        for (const char* cursor = name; *cursor; cursor++)
        {
            const char symbol = *cursor;
            if (symbol == '\\' || symbol == '/' || symbol == ':' || symbol == '*' || symbol == '?' || symbol == '"' || symbol == '<' || symbol == '>' || symbol == '|')
                return false;
        }

        return true;
    }

    static auto strip_extension(char* name) -> void
    {
        const size_t length = CRT::StringLength(name);
        if (length < 4)
            return;

        char* extension = name + (length - 4);
        if (CRT::StringCompareI(extension, ".cfg") == 0)
            *extension = '\0';
    }

    static auto make_path(const char* name, char* out, size_t size) -> bool
    {
        if (!valid_name(name))
            return false;

        char cleaned[64] = {};
        CRT::StringCopyN(cleaned, name, sizeof(cleaned) - 1);
        cleaned[sizeof(cleaned) - 1] = '\0';
        strip_extension(cleaned);
        if (!cleaned[0])
            return false;

        const int written = snprintf(out, size, "%s\\%s.cfg", root, cleaned);
        return written > 0 && written < static_cast<int>(size);
    }

    auto directory() -> const char*
    {
        return root;
    }

    auto names() -> const std::vector<std::string>&
    {
        return items;
    }

    auto selected() -> int&
    {
        return current;
    }

    auto refresh() -> void
    {
        items.clear();
        CreateDirectoryA(root, nullptr);

        WIN32_FIND_DATAA data = {};
        HANDLE find = FindFirstFileA("C:\\cs\\*.cfg", &data);
        if (find == INVALID_HANDLE_VALUE)
        {
            current = -1;
            return;
        }

        do
        {
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            strip_extension(data.cFileName);
            if (!data.cFileName[0])
                continue;

            bool already = false;
            for (const std::string& item : items)
            {
                if (CRT::StringCompareI(item.c_str(), data.cFileName) == 0)
                {
                    already = true;
                    break;
                }
            }

            if (!already)
                items.emplace_back(data.cFileName);
        } while (FindNextFileA(find, &data));

        FindClose(find);

        if (current >= static_cast<int>(items.size()))
            current = items.empty() ? -1 : 0;
    }

    auto exists(const char* name) -> bool
    {
        if (!valid_name(name))
            return false;

        for (const std::string& item : items)
        {
            if (CRT::StringCompare(item.c_str(), name) == 0)
                return true;
        }

        char path[260] = {};
        if (!make_path(name, path, sizeof(path)))
            return false;

        const DWORD attributes = GetFileAttributesA(path);
        return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    auto save(const char* name) -> bool
    {
        char path[260] = {};
        if (!make_path(name, path, sizeof(path)))
            return false;

        CreateDirectoryA(root, nullptr);

        blob_t blob = {};
        blob.magic = 0x474F4C44;
        blob.version = config_version;
        blob.esp = vars::esp;
        blob.chams = vars::chams;
        blob.misc = vars::misc;
        blob.third_person = vars::third_person;
        blob.nightmode = vars::nightmode;
        blob.skybox = vars::skybox;
        blob.aspect_ratio = vars::aspect_ratio;
        blob.viewmodel = vars::viewmodel;
        blob.fov_changer = vars::fov_changer;
        blob.antihit = vars::antihit;
        blob.removals = vars::removals;
        blob.model_changer = vars::model_changer;
        blob.bullet_tracers = vars::bullet_tracers;
        blob.bullet_impacts = vars::bullet_impacts;
        blob.ragebot = vars::ragebot;
        blob.legitbot = vars::legitbot;

        HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return false;

        DWORD written = 0;
        const BOOL ok = WriteFile(file, &blob, sizeof(blob), &written, nullptr);
        CloseHandle(file);

        refresh();
        for (int index = 0; index < static_cast<int>(items.size()); index++)
        {
            if (CRT::StringCompare(items[index].c_str(), name) == 0)
            {
                current = index;
                break;
            }
        }

        return ok && written == sizeof(blob);
    }

    auto load(const char* name) -> bool
    {
        char path[260] = {};
        if (!make_path(name, path, sizeof(path)))
            return false;

        HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return false;

        blob_t blob = {};
        DWORD read = 0;
        const BOOL ok = ReadFile(file, &blob, sizeof(blob), &read, nullptr);
        CloseHandle(file);

        if (!ok || read != sizeof(blob) || blob.magic != 0x474F4C44 || blob.version != config_version)
            return false;

        vars::esp = blob.esp;
        vars::chams = blob.chams;
        vars::misc = blob.misc;
        vars::third_person = blob.third_person;
        vars::nightmode = blob.nightmode;
        vars::skybox = blob.skybox;
        vars::aspect_ratio = blob.aspect_ratio;
        vars::viewmodel = blob.viewmodel;
        vars::fov_changer = blob.fov_changer;
        vars::antihit = blob.antihit;
        vars::removals = blob.removals;
        vars::model_changer = blob.model_changer;
        vars::bullet_tracers = blob.bullet_tracers;
        vars::bullet_impacts = blob.bullet_impacts;
        vars::ragebot = blob.ragebot;
        vars::legitbot = blob.legitbot;
        return true;
    }

    auto remove(const char* name) -> bool
    {
        char path[260] = {};
        if (!make_path(name, path, sizeof(path)))
            return false;

        const bool ok = DeleteFileA(path) != 0;
        refresh();
        return ok;
    }

    auto open_directory() -> void
    {
        CreateDirectoryA(root, nullptr);
        ShellExecuteA(nullptr, "open", root, nullptr, nullptr, SW_SHOWNORMAL);
    }
}
