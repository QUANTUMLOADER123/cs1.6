// main.cpp — headless-рендер меню в PNG.
//
// Что делает:
//   1. поднимает ImGui-контекст без окна и без GPU
//   2. грузит TTF, собирает атлас глифов силами самого ImGui (FreeType-стаб внутри)
//   3. рисует меню (render::atlas получает alpha-пиксели прямо из ImGui-атласа)
//   4. программно растеризует ImDrawData на CPU: scissor, alpha-blend, глифы
//   5. пишет PNG через stb_image_write
//
// Запуск: ./headless_menu <out.png> [scale] [tab]
//   scale — множитель разрешения (1 = 820x560, 2 = 1640x1120)
//   tab   — какая вкладка открыта (0 rage, 1 visuals, 2 misc, 3 settings, 4 skins)

#include "imgui.h"
#include "imgui_internal.h"
#include "../include/gui_theme.h"
#include "../include/gui_widgets.h"
#include "../include/gui_menu.h"
#include "../include/gui_render.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// ---- CPU-атлас: ImGui отдаёт нам свою alpha8-текстуру, мы её используем напрямую ----
namespace render
{
    Atlas g_atlas;
}

// Импорт атласа из ImGui. Глифы, белый пиксель и UV — всё уже согласовано,
// потому что ImGui-шрифт и наши символы ссылаются на одну и ту же текстуру.
static void ImportAtlasFromImGui()
{
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels = nullptr;
    int w = 0, h = 0;
    int bpp = 0;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &w, &h, &bpp);
    if (!pixels) { fprintf(stderr, "[!] атлас пуст\n"); return; }

    render::g_atlas.w = w;
    render::g_atlas.h = h;
    render::g_atlas.pixels.assign(pixels, pixels + (size_t)w * h);
    render::g_atlas.valid = true;
    printf("[*] атлас ImGui: %dx%d, alpha8\n", w, h);
}

// Имитация кадра из CS 1.6: пустынная карта (de_dust2-подобная палитра),
// чтобы на скриншоте было видно, что меню — оверлей поверх игры.
static void draw_game_background(render::Image& img)
{
    const int W = img.w, H = img.h;
    auto put = [&](int x, int y, uint32_t c) {
        if (x >= 0 && x < W && y >= 0 && y < H) img.px[(size_t)y * W + x] = c;
    };
    auto rect = [&](int x0, int y0, int x1, int y1, uint32_t c) {
        for (int y = y0; y < y1; ++y) for (int x = x0; x < x1; ++x) put(x, y, c);
    };

    const int horizon = (int)(H * 0.46f);

    // небо: синевато-серое, светлеет к горизонту
    for (int y = 0; y < horizon; ++y)
    {
        float t = (float)y / horizon;
        uint32_t r = (uint32_t)(96 + 70 * t), g = (uint32_t)(110 + 66 * t), b = (uint32_t)(126 + 52 * t);
        rect(0, y, W, y + 1, 0xff000000 | (b << 16) | (g << 8) | r);
    }

    // песчаная земля с полосами
    for (int y = horizon; y < H; ++y)
    {
        float t = (float)(y - horizon) / (H - horizon);
        uint32_t r = (uint32_t)(150 + 55 * t), g = (uint32_t)(132 + 40 * t), b = (uint32_t)(96 + 22 * t);
        uint32_t c = 0xff000000 | (b << 16) | (g << 8) | r;
        rect(0, y, W, y + 1, c);
        if ((y - horizon) % 24 == 0) rect(0, y, W, y + 1, 0xff000000 | ((b - 14) << 16) | ((g - 14) << 8) | (r - 16));
    }

    // дальняя стена
    rect(0, horizon - 130, W, horizon, 0xff000000 | (0x6a << 16) | (0x74 << 8) | 0x86);
    // арка в стене
    rect((int)(W * 0.42f), horizon - 176, (int)(W * 0.58f), horizon, 0xff000000 | (0x40 << 16) | (0x46 << 8) | 0x52);

    // ящики слева, ближе к камере
    rect((int)(W * 0.06f), (int)(H * 0.56f), (int)(W * 0.20f), (int)(H * 0.70f), 0xff000000 | (0x4c << 16) | (0x53 << 8) | 0x5e);
    rect((int)(W * 0.08f), (int)(H * 0.50f), (int)(W * 0.17f), (int)(H * 0.58f), 0xff000000 | (0x58 << 16) | (0x60 << 8) | 0x6c);

    // ящики справа
    rect((int)(W * 0.76f), (int)(H * 0.52f), (int)(W * 0.90f), (int)(H * 0.68f), 0xff000000 | (0x50 << 16) | (0x58 << 8) | 0x64);

    // модель игрока в центре (силуэт CT)
    const int px = W / 2, py = (int)(H * 0.60f);
    rect(px - 16, py - 96, px + 16, py - 60, 0xff000000 | (0x3c << 16) | (0x44 << 8) | 0x50); // голова
    rect(px - 30, py - 58, px + 30, py + 46, 0xff000000 | (0x32 << 16) | (0x3a << 8) | 0x46);  // торс
    rect(px - 30, py + 46, px - 10, py + 110, 0xff000000 | (0x2a << 16) | (0x32 << 8) | 0x3e); // ноги
    rect(px + 10, py + 46, px + 30, py + 110, 0xff000000 | (0x2a << 16) | (0x32 << 8) | 0x3e);

    // HUD: полоска здоровья и патроны
    rect(24, H - 60, 220, H - 40, 0xaa000000 | (0x20 << 16) | (0x20 << 8) | 0x20);
    rect(26, H - 58, 26 + (int)(192 * 0.78f), H - 42, 0xcc000000 | (0x30 << 16) | (0x90 << 8) | 0x40);
    rect(W - 160, H - 64, W - 24, H - 28, 0x99000000 | (0x18 << 16) | (0x18 << 8) | 0x18);

    // прицел
    const int cx = W / 2, cy = H / 2;
    for (int i = 0; i < 14; ++i)
    {
        put(cx - 22 - i, cy, 0xff5ad25a); put(cx + 22 + i, cy, 0xff5ad25a);
        put(cx, cy - 22 - i, 0xff5ad25a); put(cx, cy + 22 + i, 0xff5ad25a);
    }
    for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) put(cx + dx, cy + dy, 0xff5ad25a);
}

int main(int argc, char** argv)
{
    const char* out_path = (argc > 1) ? argv[1] : "menu.png";
    const int scale = (argc > 2) ? atoi(argv[2]) : 2;
    const int tab = (argc > 3) ? atoi(argv[3]) : 0;
    const char* font_path = (argc > 4) ? argv[4] : "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1920.0f, 1080.0f);
    io.DeltaTime = 1.0f / 60.0f;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.BackendFlags = 0;

    // масштаб UI: меню 820x560 должно занимать ~78% высоты кадра
    const float ui_scale = 1080.0f / 617.0f;   // окно = высота кадра
    gui::geo::SetScale(ui_scale);
    gui::Apply();

    ImFontConfig cfg;
    cfg.SizePixels = gui::geo::TEXT_SIZE;
    cfg.OversampleH = 1;
    cfg.OversampleV = 1;
    cfg.PixelSnapH = true;
    ImFont* font = io.Fonts->AddFontFromFileTTF(font_path, cfg.SizePixels, &cfg);
    if (!font)
    {
        fprintf(stderr, "[!] не загрузился шрифт: %s\n", font_path);
        return 1;
    }
    io.Fonts->Build();
    io.FontDefault = font;
    ImportAtlasFromImGui();

    gui::Settings& s = gui::State();
    s.menu_open = true;
    s.active_tab = tab;

    // пара кадров, чтобы ImGui вычислил размеры и устоялся layout
    for (int frame = 0; frame < 3; ++frame)
    {
        ImGui::NewFrame();
        gui::Render(s);
        ImGui::EndFrame();
    }

    ImGui::NewFrame();
    gui::Render(s);
    ImGui::Render();
    ImDrawData* dd = ImGui::GetDrawData();

    render::Image img;
    const int W = (int)io.DisplaySize.x;
    const int H = (int)io.DisplaySize.y;
    img.resize(W, H);
    draw_game_background(img);

    printf("[*] рендер %dx%d, draw lists: %d, вершин: %d\n",
           W, H, dd->CmdListsCount, dd->TotalVtxCount);

    render::render_draw_data(img, dd);

    if (!stbi_write_png(out_path, W, H, 4, img.px.data(), W * 4))
    {
        fprintf(stderr, "[!] не записался PNG: %s\n", out_path);
        return 1;
    }
    printf("[+] сохранено: %s (%dx%d)\n", out_path, W, H);

    ImGui::DestroyContext();
    return 0;
}
