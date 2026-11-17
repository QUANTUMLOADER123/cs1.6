// gui_widgets.h — виджеты gamesense, воспроизведённые по пикселям.
//
// Замеры с референса 731x617:
//   чекбокс          x 122..127 (6px), y 53..59 (7px с учётом рамки),
//                    выключен — #474747 (71,71,71); включён — градиент
//                    (156,199,40) сверху до (122,165,6) снизу (6 строк)
//   текст чекбокса   x 143, cap-height 52..61, #CDCDCD
//   дропдаун         x 143..334 (192px), градиент #1F1F1F -> #242424,
//                    шеврон на x 325..330, y 108..110
//   шапка колонки    буква A на x 115..119, y 27..31 (7x5 глиф в 9px шрифте)
//   тело колонки     #171717, рамка #303030, x 101..392 / 415..705, y 31..591
#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "gui_theme.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace gui
{
    // Индексы плиток в ОРИГИНАЛЬНОМ tab_icons.png (8 штук по 48x48).
    // Порядок вкладок в самом gamesense (сверху вниз): Rage, Anti-Aim, Visuals,
    // Misc, Settings, Skins, Players, Lua.
    enum class Icon
    {
        Headshot = 0,   // Rage
        NoSpread = 6,   // Anti-Aim (pistol in circle)
        Crosshair = 1,  // Visuals
        SunMoon = 2,    // Misc
        Gears = 3,      // Settings
        Knife = 4,      // Skins
        Player = 5,     // Players
        Floppy = 7,     // Lua
        Count = 8
    };

    struct IconAtlas
    {
        int w = 0, h = 0, tile = 0;
        ImTextureID tex = (ImTextureID)0;
        bool ready = false;
    };
    inline IconAtlas& Atlas()
    {
        static IconAtlas a;
        return a;
    }

    // Крупный шрифт для подписи вкладки в сайдбаре (бейк ~21px).
    inline ImFont*& BigFont()
    {
        static ImFont* f = nullptr;
        return f;
    }
    inline void SetBigFont(ImFont* f) { BigFont() = f; }

    // Рисует ОРИГИНАЛЬНУЮ иконку из спрайта gamesense: тайл 48x48, тонирование.
    inline void DrawIcon(ImDrawList* dl, Icon ic, ImVec2 c, float size, ImU32 tint)
    {
        IconAtlas& a = Atlas();
        if (!a.ready || a.tex == (ImTextureID)0)
            return;

        const int idx = (int)ic;
        const float t = (float)a.tile;
        const float pu = 0.5f / (float)a.w;
        const float u0 = (idx * t) / (float)a.w + pu;
        const float u1 = ((idx + 1) * t) / (float)a.w - pu;
        const float pv = 2.0f / (float)a.h;

        // Пропорции сохраняем: в референсе иконки чуть «сплюснуты» по Y
        // (напр. 38x34 при тайле 48x48) — это исходный спрайт, а не искажение.
        const float half_w = size * 0.5f;
        const float half_h = size * geo::ICON_ASPECT * 0.5f;
        dl->AddImage(a.tex,
                     ImVec2(c.x - half_w, c.y - half_h),
                     ImVec2(c.x + half_w, c.y + half_h),
                     ImVec2(u0, pv), ImVec2(u1, 1.0f - pv), tint);
    }

    // ------------------------------------------------------------------
    // Текст с точным межбуквенным интервалом.
    //
    // В оригинале gamesense (GDI/FreeType) буквы стоят на целых пикселях и
    // между ними есть зазор. ImGui-атлас пакует глифы вплотную, поэтому
    // рисуем каждый символ отдельно и сдвигаем курсор на целое число пикселей.
    // ------------------------------------------------------------------
    inline void DrawTextSpaced(ImDrawList* dl, ImVec2 pos, ImU32 color, const char* text,
                               float size, float extra = 0.0f, ImFont* font_override = nullptr)
    {
        ImFont* font = font_override ? font_override : ImGui::GetFont();
        ImFontBaked* baked = font->GetFontBaked(size);
        if (!baked) { dl->AddText(font, size, pos, color, text); return; }

        float x = pos.x;
        const float y = pos.y;
        for (const char* p = text; *p; ++p)
        {
            const unsigned char ch = (unsigned char)*p;
            if (ch == ' ')
            {
                x += baked->GetCharAdvance(' ') + extra;
                continue;
            }
            const ImFontGlyph* g = baked->FindGlyph((ImWchar)ch);
            if (!g) continue;
            // AddText сам учитывает g->X0/Y0, поэтому позицию не смещаем —
            // иначе глифы уезжают вниз на высоту строки.
            dl->AddText(font, size, ImVec2(ImFloor(x), ImFloor(y)), color, p, p + 1);
            x += g->AdvanceX + extra;
        }
    }

    // ------------------------------------------------------------------
    // Вертикальный градиент (2 цвета) — как у полей gamesense.
    // ------------------------------------------------------------------
    inline void VGradient(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 top, ImU32 bot)
    {
        dl->AddRectFilledMultiColor(a, b, top, top, bot, bot);
    }

    // ------------------------------------------------------------------
    // Чекбокс 6x6 + текст. Полностью ручная отрисовка в абсолютных
    // координатах колонки — так проще держать пиксельную точность.
    // Возвращает true, если значение изменилось.
    // ------------------------------------------------------------------
    inline bool CheckboxAbs(ImVec2 col_pos, float& row_y, const char* label, bool* v,
                            float indent = 0.0f)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float y = col_pos.y + row_y * geo::SCALE - 1 * geo::SCALE;   // ink-top текста -1
        const float row_h = geo::ITEM_H * geo::SCALE;
        const float cb = geo::CHECK_SIZE * geo::SCALE;
        // референс: ink текста 52, верх бокса 54 => бокс на 2px ниже
        // референс: бокс 123..128, текст ink 143 (шаг 20 от бокса)
        // x=122 (бордер) -> отсюда +1 в CHECK_X даёт ровно 122 при COL_X_A=101
        const ImVec2 cpos(col_pos.x + (geo::CHECK_X - geo::COL_X_A) * geo::SCALE + indent,
                          y + 3 * geo::SCALE);
        const ImVec2 tpos(col_pos.x + (geo::LABEL_X - geo::COL_X_A) * geo::SCALE + indent,
                          y);

        // хитбокс на всю строку — как в gamesense (клик по тексту тоже работает)
        char id[128];
        snprintf(id, sizeof(id), "##cb_%s_%.0f_%.0f", label, cpos.x, cpos.y);
        ImGui::SetCursorScreenPos(ImVec2(cpos.x, cpos.y));
        ImGui::InvisibleButton(id, ImVec2(geo::COL_W - (cpos.x - col_pos.x) - 6 * geo::SCALE,
                                          row_h));
        const bool hovered = ImGui::IsItemHovered();
        bool changed = false;
        if (ImGui::IsItemClicked())
        {
            *v = !*v;
            changed = true;
        }

        // Бокс 9x9 = 1px бордер #0C0C0C со всех сторон + 7x7 заливки.
        // Снято с референса: y=53 бордер, y=54..59 заливка (6 строк, шаг ~7),
        // y=60 бордер; x=122 бордер, x=123..128 заливка, x=129 бордер.
        {
            const int x0 = (int)ImFloor(cpos.x), y0 = (int)ImFloor(cpos.y);
            const int n = 9;      // 1 бордер + 6 заливки + 1 бордер + 1 запас снизу
            const ImU32 bd = col::CHECK_BORDER;
            dl->AddRectFilled(ImVec2((float)x0, (float)y0), ImVec2((float)(x0 + n), (float)(y0 + 1)), bd);
            dl->AddRectFilled(ImVec2((float)x0, (float)(y0 + 7)), ImVec2((float)(x0 + n), (float)(y0 + 8)), bd);
            dl->AddRectFilled(ImVec2((float)x0, (float)y0), ImVec2((float)(x0 + 1), (float)(y0 + n)), bd);
            dl->AddRectFilled(ImVec2((float)(x0 + n - 1), (float)y0), ImVec2((float)(x0 + n), (float)(y0 + n)), bd);
            // 6 ступеней заливки (в REF ровно 6 строк между бордерами)
            const int rTop = *v ? 156 : 77,  rBot = *v ? 122 : 52;
            const int gTop = *v ? 199 : 77,  gBot = *v ? 165 : 52;
            const int bTop = *v ? 40  : 77,  bBot = *v ? 6   : 52;
            for (int i = 0; i < 6; ++i)
            {
                const float u = (float)i / 5.0f;
                const int r = (int)std::lround(rTop + (rBot - rTop) * u);
                const int g = (int)std::lround(gTop + (gBot - gTop) * u);
                const int b = (int)std::lround(bTop + (bBot - bTop) * u);
                dl->AddRectFilled(ImVec2((float)(x0 + 1), (float)(y0 + 1 + i)),
                                  ImVec2((float)(x0 + n - 1), (float)(y0 + 2 + i)),
                                  IM_COL32(r, g, b, 255));
            }
            (void)hovered;
        }

        DrawTextSpaced(dl, tpos, col::TEXT, label, geo::TEXT_SIZE, 0.5f * geo::SCALE);
        row_y += geo::ITEM_H;
        return changed;
    }

    // ------------------------------------------------------------------
    // Кнопка gamesense: 1px бордер #0C0C0C, градиентная плашка 24px,
    // текст по центру. Ширина — на всю колонку полей (x 153..333).
    // ------------------------------------------------------------------
    inline bool ButtonAbs(ImVec2 col_pos, float& row_y, const char* label,
                          float w = 181.0f)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float y = col_pos.y + row_y * geo::SCALE + 4.0f * geo::SCALE;
        const float h = 22.0f * geo::SCALE;
        const ImVec2 p(col_pos.x + (geo::FIELD_X - geo::COL_X_A) * geo::SCALE, y);
        const ImVec2 q(p.x + w * geo::SCALE, p.y + h);

        ImGui::SetCursorScreenPos(p);
        ImGui::InvisibleButton(label, ImVec2(q.x - p.x, h));
        const bool pressed = ImGui::IsItemClicked();
        const bool hovered = ImGui::IsItemHovered();

        const int x0 = (int)ImFloor(p.x), y0 = (int)ImFloor(p.y);
        const int x1 = (int)ImFloor(q.x) - 1, y1 = (int)ImFloor(q.y) - 1;
        dl->AddRectFilled(ImVec2((float)x0, (float)y0), ImVec2((float)(x1 + 1), (float)(y0 + 1)), col::CHECK_BORDER);
        dl->AddRectFilled(ImVec2((float)x0, (float)y1), ImVec2((float)(x1 + 1), (float)(y1 + 1)), col::CHECK_BORDER);
        dl->AddRectFilled(ImVec2((float)x0, (float)y0), ImVec2((float)(x0 + 1), (float)(y1 + 1)), col::CHECK_BORDER);
        dl->AddRectFilled(ImVec2((float)x1, (float)y0), ImVec2((float)(x1 + 1), (float)(y1 + 1)), col::CHECK_BORDER);
        const int t = hovered ? 52 : 44, bo = hovered ? 52 : 44;
        for (int i = y0 + 1; i < y1; ++i)
        {
            const float u = (float)(i - y0 - 1) / (float)std::max(1, (y1 - y0 - 2));
            const int v = (int)std::lround(t + (bo - t) * u);
            dl->AddRectFilled(ImVec2((float)(x0 + 1), (float)i), ImVec2((float)x1, (float)(i + 1)),
                              IM_COL32(v, v, v, 255));
        }

        ImFontBaked* bk = ImGui::GetFont()->GetFontBaked(geo::TEXT_SIZE);
        const float gh = bk ? bk->Size : geo::TEXT_SIZE;
        float tw = 0.0f;
        if (bk)
            for (const char* c = label; *c; ++c) tw += bk->GetCharAdvance((ImWchar)*c);
        const float tx = ImFloor((p.x + q.x) * 0.5f - tw * 0.5f);
        const float ty = ImFloor(p.y + (h - gh) * 0.5f) + 1.0f;
        DrawTextSpaced(dl, ImVec2(tx, ty), col::TEXT, label, geo::TEXT_SIZE, 0.5f * geo::SCALE);
        // (курсор НЕ двигаем: строку задаёт ColTo у вызывающего кода)
        return pressed;
    }

    // ------------------------------------------------------------------
    // Простая подпись (заголовок группы). Высота 14px как у чекбокса.
    // ------------------------------------------------------------------
    // Подпись НЕ двигает курсор: в референсе подпись и её поле занимают одну
    // "строку" (подпись сверху, поле снизу), следующую задаёт ColTo.
    inline void LabelAbs(ImVec2 col_pos, float& row_y, const char* text)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 p(col_pos.x + (geo::LABEL_X - 1 - geo::COL_X_A) * geo::SCALE,
                       col_pos.y + row_y * geo::SCALE - 1 * geo::SCALE);
        DrawTextSpaced(dl, p, col::TEXT, text, geo::TEXT_SIZE, 0.5f * geo::SCALE);
    }

    // ------------------------------------------------------------------
    // Дропдаун: 192px шириной, градиент #1F1F1F -> #242424, шеврон справа.
    // Открывает обычный попап ImGui со списком.
    // ------------------------------------------------------------------
    inline bool ComboAbs(ImVec2 col_pos, float& row_y, const char* id, int* current,
                         const char* const items[], int count)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // ink-top текста поля -> верх поля ровно +12px (референс: текст 89, поле 101)
        const float y = col_pos.y + row_y * geo::SCALE + 14.0f * geo::SCALE;
        const float fh = 20.0f * geo::SCALE;   // бордер 1px + 18px градиента + бордер 1px
        const float w = 181.0f * geo::SCALE;   // x 153..333
        const ImVec2 p(col_pos.x + (geo::FIELD_X - geo::COL_X_A) * geo::SCALE, y);
        const ImVec2 q(p.x + w, p.y + fh);

        ImGui::SetCursorScreenPos(p);
        ImGui::InvisibleButton(id, ImVec2(w, fh));
        const bool hovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked())
            ImGui::OpenPopup(id);

        if (hovered)
        {
            VGradient(dl, p, q, IM_COL32(40, 40, 40, 255), IM_COL32(45, 45, 45, 255));
        }
        else
        {
            const float px = ImFloor(p.x), pw = ImFloor(q.x) - px;
            // бордеры поля: 1px #0C0C0C сверху и снизу (REF: y=100 и y=119)
            dl->AddRectFilled(ImVec2(px, ImFloor(p.y)), ImVec2(px + pw, ImFloor(p.y) + 1),
                              col::CHECK_BORDER);
            dl->AddRectFilled(ImVec2(px, ImFloor(p.y) + 19), ImVec2(px + pw, ImFloor(p.y) + 20),
                              col::CHECK_BORDER);
            // 18 ступеней градиента — снято с референса y=101..118
            static const int ramp[18] = {31,31,32,32,32,32,33,33,33,34,34,34,34,35,35,35,36,36};
            for (int i = 0; i < 18; ++i)
                dl->AddRectFilled(ImVec2(px, ImFloor(p.y) + 1 + i),
                                  ImVec2(px + pw, ImFloor(p.y) + 2 + i),
                                  IM_COL32(ramp[i], ramp[i], ramp[i], 255));
        }

        const ImVec2 ts = ImGui::CalcTextSize(items[*current]);
        DrawTextSpaced(dl, ImVec2(p.x + 11 * geo::SCALE, p.y + (fh - ts.y) * 0.5f),
                       col::FIELD_GLYPH, items[*current], geo::TEXT_SIZE, 0.5f * geo::SCALE);

        // шеврон: 5x3 треугольника на x 325..330, центр y
        const float cxx = q.x - 5 * geo::SCALE;      // центр 328
        const float cyy = p.y + 8 * geo::SCALE;      // верх 108
        const float rw = 2.5f * geo::SCALE;
        dl->AddTriangleFilled(ImVec2(cxx - rw, cyy),
                              ImVec2(cxx + rw, cyy),
                              ImVec2(cxx, cyy + 3 * geo::SCALE),
                              hovered ? col::TEXT : col::CHEVRON);

        bool changed = false;
        ImGui::SetNextWindowPos(ImVec2(p.x, q.y));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(col::FIELD_BOT));
        if (ImGui::BeginPopup(id))
        {
            for (int i = 0; i < count; ++i)
            {
                const bool sel = (*current == i);
                if (ImGui::Selectable(items[i], sel))
                {
                    *current = i;
                    changed = true;
                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        // (курсор НЕ двигаем: строку задаёт ColTo у вызывающего кода)
        return changed;
    }

    // ------------------------------------------------------------------
    // Слайдер gamesense: полоса поля + зелёный «грабер» 5x10.
    // ------------------------------------------------------------------
    inline bool SliderAbs(ImVec2 col_pos, float& row_y, const char* id, float* v,
                          float lo, float hi, const char* fmt)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // та же привязка, что у дропдауна: бордер поля = row_y + 14
        const float y = col_pos.y + row_y * geo::SCALE + 14.0f * geo::SCALE;
        const float fh = 20.0f * geo::SCALE;   // бордер 1px + 18px + бордер 1px
        const float w = 181.0f * geo::SCALE;   // x 153..333
        const ImVec2 p(col_pos.x + (geo::FIELD_X - geo::COL_X_A) * geo::SCALE, y);
        const ImVec2 q(p.x + w, p.y + fh);

        // плашка поля — как у дропдауна
        VGradient(dl, p, q, col::FIELD_TOP, col::FIELD_BOT);
        const float px = ImFloor(p.x), pw = ImFloor(q.x) - px;
        dl->AddRectFilled(ImVec2(px, ImFloor(p.y)), ImVec2(px + pw, ImFloor(p.y) + 1),
                          col::CHECK_BORDER);
        dl->AddRectFilled(ImVec2(px, ImFloor(p.y) + 19), ImVec2(px + pw, ImFloor(p.y) + 20),
                          col::CHECK_BORDER);

        ImGui::SetCursorScreenPos(p);
        ImGui::InvisibleButton(id, ImVec2(w, fh));
        const bool active = ImGui::IsItemActive();
        const float t = std::clamp((*v - lo) / (hi - lo), 0.0f, 1.0f);

        // ЗАЛИВКА трека градиентом #99C427 -> #699404 (как в gamesense),
        // строго внутри бордеров: x 154..332, y 101..118.
        const int fx0 = (int)px + 1;
        const int fy0 = (int)ImFloor(p.y) + 1;
        const int fw = (int)pw - 2;
        const int fill_w = (int)std::floor(fw * t + 0.5f);
        for (int i = 0; i < 18; ++i)
        {
            const float u = (float)i / 17.0f;
            const int r = (int)(0x99 + (0x69 - 0x99) * u);
            const int g = (int)(0xC4 + (0x94 - 0xC4) * u);
            const int b = (int)(0x27 + (0x04 - 0x27) * u);
            if (fill_w > 0)
                dl->AddRectFilled(ImVec2((float)fx0, (float)(fy0 + i)),
                                  ImVec2((float)(fx0 + fill_w), (float)(fy0 + i + 1)),
                                  IM_COL32(r, g, b, 255));
        }

        // Знаки "минус" и "плюс" по краям трека — как в оригинальном меню.
        {
            const float cy = ImFloor(p.y) + 10.0f;
            const ImU32 sc = col::TEXT_DIM;
            dl->AddRectFilled(ImVec2(ImFloor(p.x) - 14.0f, cy), ImVec2(ImFloor(p.x) - 9.0f, cy + 1.0f), sc);
            dl->AddRectFilled(ImVec2(ImFloor(q.x) + 7.0f, cy), ImVec2(ImFloor(q.x) + 12.0f, cy + 1.0f), sc);
            dl->AddRectFilled(ImVec2(ImFloor(q.x) + 9.0f, cy - 2.0f), ImVec2(ImFloor(q.x) + 10.0f, cy + 3.0f), sc);
        }

        char buf[64];
        snprintf(buf, sizeof(buf), fmt, *v);
        {
            ImFontBaked* bk = ImGui::GetFont()->GetFontBaked(geo::TEXT_SIZE);
            const float glyph_h = bk ? bk->Size : geo::TEXT_SIZE;
            const float ty = ImFloor(p.y) + ImFloor((fh - glyph_h) * 0.5f) + 1.0f;
            float tw = 0.0f;
            if (bk)
                for (const char* c = buf; *c; ++c) tw += bk->GetCharAdvance((ImWchar)*c);
            // подложка под числом — только если заливка до него доходит
            if (fill_w > (int)(pw - tw - 16))
                dl->AddRectFilled(ImVec2(px + pw - tw - 13, ImFloor(p.y) + 1),
                                  ImVec2(px + pw - 7, ImFloor(p.y) + 19), col::FIELD_BOT);
            DrawTextSpaced(dl, ImVec2(ImFloor(px + pw - tw - 11), ty),
                           col::TEXT, buf, geo::TEXT_SIZE, 0.5f * geo::SCALE);
        }

        bool changed = false;
        if (active)
        {
            const float mx = ImGui::GetIO().MousePos.x;
            const float nt = std::clamp((mx - p.x) / w, 0.0f, 1.0f);
            const float nv = lo + (hi - lo) * nt;
            if (nv != *v) { *v = nv; changed = true; }
        }

        // (курсор НЕ двигаем: строку задаёт ColTo у вызывающего кода)
        return changed;
    }

    // ------------------------------------------------------------------
    // Рамка колонки A/B: #303030 1px + тело #171717 + буква в левом углу.
    // Рисуется вручную; контент дальше идёт абсолютными координатами.
    // ------------------------------------------------------------------
    inline void DrawColumnFrame(ImVec2 pos, ImVec2 size, const char* letter)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), col::BOX);
        dl->AddRect(pos, ImVec2(pos.x + size.x - 1, pos.y + size.y - 1), col::BOX_BORDER);
        if (letter && *letter)
        {
            // буква чуть выше и левее тела: x 115 (pos.x+14), y 27 (pos.y-4)
            dl->AddText(ImVec2(pos.x + 14 * geo::SCALE, pos.y - 4 * geo::SCALE),
                        col::TEXT, letter);
        }
    }
}
