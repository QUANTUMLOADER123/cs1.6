#include <imgui/imgui.h>
#include <core/input.hpp>
#include <render/render.hpp>
#include <render/menu.hpp>

static auto virtual_key_to_imgui(int virtual_key) -> ImGuiKey
{
    switch (virtual_key)
    {
    case VK_TAB: return ImGuiKey_Tab;
    case VK_LEFT: return ImGuiKey_LeftArrow;
    case VK_RIGHT: return ImGuiKey_RightArrow;
    case VK_UP: return ImGuiKey_UpArrow;
    case VK_DOWN: return ImGuiKey_DownArrow;
    case VK_PRIOR: return ImGuiKey_PageUp;
    case VK_NEXT: return ImGuiKey_PageDown;
    case VK_HOME: return ImGuiKey_Home;
    case VK_END: return ImGuiKey_End;
    case VK_INSERT: return ImGuiKey_Insert;
    case VK_DELETE: return ImGuiKey_Delete;
    case VK_BACK: return ImGuiKey_Backspace;
    case VK_SPACE: return ImGuiKey_Space;
    case VK_RETURN: return ImGuiKey_Enter;
    case VK_ESCAPE: return ImGuiKey_Escape;
    case VK_LCONTROL: return ImGuiKey_LeftCtrl;
    case VK_RCONTROL: return ImGuiKey_RightCtrl;
    case VK_CONTROL: return ImGuiKey_LeftCtrl;
    case VK_LSHIFT: return ImGuiKey_LeftShift;
    case VK_RSHIFT: return ImGuiKey_RightShift;
    case VK_SHIFT: return ImGuiKey_LeftShift;
    case VK_LMENU: return ImGuiKey_LeftAlt;
    case VK_RMENU: return ImGuiKey_RightAlt;
    case VK_MENU: return ImGuiKey_LeftAlt;
    default:
        break;
    }

    if (virtual_key >= '0' && virtual_key <= '9')
        return static_cast<ImGuiKey>(ImGuiKey_0 + (virtual_key - '0'));
    if (virtual_key >= 'A' && virtual_key <= 'Z')
        return static_cast<ImGuiKey>(ImGuiKey_A + (virtual_key - 'A'));

    return ImGuiKey_None;
}

static auto paste_clipboard(ImGuiIO& io) -> void
{
    if (!OpenClipboard(nullptr))
        return;

    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    if (data)
    {
        const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(data));
        if (text)
        {
            char utf8[1024] = {};
            WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8, sizeof(utf8) - 1, nullptr, nullptr);
            io.AddInputCharactersUTF8(utf8);
            GlobalUnlock(data);
        }
    }
    else
    {
        data = GetClipboardData(CF_TEXT);
        if (data)
        {
            const char* text = static_cast<const char*>(GlobalLock(data));
            if (text)
            {
                io.AddInputCharactersUTF8(text);
                GlobalUnlock(data);
            }
        }
    }

    CloseClipboard();
}

void input::update()
{
    ImGuiIO& io = ImGui::GetIO();

    POINT cursor;
    GetCursorPos(&cursor);
    ScreenToClient(render::window, &cursor);

    io.AddMousePosEvent(static_cast<float>(cursor.x), static_cast<float>(cursor.y));
    io.AddMouseButtonEvent(0, (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
    io.AddMouseButtonEvent(1, (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
    io.AddMouseButtonEvent(2, (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0);

    static bool previous[256] = {};
    BYTE keyboard[256] = {};
    GetKeyboardState(keyboard);

    const bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    io.AddKeyEvent(ImGuiMod_Ctrl, control);
    io.AddKeyEvent(ImGuiMod_Shift, (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (GetAsyncKeyState(VK_MENU) & 0x8000) != 0);

    for (int virtual_key = 1; virtual_key < 256; virtual_key++)
    {
        if (virtual_key == VK_LBUTTON || virtual_key == VK_RBUTTON || virtual_key == VK_MBUTTON)
            continue;

        const bool down = (GetAsyncKeyState(virtual_key) & 0x8000) != 0;
        if (down == previous[virtual_key])
            continue;

        previous[virtual_key] = down;

        const ImGuiKey key = virtual_key_to_imgui(virtual_key);
        if (key != ImGuiKey_None)
            io.AddKeyEvent(key, down);

        if (!down || !menu::opened)
            continue;

        if (control && virtual_key == 'V')
        {
            paste_clipboard(io);
            continue;
        }

        if (control)
            continue;

        wchar_t buffer[4] = {};
        const int written = ToUnicode(static_cast<UINT>(virtual_key), MapVirtualKeyA(static_cast<UINT>(virtual_key), MAPVK_VK_TO_VSC), keyboard, buffer, 4, 0);
        for (int index = 0; index < written; index++)
        {
            if (buffer[index] >= 32)
                io.AddInputCharacter(static_cast<unsigned int>(buffer[index]));
        }
    }
}
