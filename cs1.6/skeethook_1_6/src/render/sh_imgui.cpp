/* ИНТЕГРАЦИЯ DEAR IMGUI.

   Раньше меню было самописным: свои чекбоксы, слайдеры, скроллбар,
   и почти каждый элемент приходилось чинить отдельно (клики по
   уровню вместо фронта, курсор, отсечение, шрифт). В референсе,
   который присылал пользователь, всё это уже решено Dear ImGui —
   берём его вместо самописа.

   Три вещи, которые надо было понять по референсу:

   1. Ввод. ImGui не читает мышь сам. Позиция и кнопки передаются
      вручную через AddMousePosEvent / AddMouseButtonEvent на КАЖДЫЙ
      кадр, клавиши — через AddKeyEvent. Без этого ImGui рисует,
      но не реагирует на нажатия (ровно то, на что жаловались:
      "перестало реагировать на нажатия").

   2. Курсор. ImGui НЕ управляет курсором Windows. Его нужно
      освободить самому: ClipCursor(0) + ReleaseCapture +
      ShowCursor(TRUE). Флаг NoMouseCursorChange как раз говорит
      ImGui "не трогай мой курсор, я сам".

   3. Перетаскивание окна делает сам ImGui (заголовок окна),
      если НЕ ставить ImGuiWindowFlags_NoMove.
*/

#include "../../include/sh_imgui.h"
#include "../../include/sh_render.h"

#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/backend/imgui_impl_opengl.h"

#include <windows.h>

static qbool g_init;
static qbool g_frame_open;
static HWND g_window;
static f32 g_font_scale = -1.0f;
static f32 g_draw_w = 640.0f;
static f32 g_draw_h = 480.0f;

/* Tahoma есть на любой Windows с 2000; остальные - фолбэк */
static const char *g_font_paths[3] = {
    "C:\\Windows\\Fonts\\tahoma.ttf",
    "C:\\Windows\\Fonts\\segoeui.ttf",
    "C:\\Windows\\Fonts\\verdana.ttf"
};

static i32 g_prev_keys[256];
static i32 g_prev_mouse[3];

static void *sh_gl_proc2(const char *name)
{
    static HMODULE gl;

    if (!gl)
        gl = LoadLibraryA("opengl32.dll");

    return gl ? (void *)GetProcAddress(gl, name) : 0;
}

/* сопоставление VK -> ImGuiKey, как в референсе (core/input.cpp) */
static ImGuiKey sh_vk_to_imgui(i32 vk)
{
    switch (vk)
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
    case VK_LCONTROL: case VK_CONTROL: return ImGuiKey_LeftCtrl;
    case VK_RCONTROL: return ImGuiKey_RightCtrl;
    case VK_LSHIFT: case VK_SHIFT: return ImGuiKey_LeftShift;
    case VK_RSHIFT: return ImGuiKey_RightShift;
    case VK_LMENU: case VK_MENU: return ImGuiKey_LeftAlt;
    case VK_RMENU: return ImGuiKey_RightAlt;
    default: break;
    }

    if (vk >= '0' && vk <= '9')
        return (ImGuiKey)(ImGuiKey_0 + (vk - '0'));

    if (vk >= 'A' && vk <= 'Z')
        return (ImGuiKey)(ImGuiKey_A + (vk - 'A'));

    return ImGuiKey_None;
}

static void sh_imgui_style(void)
{
    ImGuiStyle *st = &ImGui::GetStyle();
    ImVec4 *c = st->Colors;

    st->WindowRounding = 6.0f;
    st->ChildRounding = 4.0f;
    st->FrameRounding = 3.0f;
    st->PopupRounding = 4.0f;
    st->ScrollbarRounding = 4.0f;
    st->GrabRounding = 3.0f;
    st->TabRounding = 3.0f;
    st->WindowBorderSize = 1.0f;
    st->FrameBorderSize = 0.0f;
    st->WindowPadding = ImVec2(10.0f, 10.0f);
    st->FramePadding = ImVec2(7.0f, 4.0f);
    st->ItemSpacing = ImVec2(7.0f, 4.0f);
    st->ScrollbarSize = 9.0f;
    st->GrabMinSize = 9.0f;

    c[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.94f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.47f, 0.47f, 0.50f, 1.00f);
    c[ImGuiCol_WindowBg] = ImVec4(0.043f, 0.043f, 0.055f, 0.98f);
    c[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_PopupBg] = ImVec4(0.055f, 0.055f, 0.070f, 0.99f);
    c[ImGuiCol_Border] = ImVec4(0.16f, 0.16f, 0.19f, 1.00f);
    c[ImGuiCol_FrameBg] = ImVec4(0.082f, 0.082f, 0.098f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.118f, 0.118f, 0.137f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.145f, 0.145f, 0.169f, 1.00f);
    c[ImGuiCol_TitleBg] = ImVec4(0.055f, 0.055f, 0.070f, 1.00f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.070f, 0.070f, 0.090f, 1.00f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.745f, 0.922f, 0.235f, 1.00f);
    c[ImGuiCol_CheckMark] = ImVec4(0.745f, 0.922f, 0.235f, 1.00f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.745f, 0.922f, 0.235f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.800f, 0.960f, 0.300f, 1.00f);
    c[ImGuiCol_Button] = ImVec4(0.082f, 0.082f, 0.098f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.130f, 0.130f, 0.155f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.745f, 0.922f, 0.235f, 0.45f);
    c[ImGuiCol_Header] = ImVec4(0.745f, 0.922f, 0.235f, 0.32f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.745f, 0.922f, 0.235f, 0.48f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.745f, 0.922f, 0.235f, 0.65f);
    c[ImGuiCol_Separator] = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.745f, 0.922f, 0.235f, 0.35f);
}

/* Пересобрать атлас под масштаб.

   Почему пересборка, а не FontGlobalScale: глобальный масштаб просто
   растягивает уже отрисованные глифы, и на 1080p текст мылится.
   Пересборка даёт чёткие буквы нужного кегля.

   Стоит дорого (растеризация сотен глифов), а вызывается из
   HUD_Redraw, поэтому пересобираем ТОЛЬКО при смене масштаба -
   сравнение с шагом 0.1. */
void sh_imgui_scale_font(f32 scale)
{
    ImGuiIO *io;
    /* ImFontConfig по умолчанию сконструирован корректно, но если
       в нём окажется мусор (OversampleH/V=0), атлас выйдет пустым.
       Выставляем явно то, что влияет на растеризацию. */
    ImFontConfig cfg;

    cfg.OversampleH = 1;
    cfg.OversampleV = 1;
    cfg.PixelSnapH = 1;
    cfg.RasterizerDensity = 1.0f;

    qbool loaded = 0;
    i32 px;

    if (!g_init)
        return;

    if (scale < 0.5f)
        scale = 0.5f;

    if (scale > 2.5f)
        scale = 2.5f;

    {
        f32 q = (f32)(i32)(scale * 10.0f + 0.5f) / 10.0f;

        if (q == g_font_scale)
            return;

        scale = q;
    }

    g_font_scale = scale;

    px = (i32)(14.0f * scale + 0.5f);

    if (px < 10)
        px = 10;

    io = &ImGui::GetIO();

    io->Fonts->Clear();
    io->FontGlobalScale = 1.0f;

    for (i32 i = 0; i < 3 && !loaded; ++i)
    {
        if (GetFileAttributesA(g_font_paths[i]) == INVALID_FILE_ATTRIBUTES)
            continue;

        if (io->Fonts->AddFontFromFileTTF(g_font_paths[i], (f32)px, &cfg,
                                          io->Fonts->GetGlyphRangesCyrillic()))
            loaded = 1;
    }

    if (!loaded)
        io->Fonts->AddFontDefault();

    io->Fonts->Build();

    ImGui_ImplOpenGL_RefreshFontTexture();
}

qbool sh_imgui_init(void *game_window)
{
    ImGuiIO *io;
    /* ImFontConfig по умолчанию сконструирован корректно, но если
       в нём окажется мусор (OversampleH/V=0), атлас выйдет пустым.
       Выставляем явно то, что влияет на растеризацию. */
    ImFontConfig cfg;

    cfg.OversampleH = 1;
    cfg.OversampleV = 1;
    cfg.PixelSnapH = 1;
    cfg.RasterizerDensity = 1.0f;

    qbool loaded = 0;

    if (g_init)
        return 1;

    g_window = (HWND)game_window;

    ImGui::CreateContext();
    sh_imgui_style();

    io = &ImGui::GetIO();
    /* "не трогай системный курсор" - им управляем мы сами */
    io->ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io->IniFilename = 0;   /* не пишем imgui.ini рядом с игрой */
    io->LogFilename = 0;

    /* ВАЖНО: шрифт добавляется ДО Init(). Бэкенд в Init() заливает
       атлас в текстуру; если добавить шрифт после, атлас уйдёт
       пустым и весь текст будет невидим. */
    for (i32 i = 0; i < 3 && !loaded; ++i)
    {
        if (GetFileAttributesA(g_font_paths[i]) == INVALID_FILE_ATTRIBUTES)
            continue;

        if (io->Fonts->AddFontFromFileTTF(g_font_paths[i], 16.0f, &cfg,
                                          io->Fonts->GetGlyphRangesCyrillic()))
            loaded = 1;
    }

    if (!loaded)
        io->Fonts->AddFontDefault();

    io->Fonts->Build();

    if (!ImGui_ImplOpenGL_Init())
    {
        ImGui::DestroyContext();
        return 0;
    }

    g_init = 1;
    return 1;
}

void sh_imgui_shutdown(void)
{
    if (!g_init)
        return;

    ImGui_ImplOpenGL_Shutdown();
    ImGui::DestroyContext();
    g_init = 0;
}

void sh_imgui_begin(i32 width, i32 height, qbool menu_visible)
{
    ImGuiIO *io;

    if (!g_init)
        return;

    io = &ImGui::GetIO();

    io->DisplaySize = ImVec2((f32)width, (f32)height);
    g_draw_w = (f32)width;
    g_draw_h = (f32)height;
    io->DeltaTime = 1.0f / 60.0f;

    /* --- ВВОД ---
       Без этого блока ImGui рисует, но не реагирует на клики:
       он не читает мышь сам, всё передаётся руками. */
    {
        POINT p;

        p.x = 0;
        p.y = 0;

        GetCursorPos(&p);

        if (g_window)
            ScreenToClient(g_window, &p);

        /* КООРДИНАТЫ КУРСОРА.

           GetCursorPos/ScreenToClient дают пиксели КЛИЕНТСКОЙ ОБЛАСТИ
           окна, а ImGui рисует в координатах io->DisplaySize, то есть
           в координатах движка (1280x720, 1920x1080 и т.д.). Если
           клиентская область отличается от них - а в оконном режиме
           отличается всегда - клики попадают не туда: "курсор сломан".

           Пересчитываем в пространство рендера. */
        {
            RECT rc;

            if (g_window && GetClientRect(g_window, &rc))
            {
                f32 cw = (f32)(rc.right - rc.left);
                f32 ch = (f32)(rc.bottom - rc.top);

                /* соотношение разумное - пересчитываем, иначе
                   оставляем как есть: битое GetClientRect не должно
                   утащить курсор в бесконечность */
                if (cw > 0.0f && ch > 0.0f &&
                    cw < 20000.0f && ch < 20000.0f)
                {
                    io->AddMousePosEvent((f32)p.x * g_draw_w / cw,
                                         (f32)p.y * g_draw_h / ch);
                }
                else
                {
                    io->AddMousePosEvent((f32)p.x, (f32)p.y);
                }
            }
            else
            {
                io->AddMousePosEvent((f32)p.x, (f32)p.y);
            }
        }

        {
            i32 mb[3];

            mb[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0;
            mb[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 1 : 0;
            mb[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) ? 1 : 0;

            for (i32 i = 0; i < 3; ++i)
            {
                if (mb[i] != g_prev_mouse[i])
                {
                    io->AddMouseButtonEvent(i, mb[i] != 0);
                    g_prev_mouse[i] = mb[i];
                }
            }
        }

        /* клавиши: только пока меню открыто, иначе чит съест ввод
           у самой игры (например W/A/S/D) */
        if (menu_visible)
        {
            for (i32 vk = 1; vk < 256; ++vk)
            {
                i32 down;

                if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON)
                    continue;

                down = (GetAsyncKeyState(vk) & 0x8000) ? 1 : 0;

                if (down == g_prev_keys[vk])
                    continue;

                g_prev_keys[vk] = down;

                {
                    ImGuiKey k = sh_vk_to_imgui(vk);

                    if (k != ImGuiKey_None)
                        io->AddKeyEvent(k, down != 0);
                }
            }
        }
    }

    ImGui_ImplOpenGL_NewFrame();

    ImGui::NewFrame();

    g_frame_open = 1;
}

void sh_imgui_end(void)
{
    if (!g_frame_open)
        return;

    g_frame_open = 0;

    ImGui::Render();
    ImGui_ImplOpenGL_RenderDrawData(ImGui::GetDrawData());
}

qbool sh_imgui_ready(void)
{
    return g_init;
}

void *sh_imgui_font(void)
{
    if (!g_init)
        return 0;

    return (void *)ImGui::GetIO().Fonts->Fonts[0];
}
