// dllmain.cpp — Windows-тест-плагин меню поверх приложения на OpenGL через kiero.
//
// Что делает:
//   * в отдельном потоке ждёт готовности opengl32 и хукает wglSwapBuffers
//     (метод-таблица kiero, индекс 335)
//   * на каждый SwapBuffers: инициализирует ImGui (OpenGL2 backend), рисует меню
//   * INSERT — показать/скрыть меню
//   * F12   — сохранить скриншот кадра (меню + игра) в menu_shot_<N>.png
//
// Сборка: см. build_win.sh — 32-бит под CS 1.6 (x86-windows-gnu через zig).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstring>
#include <string>

#include "imgui.h"
#include "imgui_impl_opengl2.h"
#include "imgui_impl_win32.h"
#include "MinHook.h"
#include "gui_theme.h"
#include "gui_widgets.h"
#include "gui_menu.h"

// --- оригинальный спрайт иконок gamesense + Tahoma Bold из памяти ---
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "gui_icons_init.h"
#include "gui_icons_gl.h"
#include "gui_font_data.h"

// --- stb_image_write для скриншота ---
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef BOOL(WINAPI* twglSwapBuffers)(HDC);
static BOOL WINAPI Hook_wglSwapBuffers(HDC hdc);
static twglSwapBuffers o_wglSwapBuffers = nullptr;
static bool g_imgui_ready = false;
static HWND g_hwnd = nullptr;
static int g_shot_index = 0;

// Отдельный thread нужен, чтобы не грузить opengl32 в DllMain (loader lock).
static DWORD WINAPI HookThread(LPVOID)
{
    // ждём, пока приложение создаст GL-контекст
    for (int i = 0; i < 600; ++i)  // ~30 секунд
    {
        if (GetModuleHandleA("opengl32.dll") != nullptr)
            break;
        Sleep(50);
    }

    // opengl32 подгрузился, но wglSwapBuffers в 1.6 резолвится через opengl32.dll
    // (в NT6+ он реэкспортируется туда из gdi32). Берём оттуда, MinHook дергает напрямую.
    HMODULE h_gl = GetModuleHandleA("opengl32.dll");
    if (!h_gl) return 0;

    o_wglSwapBuffers = (twglSwapBuffers)GetProcAddress(h_gl, "wglSwapBuffers");
    if (!o_wglSwapBuffers)
    {
        HMODULE h_gdi = GetModuleHandleA("gdi32.dll");
        if (h_gdi) o_wglSwapBuffers = (twglSwapBuffers)GetProcAddress(h_gdi, "wglSwapBuffers");
    }
    if (!o_wglSwapBuffers) return 0;

    // MinHook: ставим detour на wglSwapBuffers
    if (MH_Initialize() != MH_OK) return 0;
    if (MH_CreateHook(reinterpret_cast<LPVOID>(o_wglSwapBuffers),
                      reinterpret_cast<LPVOID>(&Hook_wglSwapBuffers),
                      reinterpret_cast<LPVOID*>(&o_wglSwapBuffers)) != MH_OK)
        return 0;
    MH_EnableHook(reinterpret_cast<LPVOID>(o_wglSwapBuffers));
    return 0;
}

// --- сам хук ---
static bool init_imgui(HWND hwnd);

static BOOL WINAPI Hook_wglSwapBuffers(HDC hdc)
{
    if (!g_imgui_ready)
    {
        // окно берём по контексту устройства
        g_hwnd = WindowFromDC(hdc);
        if (!g_hwnd) g_hwnd = GetForegroundWindow();
        if (!init_imgui(g_hwnd))
            return o_wglSwapBuffers(hdc);
        g_imgui_ready = true;
    }

    // --- toggle menu ---
    static bool prev_insert = false;
    const bool insert = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
    if (insert && !prev_insert)
        gui::State().menu_open = !gui::State().menu_open;
    prev_insert = insert;

    // --- скриншот ---
    static bool prev_f12 = false;
    const bool f12 = (GetAsyncKeyState(VK_F12) & 0x8000) != 0;
    const bool want_shot = f12 && !prev_f12;
    prev_f12 = f12;

    ImGuiIO& io = ImGui::GetIO();
    RECT rc;
    if (g_hwnd && GetClientRect(g_hwnd, &rc))
        io.DisplaySize = ImVec2((float)(rc.right - rc.left), (float)(rc.bottom - rc.top));

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    gui::Render(gui::State());

    ImGui::Render();

    // Снимаем кадр ДО отрисовки оверлея — получаем чистую игру.
    // Так на скриншоте сохраняется именно то, что видит игрок + меню поверх.
    if (want_shot)
    {
        const int w = (int)io.DisplaySize.x, h = (int)io.DisplaySize.y;
        if (w > 0 && h > 0)
        {
            unsigned char* pixels = (unsigned char*)malloc((size_t)w * h * 3);
            if (pixels)
            {
                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

                // GL отдаёт снизу вверх — переворачиваем
                unsigned char* flipped = (unsigned char*)malloc((size_t)w * h * 3);
                if (flipped)
                {
                    for (int y = 0; y < h; ++y)
                        memcpy(flipped + (size_t)y * w * 3,
                               pixels + (size_t)(h - 1 - y) * w * 3,
                               (size_t)w * 3);

                    char name[128];
                    snprintf(name, sizeof(name), "menu_shot_%d.png", g_shot_index++);
                    stbi_write_png(name, w, h, 3, flipped, w * 3);
                    free(flipped);
                }
                free(pixels);
            }
        }
    }

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    return o_wglSwapBuffers(hdc);
}

static bool init_imgui(HWND hwnd)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    if (!ImGui_ImplWin32_Init(hwnd))
        return false;
    if (!ImGui_ImplOpenGL2_Init())
        return false;

    // Шрифт: НАСТОЯЩИЙ Tahoma Bold из встроенного массива (тот же, что в gamesense).
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    cfg.OversampleH = 1;
    cfg.OversampleV = 1;
    cfg.PixelSnapH = true;
    cfg.SizePixels = gui::geo::TEXT_SIZE;
    if (!io.Fonts->AddFontFromMemoryTTF((void*)g_tahoma_bold, (int)g_tahoma_bold_size,
                                        cfg.SizePixels, &cfg,
                                        io.Fonts->GetGlyphRangesCyrillic()))
        return false;

    // ОРИГИНАЛЬНЫЙ спрайт иконок gamesense: декодируем PNG и грузим в GL
    gui::InitIconAtlasCPU();
    gui::InitIconAtlasGL();

    gui::geo::SetScale(1.0f);
    gui::Apply();
    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        HANDLE t = CreateThread(nullptr, 0, HookThread, nullptr, 0, nullptr);
        if (t) CloseHandle(t);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        if (g_imgui_ready)
        {
            ImGui_ImplOpenGL2_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
        }
    }
    return TRUE;
}
