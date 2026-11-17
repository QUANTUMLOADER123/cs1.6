// gui_render.h — программный растеризатор для headless-скриншотов.
//
// Зачем: ImGui рисует в список команд (ImDrawData). Чтобы отрендерить меню без окна
// и без GPU, мы интерпретируем эти команды на CPU: треугольники, scissor-клип,
// альфа-блендинг. Затем апскейлим картинку до целевого разрешения (сеткой, чтобы
// текст оставался резким).
//
// Итог: полноценный PNG меню, без запуска игры и без видеокарты.
#pragma once

#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace render
{
    struct Image
    {
        int w = 0, h = 0;
        std::vector<uint32_t> px; // RGBA, 0xAABBGGRR (little-endian) — формат ImGui

        void resize(int width, int height)
        {
            w = width; h = height;
            px.assign((size_t)w * h, 0x00000000);
        }
    };

    // Цвет в uint32 (RGBA как в ImGui: R в младшем байте)
    static inline uint32_t blend(uint32_t dst, uint32_t src)
    {
        const uint32_t sa = (src >> 24) & 0xff;
        if (sa == 0) return dst;
        const uint32_t da = (dst >> 24) & 0xff;

        // источник непрозрачный — просто копируем
        if (sa == 255) return src;

        const uint32_t sr = src & 0xff, sg = (src >> 8) & 0xff, sb = (src >> 16) & 0xff;
        const uint32_t dr = dst & 0xff, dg = (dst >> 8) & 0xff, db = (dst >> 16) & 0xff;

        const uint32_t out_a = sa + (da * (255 - sa)) / 255;
        auto mix = [&](uint32_t s, uint32_t d) -> uint32_t {
            return (s * sa + d * da * (255 - sa) / 255) / (out_a ? out_a : 1);
        };
        const uint32_t r = std::min<uint32_t>(255, mix(sr, dr));
        const uint32_t g = std::min<uint32_t>(255, mix(sg, dg));
        const uint32_t b = std::min<uint32_t>(255, mix(sb, db));
        return (out_a << 24) | (b << 16) | (g << 8) | r;
    }

    struct Clip { int x0, y0, x1, y1; };

    // Атлас глифов: заполняется генератором текста (glyph[] ниже).
    struct Glyph { int w, h, xoff, yoff, advance; float u0, v0, u1, v1; };
    struct Atlas
    {
        std::vector<uint8_t> pixels; // alpha
        int w = 0, h = 0;
        std::vector<Glyph> glyphs;   // [cp - 32]
        int first_cp = 32, last_cp = 126;
        bool valid = false;
    };
    extern Atlas g_atlas;

    static inline void fill_triangle(Image& img, Clip clip,
                                     const ImDrawVert& a, const ImDrawVert& b, const ImDrawVert& c)
    {
        // координаты и атрибуты
        float x0 = a.pos.x, y0 = a.pos.y;
        float x1 = b.pos.x, y1 = b.pos.y;
        float x2 = c.pos.x, y2 = c.pos.y;

        const float area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
        if (std::fabs(area) < 1e-6f) return;

        int minx = std::max<int>(clip.x0, (int)std::floor(std::min({x0, x1, x2})));
        int maxx = std::min<int>(clip.x1, (int)std::ceil(std::max({x0, x1, x2})));
        int miny = std::max<int>(clip.y0, (int)std::floor(std::min({y0, y1, y2})));
        int maxy = std::min<int>(clip.y1, (int)std::ceil(std::max({y0, y1, y2})));
        if (minx >= maxx || miny >= maxy) return;

        const float inv_area = 1.0f / area;

        for (int y = miny; y < maxy; ++y)
        {
            const float py = y + 0.5f;
            for (int x = minx; x < maxx; ++x)
            {
                const float px = x + 0.5f;

                // барицентрические веса
                const float w0 = ((x1 - px) * (y2 - py) - (x2 - px) * (y1 - py)) * inv_area;
                const float w1 = ((x2 - px) * (y0 - py) - (x0 - px) * (y2 - py)) * inv_area;
                const float w2 = 1.0f - w0 - w1;
                if (w0 < -0.0001f || w1 < -0.0001f || w2 < -0.0001f) continue;

                // интерполяция цвета и UV
                const float ur = a.uv.x * w0 + b.uv.x * w1 + c.uv.x * w2;
                const float vr = a.uv.y * w0 + b.uv.y * w1 + c.uv.y * w2;

                const uint32_t ca = a.col, cb = b.col, cc = c.col;
                auto lerp8 = [&](int shift) -> uint32_t {
                    const float v = ((ca >> shift) & 0xff) * w0 + ((cb >> shift) & 0xff) * w1 +
                                    ((cc >> shift) & 0xff) * w2;
                    return (uint32_t)std::clamp(v, 0.0f, 255.0f);
                };
                uint32_t col = (lerp8(24) << 24) | (lerp8(16) << 16) | (lerp8(8) << 8) | lerp8(0);

                // шрифт: сэмплируем атлас глифов (8-битный alpha)
                if (g_atlas.valid && ur > 0.0f && vr > 0.0f)
                {
                    const int tx = std::clamp((int)(ur * g_atlas.w), 0, g_atlas.w - 1);
                    const int ty = std::clamp((int)(vr * g_atlas.h), 0, g_atlas.h - 1);
                    const uint8_t alpha = g_atlas.pixels[(size_t)ty * g_atlas.w + tx];
                    if (alpha == 0) continue;
                    // умножаем цвет на alpha текстуры
                    const uint32_t a8 = ((col >> 24) & 0xff) * alpha / 255;
                    col = (a8 << 24) | (col & 0x00ffffff);
                }

                img.px[(size_t)y * img.w + x] = blend(img.px[(size_t)y * img.w + x], col);
            }
        }
    }

    inline void render_draw_data(Image& img, ImDrawData* draw_data)
    {
        for (int n = 0; n < draw_data->CmdListsCount; ++n)
        {
            const ImDrawList* list = draw_data->CmdLists[n];
            const ImDrawVert* vtx = list->VtxBuffer.Data;
            const ImDrawIdx* idx = list->IdxBuffer.Data;

            for (const ImDrawCmd& cmd : list->CmdBuffer)
            {
                if (cmd.UserCallback) continue;

                Clip clip{
                    std::max(0, (int)cmd.ClipRect.x),
                    std::max(0, (int)cmd.ClipRect.y),
                    std::min(img.w, (int)cmd.ClipRect.z),
                    std::min(img.h, (int)cmd.ClipRect.w)
                };
                if (clip.x0 >= clip.x1 || clip.y0 >= clip.y1) continue;

                for (unsigned i = 0; i + 2 < cmd.ElemCount; i += 3)
                {
                    const ImDrawVert& a = vtx[idx[cmd.IdxOffset + i + 0]];
                    const ImDrawVert& b = vtx[idx[cmd.IdxOffset + i + 1]];
                    const ImDrawVert& c = vtx[idx[cmd.IdxOffset + i + 2]];
                    fill_triangle(img, clip, a, b, c);
                }
            }
        }
    }

    // Апскейл «сеткой»: каждый пиксель исходника размножается в блок scale×scale.
    // Текст остаётся ровным, без мыла билинейной интерполяции.
    inline Image upscale_grid(const Image& src, int scale)
    {
        if (scale <= 1) return src;
        Image out;
        out.resize(src.w * scale, src.h * scale);
        for (int y = 0; y < src.h; ++y)
        {
            for (int x = 0; x < src.w; ++x)
            {
                const uint32_t c = src.px[(size_t)y * src.w + x];
                for (int dy = 0; dy < scale; ++dy)
                {
                    uint32_t* row = &out.px[(size_t)(y * scale + dy) * out.w + x * scale];
                    for (int dx = 0; dx < scale; ++dx) row[dx] = c;
                }
            }
        }
        return out;
    }

    // Заливка фона «шахматкой» — так сразу видно альфа-канал (для отладки overlay).
    inline void checkerboard(Image& img, int cell = 8,
                             uint32_t c1 = 0xff202024, uint32_t c2 = 0xff28282c)
    {
        for (int y = 0; y < img.h; ++y)
            for (int x = 0; x < img.w; ++x)
                img.px[(size_t)y * img.w + x] = (((x / cell) + (y / cell)) & 1) ? c1 : c2;
    }
}
