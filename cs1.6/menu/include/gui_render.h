// gui_render.h — программный растеризатор для headless-скриншотов.
//
// Зачем: ImGui рисует в список команд (ImDrawData). Чтобы отрендерить меню без окна
// и без GPU, мы интерпретируем эти команды на CPU: треугольники, scissor-клип,
// альфа-блендинг. Затем апскейлим картинку до целевого разрешения (сеткой, чтобы
// текст оставался резким).
//
// Итог: полноценный PNG меню, без запуска игры и без видеокарты.
#pragma once

// guard-макрос: по нему gui_icons_init.h понимает, что доступен CPU-растеризатор
#define GUI_RENDER_H 1

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

    // Пользовательские текстуры (спрайты иконок и т.п.), декодированные в RGBA.
    struct Texture
    {
        int w = 0, h = 0;
        std::vector<uint32_t> px;   // 0xAABBGGRR
    };
    // Идентификатор текстуры в ImGui 1.93 — ImTextureID (unsigned long long).
    inline std::vector<std::pair<ImTextureID, Texture>>& Textures()
    {
        static std::vector<std::pair<ImTextureID, Texture>> t;
        return t;
    }
    inline ImTextureID RegisterTexture(Texture tex)
    {
        const ImTextureID id = (ImTextureID)(0x1000 + Textures().size());
        Textures().push_back({id, std::move(tex)});
        return id;
    }
    inline const Texture* FindTexture(ImTextureID id)
    {
        for (auto& kv : Textures())
            if (kv.first == id) return &kv.second;
        return nullptr;
    }

    constexpr ImTextureID FONT_ATLAS_TEX_ID = (ImTextureID)0xFFFFFF01ull;

    // Бэкенд-часть 1.92+: обслуживает запросы ImDrawData::Textures
    // (шрифтовой атлас создаётся/обновляется движком ImGui сам).
    inline void backend_update_textures(ImDrawData* dd)
    {
        if (!dd->Textures) return;
        for (ImTextureData* td : *dd->Textures)
        {
            if (td->Status == ImTextureStatus_WantCreate ||
                td->Status == ImTextureStatus_WantUpdates)
            {
                // Шрифтовой атлас создаётся самим ImGui; в headless у него нет
                // бэкенда, поэтому выдаём служебный ID, чтобы проходили ассерты.
                // Пиксели шрифта берём из g_atlas, регистрировать его не нужно.
                if (td->TexID == ImTextureID_Invalid)
                {
                    td->SetTexID(FONT_ATLAS_TEX_ID);
                    td->SetStatus(ImTextureStatus_OK);
                    continue;
                }
                // копируем пиксели атласа в CPU-текстуру по её TexID
                Texture* dst = nullptr;
                for (auto& kv : Textures())
                    if (kv.first == td->TexID) { dst = &kv.second; break; }
                if (!dst)
                {
                    Texture t;
                    t.w = td->Width;
                    t.h = td->Height;
                    t.px.assign((size_t)t.w * t.h, 0u);
                    Textures().push_back({td->TexID, std::move(t)});
                    dst = &Textures().back().second;
                }
                if (td->Format == ImTextureFormat_Alpha8)
                {
                    for (int y = 0; y < td->Height; ++y)
                        for (int x = 0; x < td->Width; ++x)
                        {
                            const uint8_t a = td->Pixels[y * td->GetPitch() + x];
                            dst->px[(size_t)y * dst->w + x] = ((uint32_t)a << 24) | 0x00ffffffu;
                        }
                }
                else
                {
                    for (int y = 0; y < td->Height; ++y)
                        for (int x = 0; x < td->Width; ++x)
                        {
                            const unsigned char* p = td->Pixels + y * td->GetPitch() + x * 4;
                            dst->px[(size_t)y * dst->w + x] =
                                ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) |
                                ((uint32_t)p[1] << 8) | p[0];
                        }
                }
                td->SetStatus(ImTextureStatus_OK);
            }
        }
    }

    static inline void fill_triangle(Image& img, Clip clip,
                                     const ImDrawVert& a, const ImDrawVert& b, const ImDrawVert& c,
                                     const Texture* tex = nullptr)
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

                // спрайт: билинейно сэмплируем RGBA-текстуру
                if (tex && tex->w > 0 && tex->h > 0)
                {
                    const float fx = (ur * tex->w) - 0.5f;
                    const float fy = (vr * tex->h) - 0.5f;
                    const int x0 = (int)std::floor(fx), y0 = (int)std::floor(fy);
                    const float ax = fx - x0, ay = fy - y0;
                    auto tap = [&](int xx, int yy) -> uint32_t {
                        xx = std::clamp(xx, 0, tex->w - 1);
                        yy = std::clamp(yy, 0, tex->h - 1);
                        return tex->px[(size_t)yy * tex->w + xx];
                    };
                    const uint32_t p00 = tap(x0, y0), p10 = tap(x0 + 1, y0);
                    const uint32_t p01 = tap(x0, y0 + 1), p11 = tap(x0 + 1, y0 + 1);
                    auto bilerp = [&](int shift) -> float {
                        const float v00 = (p00 >> shift) & 0xff, v10 = (p10 >> shift) & 0xff;
                        const float v01 = (p01 >> shift) & 0xff, v11 = (p11 >> shift) & 0xff;
                        const float top = v00 + (v10 - v00) * ax;
                        const float bot = v01 + (v11 - v01) * ax;
                        return top + (bot - top) * ay;
                    };
                    const uint32_t sr = (uint32_t)bilerp(0), sg = (uint32_t)bilerp(8);
                    const uint32_t sb = (uint32_t)bilerp(16);
                    const uint32_t sa = (uint32_t)bilerp(24);

                    // тонирование: текстура белая, цвет берём из вершины
                    const uint32_t cr = col & 0xff, cg = (col >> 8) & 0xff, cb = (col >> 16) & 0xff;
                    const uint32_t ca = (col >> 24) & 0xff;
                    const uint32_t out_a = sa * ca / 255;
                    if (out_a == 0) continue;
                    col = (out_a << 24) | ((sb * cb / 255) << 16) | ((sg * cg / 255) << 8) | (sr * cr / 255);
                    img.px[(size_t)y * img.w + x] = blend(img.px[(size_t)y * img.w + x], col);
                    continue;
                }

                // шрифт: сэмплируем атлас глифов (8-битный alpha)
                if (g_atlas.valid && ur > 0.0f && vr > 0.0f)
                {
                    // ТОЧНАЯ выборка глифа: UV → пиксель атласа.
                    // Атлас ImGui держит по 1px прозрачного запаса вокруг глифа,
                    // поэтому берём целый тексель без интерполяции: билинейка
                    // размазывала штрихи и буквы «слипались».
                    const float fx = ur * g_atlas.w;
                    const float fy = vr * g_atlas.h;
                    const int tx = std::clamp((int)fx, 0, g_atlas.w - 1);
                    const int ty = std::clamp((int)fy, 0, g_atlas.h - 1);
                    const float af = (float)g_atlas.pixels[(size_t)ty * g_atlas.w + tx];
                    if (af < 0.5f) continue;
                    const uint32_t a8 = (uint32_t)(((col >> 24) & 0xff) * af / 255.0f);
                    col = (a8 << 24) | (col & 0x00ffffff);
                }

                img.px[(size_t)y * img.w + x] = blend(img.px[(size_t)y * img.w + x], col);
            }
        }
    }

    // Фон меню gamesense: шахматный дизеринг #0C0C0C/#141414.
    // Рисуется напрямую в кадр (ImDrawList не тянет 60k прямоугольников:
    // лимит 16-битных индексов), координаты — экранные, 1:1.
    inline void dither_rect(Image& img, float x0, float y0, float x1, float y1)
    {
        const ImU32 bg = 0xff0c0c0c, fg = 0xff141414;
        const int ix0 = std::max(0, (int)x0), iy0 = std::max(0, (int)y0);
        const int ix1 = std::min(img.w, (int)x1), iy1 = std::min(img.h, (int)y1);
        // Точный паттерн референса (снят по 2D-карте на двух зонах):
        //   чётные строки: #141414 при x%4 == 1 или 3
        //   нечётные      : чередование по периоду 4 —
        //                   y%4==1 -> x%4==1,  y%4==3 -> x%4==0,1,3
        // Это даёт вертикальные полосы 3px #141414 + 1px #0C0C0C.
        for (int y = iy0; y < iy1; ++y)
        {
            const int ym = y & 3;
            for (int x = ix0; x < ix1; ++x)
            {
                const int xm = x & 3;
                bool on;
                if ((y & 1) == 0)          on = (xm == 1 || xm == 3);
                else if (ym == 1)          on = (xm == 1 || xm == 2 || xm == 3);
                else                       on = (xm == 0 || xm == 1 || xm == 3);
                img.px[(size_t)y * img.w + x] = on ? fg : bg;
            }
        }
    }

    // Дизеринг рисуется ТОЛЬКО в «пустых» зонах окна меню:
    //  - полоса под радугой (y 10..30) и над нижней рамкой (y 592..610)
    //  - зазор между сайдбаром и колонкой A (x 81..100)
    //  - зазор между колонками A и B (x 393..413)
    //  - правый отступ (x 706..724)
    inline void dither_menu_background(Image& img, float WX, float WY, float W, float H)
    {
        dither_rect(img, WX + 1, WY + 11, WX + W - 1, WY + 30);
        dither_rect(img, WX + 6, WY + 593, WX + W - 6, WY + 611);   // в REF y=593 уже дизеринг
        dither_rect(img, WX + 81, WY + 31, WX + 100, WY + 591);
        dither_rect(img, WX + 393, WY + 31, WX + 414, WY + 591);
        dither_rect(img, WX + 706, WY + 31, WX + 725, WY + 591);
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

                const Texture* tex = FindTexture(cmd.GetTexID());

                for (unsigned i = 0; i + 2 < cmd.ElemCount; i += 3)
                {
                    const ImDrawVert& a = vtx[idx[cmd.IdxOffset + i + 0]];
                    const ImDrawVert& b = vtx[idx[cmd.IdxOffset + i + 1]];
                    const ImDrawVert& c = vtx[idx[cmd.IdxOffset + i + 2]];
                    fill_triangle(img, clip, a, b, c, tex);
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
