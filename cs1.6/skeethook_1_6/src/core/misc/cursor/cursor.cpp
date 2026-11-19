#include <cstrike1_5/modules.hpp>
#include <memory/scanner.hpp>
#include <render/render.hpp>
#include <render/menu.hpp>

#include <core/misc/cursor/cursor.hpp>

namespace cursor
{
    using show_cursor_t = int(__cdecl*)(int toggle);
    using set_cursor_t = void(__cdecl*)(void* handle);
    using create_cursor_t = void*(__cdecl*)(int kind);
    using get_cursor_t = void*(__cdecl*)();
    using set_relative_t = int(__cdecl*)(int mode);
    using get_relative_t = int(__cdecl*)();

    static show_cursor_t show_cursor = nullptr;
    static set_cursor_t set_cursor = nullptr;
    static create_cursor_t create_cursor = nullptr;
    static get_cursor_t get_cursor = nullptr;
    static set_relative_t set_relative = nullptr;
    static get_relative_t get_relative = nullptr;
    static void* arrow_cursor = nullptr;
    static bool resolved = false;
    static bool available = false;

    static auto resolve() -> void
    {
        resolved = true;

        void* image = memory::get_module(modules::sdl2);
        if (!image)
            return;

        HMODULE module = static_cast<HMODULE>(image);

        show_cursor = reinterpret_cast<show_cursor_t>(GetProcAddress(module, "SDL_ShowCursor"));
        set_cursor = reinterpret_cast<set_cursor_t>(GetProcAddress(module, "SDL_SetCursor"));
        create_cursor = reinterpret_cast<create_cursor_t>(GetProcAddress(module, "SDL_CreateSystemCursor"));
        get_cursor = reinterpret_cast<get_cursor_t>(GetProcAddress(module, "SDL_GetCursor"));
        set_relative = reinterpret_cast<set_relative_t>(GetProcAddress(module, "SDL_SetRelativeMouseMode"));
        get_relative = reinterpret_cast<get_relative_t>(GetProcAddress(module, "SDL_GetRelativeMouseMode"));

        available = show_cursor && set_cursor && create_cursor && get_cursor && set_relative && get_relative;
        if (available)
            arrow_cursor = create_cursor(0);
    }

    auto poll() -> void
    {
        if (!resolved)
            resolve();

        if (!available)
            return;

        static bool opened = false;
        static int saved_relative = 0;
        static int saved_show = 0;
        static void* saved_cursor = nullptr;

        if (menu::opened)
        {
            if (!opened)
            {
                saved_relative = get_relative();
                saved_show = show_cursor(-1);
                saved_cursor = get_cursor();
                opened = true;
            }

            set_relative(0);
            show_cursor(1);
            if (arrow_cursor)
                set_cursor(arrow_cursor);
        }
        else if (opened)
        {
            if (render::window)
            {
                RECT client = {};
                GetClientRect(render::window, &client);

                POINT center = { (client.left + client.right) / 2, (client.top + client.bottom) / 2 };
                ClientToScreen(render::window, &center);
                SetCursorPos(center.x, center.y);
            }

            set_cursor(saved_cursor);
            show_cursor(saved_show);
            set_relative(saved_relative);
            opened = false;
        }
    }
}