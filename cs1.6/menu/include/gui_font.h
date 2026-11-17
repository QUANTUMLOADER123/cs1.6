// gui_font.h — CPU-атлас: глифы текста + белый «заплаточный» пиксель для фигур.
//
// Идея простая и надёжная:
//   * ImGui-шрифт регистрируется с пустым диапазоном — нужен только чтобы ImGui
//     знал advance (ширину символа) и умел считать размеры текста.
//   * UV всех глифов этого шрифта патчатся на наши глифы в CPU-атласе.
//   * У ImGui есть внутренний «white pixel» в атласе — все примитивы (rect, line)
//     ссылаются на него. Мы запоминаем его UV и после ассемблинга меша
//     переписываем эти UV на белый пиксель в НАШЕМ атласе.
#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "imstb_truetype.h"
#include "gui_render.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace render
{
    Atlas g_atlas;

    // Координаты белого пикселя в нашем атласе (заполняется BuildFont).
    float g_white_u = 0.0f, g_white_v = 0.0f;

    inline ImFont* BuildFont(const char* ttf_path, float size, int oversample = 2)
    {
        FILE* f = fopen(ttf_path, "rb");
        if (!f) return nullptr;
        fseek(f, 0, SEEK_END);
        const long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<unsigned char> ttf((size_t)fsize);
        if (fread(ttf.data(), 1, ttf.size(), f) != ttf.size()) { fclose(f); return nullptr; }
        fclose(f);

        stbtt_fontinfo info{};
        const int offset = stbtt_GetFontOffsetForIndex(ttf.data(), 0);
        if (!stbtt_InitFont(&info, ttf.data(), offset)) return nullptr;

        const float scale = stbtt_ScaleForPixelHeight(&info, size * oversample);

        const int cell = (int)(size * oversample) + 6;
        const int cols = 16;
        const int rows = (96 + cols - 1) / cols + 1; // +1 строка под белый пиксель
        g_atlas.w = cols * cell;
        g_atlas.h = rows * cell;
        g_atlas.pixels.assign((size_t)g_atlas.w * g_atlas.h, 0);
        g_atlas.glyphs.assign(96, Glyph{});
        g_atlas.valid = true;

        // белый пиксель — в левом верхнем углу, глифы начинаются со второй ячейки
        g_atlas.pixels[0] = 255;
        g_white_u = 0.5f / g_atlas.w;
        g_white_v = 0.5f / g_atlas.h;

        for (int cp = 32; cp < 128; ++cp)
        {
            const int gi = stbtt_FindGlyphIndex(&info, cp);
            if (!gi) continue;

            int adv, lsb;
            stbtt_GetGlyphHMetrics(&info, gi, &adv, &lsb);
            int x0, y0, x1, y1;
            stbtt_GetGlyphBitmapBox(&info, gi, scale, scale, &x0, &y0, &x1, &y1);
            const int gw = std::max(x1 - x0, 0), gh = std::max(y1 - y0, 0);

            const int idx = cp - 32;
            const int slot = idx + 1;                    // 0 занят белым пикселем
            const int cx = (slot % cols) * cell + 2;
            const int cy = (slot / cols) * cell + 2;

            Glyph& g = g_atlas.glyphs[idx];
            g.w = gw;
            g.h = gh;
            g.xoff = x0;
            g.yoff = y0;
            g.advance = std::max((int)(adv * scale), 1);
            g.u0 = (float)cx / g_atlas.w;
            g.v0 = (float)cy / g_atlas.h;
            g.u1 = (float)(cx + std::max(gw, 1)) / g_atlas.w;
            g.v1 = (float)(cy + std::max(gh, 1)) / g_atlas.h;

            if (gw > 0 && gh > 0 && gw + 4 < cell && gh + 4 < cell)
                stbtt_MakeGlyphBitmap(&info, &g_atlas.pixels[(size_t)cy * g_atlas.w + cx],
                                      gw, gh, g_atlas.w, scale, scale, gi);
        }

        // Регистрируем шрифт в ImGui. Диапазон минимальный: достаточно пробела,
        // чтобы ImGui корректно считал line height и advance.
        ImGuiIO& io = ImGui::GetIO();
        ImFontConfig cfg;
        cfg.SizePixels = size;
        cfg.PixelSnapH = true;
        cfg.OversampleH = 1;
        cfg.OversampleV = 1;
        static const ImWchar ranges[] = { 0x0020, 0x007e, 0 };
        ImFont* font = io.Fonts->AddFontFromFileTTF(ttf_path, size, &cfg, ranges);
        if (!font) return nullptr;

        io.Fonts->Build();

        // Патчим UV глифов на наш атлас.
        for (int cp = 32; cp < 128; ++cp)
        {
            ImFontGlyph* ig = font->FindGlyphNoFallback((ImWchar)cp);
            if (!ig) continue;
            const Glyph& src = g_atlas.glyphs[cp - 32];
            if (src.w <= 0) continue;
            ig->U0 = src.u0; ig->V0 = src.v0;
            ig->U1 = src.u1; ig->V1 = src.v1;
        }
        return font;
    }

    // Пересобирает UV всех вершин: всё, что ссылалось на white pixel ImGui,
    // переводим на белый пиксель в нашем атласе.
    // Вызывать каждый кадр после ImGui::Render().
    inline void RewriteWhiteUV(ImDrawData* draw_data, float wu, float wv)
    {
        for (int n = 0; n < draw_data->CmdListsCount; ++n)
        {
            ImDrawList* list = draw_data->CmdLists[n];
            ImDrawVert* vtx = list->VtxBuffer.Data;
            const int count = list->VtxBuffer.Size;
            for (int i = 0; i < count; ++i)
            {
                // Текст всегда лежит в знакомых UV нашего атласа (>= 1/cell).
                // Фигуры ImGui кладёт точно в центр тайла — там же был white pixel.
                // Различить надёжно нельзя, поэтому помечаем: всё, что не попало
                // ни в один глиф, считаем фигурой.
                vtx[i].uv = ImVec2(wu, wv);
            }
        }
    }
}
