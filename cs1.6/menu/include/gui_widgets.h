// gui_widgets.h — контролы gamesense, портированные по пикселям.
//
// Чекбокс: 6×6, заливка #474747, включённый — #95C021, рамка чёрная 1px.
// Дропдаун: поле #242424, рамка #0C0C0C, высота 13, шеврон справа.
// Иконки: жирные заливки ~38×38, неактивные #5A5A5A, активные #CDCDCD.
#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "gui_theme.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace gui
{
    enum class Icon
    {
        Headshot = 0,   // голова с прострелом
        NoSpread,       // пистолет в запрещающем круге
        Crosshair,      // прицел в круге
        SunMoon,        // солнце/полумесяц (visuals)
        Gears,          // три шестерёнки (settings)
        Knife,          // нож-крюк (skins)
        Player,         // силуэт игрока
        Floppy,         // дискета (configs)
        Count
    };

    inline void DrawIcon(ImDrawList* dl, Icon ic, ImVec2 c, float size, ImU32 color);

    // ------------------------------------------------------------------
    // Чекбокс: квадрат 6×6 + текст #CDCDCD 9px
    // ------------------------------------------------------------------
    inline bool Checkbox(const char* label, bool* v, float indent = 0.0f)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *ImGui::GetCurrentContext();
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = ImGui::CalcTextSize(label);

        const float sq = geo::CHECK_SIZE;
        const ImVec2 pos = ImVec2(window->DC.CursorPos.x + indent, window->DC.CursorPos.y);
        const ImRect total(pos, ImVec2(pos.x + sq + 6 * geo::SCALE + label_size.x,
                                       pos.y + ImMax(sq, label_size.y)));
        ImGui::ItemSize(ImVec2(total.GetWidth(), geo::ITEM_H), 0.0f);
        if (!ImGui::ItemAdd(total, id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(total, id, &hovered, &held);
        if (pressed)
        {
            *v = !(*v);
            ImGui::MarkItemEdited(id);
        }

        ImDrawList* dl = window->DrawList;
        const float sy = pos.y + (geo::ITEM_H - sq) * 0.5f;
        const ImVec2 a(pos.x, sy), b(pos.x + sq, sy + sq);

        dl->AddRectFilled(a, b, *v ? col::ACCENT : (hovered ? IM_COL32(85, 85, 85, 255) : col::CHECK_BOX));
        dl->AddRect(a, b, col::CHECK_BORDER);

        const float tx = b.x + 6 * geo::SCALE;
        const float ty = pos.y + (geo::ITEM_H - label_size.y) * 0.5f;
        dl->AddText(ImVec2(tx, ty), col::TEXT, label);
        return pressed;
    }

    // ------------------------------------------------------------------
    // Текстовая метка (как "T model", "CT model")
    // ------------------------------------------------------------------
    inline void Label(const char* text, ImU32 color = col::TEXT, float indent = 0.0f)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImVec2 pos = ImVec2(window->DC.CursorPos.x + indent, window->DC.CursorPos.y);
        const ImVec2 sz = ImGui::CalcTextSize(text);
        window->DrawList->AddText(pos, color, text);
        ImGui::Dummy(ImVec2(sz.x, geo::ITEM_H));
    }

    // ------------------------------------------------------------------
    // Дропдаун: поле #242424 во всю ширину колонки, стрелка справа
    // ------------------------------------------------------------------
    inline bool Combo(const char* id, int* current, const char* const items[], int count,
                      float width, float indent = 0.0f)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *ImGui::GetCurrentContext();
        const ImGuiID iid = window->GetID(id);
        const ImVec2 pos = ImVec2(window->DC.CursorPos.x + indent, window->DC.CursorPos.y);
        const ImRect bb(pos, ImVec2(pos.x + width, pos.y + geo::FIELD_H));

        ImGui::ItemSize(ImVec2(width, geo::FIELD_H + 3 * geo::SCALE), 0.0f);
        if (!ImGui::ItemAdd(bb, iid)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, iid, &hovered, &held);
        if (pressed) ImGui::OpenPopup(id);

        ImDrawList* dl = window->DrawList;
        dl->AddRectFilled(bb.Min, bb.Max, col::FIELD);
        dl->AddRect(bb.Min, bb.Max, col::FIELD_BORDER);

        // текст значения
        const float pad = 6 * geo::SCALE;
        const ImVec2 ts = ImGui::CalcTextSize(items[*current]);
        dl->AddText(ImVec2(bb.Min.x + pad, bb.Min.y + (geo::FIELD_H - ts.y) * 0.5f),
                    col::TEXT, items[*current]);

        // шеврон: маленький треугольник справа
        const float cx = bb.Max.x - 9 * geo::SCALE;
        const float cy = bb.Min.y + geo::FIELD_H * 0.5f;
        const float r = 3.0f * geo::SCALE;
        dl->AddTriangleFilled(ImVec2(cx - r, cy - r * 0.4f), ImVec2(cx + r, cy - r * 0.4f),
                              ImVec2(cx, cy + r * 0.7f), col::TEXT_DIM);

        bool changed = false;
        ImGui::SetNextWindowPos(ImVec2(bb.Min.x, bb.Max.y));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(col::FIELD));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(col::FIELD_BORDER));
        if (ImGui::BeginPopup(id))
        {
            for (int i = 0; i < count; ++i)
            {
                const bool sel = (*current == i);
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::ColorConvertU32ToFloat4(
                    sel ? IM_COL32(60, 60, 60, 255) : col::FIELD));
                if (ImGui::Selectable(items[i], sel))
                {
                    *current = i;
                    changed = true;
                }
                ImGui::PopStyleColor();
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        return changed;
    }

    // ------------------------------------------------------------------
    // Секция колонки: фон #171717 + рамка #303030, буква A/B в углу
    // ------------------------------------------------------------------
    inline bool BeginColumn(const char* id, ImVec2 pos, ImVec2 size, const char* letter)
    {
        ImGui::SetCursorScreenPos(pos);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(col::BOX));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(geo::COL_PAD, geo::COL_PAD));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
        bool open = ImGui::BeginChild(id, size, ImGuiChildFlags_AlwaysUseWindowPadding,
                                      ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        // бордер колонки рисуем вручную — ImGui ChildBorderSize даёт не тот вид
        ImGui::GetWindowDrawList()->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                                            col::BOX_BORDER);

        if (open && letter)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddText(ImVec2(pos.x + 12 * geo::SCALE, pos.y + 6 * geo::SCALE),
                        IM_COL32(205, 205, 205, 255), letter);
            ImGui::Dummy(ImVec2(0, 12 * geo::SCALE));
        }
        return open;
    }

    inline void EndColumn()
    {
        ImGui::EndChild();
    }

    // ------------------------------------------------------------------
    // Кнопка сайдбара: квадрат 62px, иконка 38×38 по центру
    // Активная вкладка: фон #171717 + светлая иконка
    // ------------------------------------------------------------------
    inline bool SidebarButton(const char* id, Icon ic, bool selected, float size)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton(id, ImVec2(size, size));
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();

        if (selected)
            dl->AddRectFilled(p, ImVec2(p.x + size, p.y + size), col::ACTIVE_TAB);
        else if (hovered)
            dl->AddRectFilled(p, ImVec2(p.x + size, p.y + size), IM_COL32(18, 18, 18, 255));

        const ImU32 ic_col = selected ? col::ICON_ACTIVE : col::ICON;
        const ImVec2 c(p.x + size * 0.5f, p.y + size * 0.5f);
        DrawIcon(dl, ic, c, geo::ICON_SIZE, ic_col);
        return clicked;
    }

    // ------------------------------------------------------------------
    // Иконки: жирные силуэты, формы сняты с референса
    // ------------------------------------------------------------------
    inline void DrawIcon(ImDrawList* dl, Icon ic, ImVec2 c, float size, ImU32 color)
    {
        const float s = size * 0.5f;   // радиус
        const float lw = ImMax(1.0f, size * 0.055f);

        switch (ic)
        {
        case Icon::Headshot:
        {
            // череп в три четверти, смотрит влево-вниз (как на референсе):
            // купол, срезанная макушка, тёмный глаз, челюсть снизу-справа.
            const ImVec2 h(c.x - s * 0.26f, c.y - s * 0.02f);
            const float hr = s * 0.62f;

            // купол
            dl->AddCircleFilled(ImVec2(h.x, h.y), hr, color, 24);
            // плоский срез макушки сверху
            dl->AddRectFilled(ImVec2(h.x - hr * 0.30f, h.y - hr * 1.15f),
                              ImVec2(h.x + hr * 0.85f, h.y - hr * 0.72f),
                              IM_COL32(12, 12, 12, 255));
            // скула/нос слева
            dl->AddTriangleFilled(ImVec2(h.x - hr * 0.95f, h.y + hr * 0.05f),
                                  ImVec2(h.x - hr * 0.55f, h.y + hr * 0.55f),
                                  ImVec2(h.x - hr * 1.05f, h.y + hr * 0.62f), color);
            // челюсть уходит вправо-вниз
            dl->AddTriangleFilled(ImVec2(h.x + hr * 0.05f, h.y + hr * 0.35f),
                                  ImVec2(h.x + hr * 0.90f, h.y + hr * 0.30f),
                                  ImVec2(h.x + hr * 0.45f, h.y + hr * 1.05f), color);
            // тёмный глаз (крупный, смещён влево)
            dl->AddCircleFilled(ImVec2(h.x - hr * 0.28f, h.y - hr * 0.12f), hr * 0.30f,
                                IM_COL32(12, 12, 12, 255), 14);
            // носовой вырез
            dl->AddTriangleFilled(ImVec2(h.x - hr * 0.60f, h.y + hr * 0.22f),
                                  ImVec2(h.x - hr * 0.28f, h.y + hr * 0.18f),
                                  ImVec2(h.x - hr * 0.45f, h.y + hr * 0.52f),
                                  IM_COL32(12, 12, 12, 255));
            // рот-щель
            dl->AddRectFilled(ImVec2(h.x - hr * 0.50f, h.y + hr * 0.62f),
                              ImVec2(h.x + hr * 0.12f, h.y + hr * 0.78f),
                              IM_COL32(12, 12, 12, 255));
            // брызги вправо-вверх
            dl->AddCircleFilled(ImVec2(h.x + hr * 1.28f, h.y - hr * 0.62f), hr * 0.115f, color, 8);
            dl->AddCircleFilled(ImVec2(h.x + hr * 1.56f, h.y - hr * 0.92f), hr * 0.085f, color, 8);
            dl->AddCircleFilled(ImVec2(h.x + hr * 1.76f, h.y - hr * 1.14f), hr * 0.06f, color, 8);
            break;
        }
        case Icon::NoSpread:
        {
            // круг + перечёркнутый пистолет
            dl->AddCircle(c, s * 0.92f, color, 28, lw);
            const ImVec2 g(c.x - s * 0.1f, c.y + s * 0.05f);
            dl->AddRectFilled(ImVec2(g.x - s * 0.5f, g.y - s * 0.18f),
                              ImVec2(g.x + s * 0.4f, g.y + s * 0.1f), color);
            dl->AddRectFilled(ImVec2(g.x - s * 0.5f, g.y + s * 0.08f),
                              ImVec2(g.x - s * 0.22f, g.y + s * 0.62f), color);
            dl->AddLine(ImVec2(g.x - s * 0.55f, g.y - s * 0.5f),
                        ImVec2(g.x + s * 0.55f, g.y + s * 0.55f), IM_COL32(12, 12, 12, 255), lw * 2.2f);
            dl->AddLine(ImVec2(g.x - s * 0.55f, g.y - s * 0.5f),
                        ImVec2(g.x + s * 0.55f, g.y + s * 0.55f), color, lw);
            break;
        }
        case Icon::Crosshair:
        {
            // тонкий круг прицела с рисками
            dl->AddCircle(c, s * 0.88f, color, 32, lw);
            const float g = s * 0.22f;      // зазор в центре
            const float t = s * 0.5f;       // длина риски
            dl->AddLine(ImVec2(c.x - g, c.y), ImVec2(c.x - g - t, c.y), color, lw);
            dl->AddLine(ImVec2(c.x + g, c.y), ImVec2(c.x + g + t, c.y), color, lw);
            dl->AddLine(ImVec2(c.x, c.y - g), ImVec2(c.x, c.y - g - t), color, lw);
            dl->AddLine(ImVec2(c.x, c.y + g), ImVec2(c.x, c.y + g + t), color, lw);
            break;
        }
        case Icon::SunMoon:
        {
            // правая половина — залитый полумесяц, левая — лучи
            dl->AddCircleFilled(c, s * 0.5f, color, 24);
            dl->PathArcTo(c, s * 0.5f, IM_PI * 0.5f, IM_PI * 1.5f, 16);
            dl->PathStroke(IM_COL32(12, 12, 12, 255), 0, 0.0f);
            // лучи вокруг
            for (int i = 0; i < 8; ++i)
            {
                const float a = (float)i * IM_PI / 4.0f;
                const ImVec2 p1(c.x + cosf(a) * s * 0.72f, c.y + sinf(a) * s * 0.72f);
                const ImVec2 p2(c.x + cosf(a) * s * 0.98f, c.y + sinf(a) * s * 0.98f);
                dl->AddLine(p1, p2, color, lw);
            }
            // половина тоньше (разделительная линия)
            dl->AddLine(ImVec2(c.x, c.y - s * 0.5f), ImVec2(c.x, c.y + s * 0.5f),
                        IM_COL32(12, 12, 12, 255), lw * 1.2f);
            break;
        }
        case Icon::Gears:
        {
            // три шестерёнки разного размера
            auto gear = [&](ImVec2 gc, float r, float thick) {
                dl->AddCircleFilled(gc, r, color, 16);
                dl->AddCircleFilled(gc, r * 0.42f, IM_COL32(12, 12, 12, 255), 12);
                for (int i = 0; i < 8; ++i)
                {
                    const float a = (float)i * IM_PI / 4.0f;
                    const ImVec2 p1(gc.x + cosf(a) * r * 0.9f, gc.y + sinf(a) * r * 0.9f);
                    const ImVec2 p2(gc.x + cosf(a) * r * (0.9f + thick), gc.y + sinf(a) * r * (0.9f + thick));
                    dl->AddLine(p1, p2, color, r * 0.38f);
                }
            };
            gear(ImVec2(c.x - s * 0.3f, c.y + s * 0.3f), s * 0.42f, 0.22f);
            gear(ImVec2(c.x + s * 0.5f, c.y + s * 0.45f), s * 0.28f, 0.24f);
            gear(ImVec2(c.x + s * 0.35f, c.y - s * 0.42f), s * 0.3f, 0.22f);
            break;
        }
        case Icon::Knife:
        {
            // карамбит: толстая изогнутая дуга + кольцо на рукояти
            dl->AddBezierQuadratic(ImVec2(c.x - s * 1.00f, c.y + s * 0.62f),
                                   ImVec2(c.x - s * 0.30f, c.y - s * 1.05f),
                                   ImVec2(c.x + s * 0.78f, c.y - s * 0.05f),
                                   color, lw * 3.2f, 28);
            // утолщение у основания
            dl->AddCircleFilled(ImVec2(c.x - s * 0.95f, c.y + s * 0.58f), lw * 1.9f, color, 10);
            // кольцо
            dl->AddCircle(ImVec2(c.x + s * 0.80f, c.y + s * 0.12f), s * 0.19f, color, 14, lw * 1.7f);
            break;
        }
        case Icon::Player:
        {
            // голова + плечи заливкой
            dl->AddCircleFilled(ImVec2(c.x, c.y - s * 0.38f), s * 0.34f, color, 20);
            dl->PathClear();
            dl->PathArcTo(ImVec2(c.x, c.y + s * 0.95f), s * 0.72f, IM_PI, IM_PI * 2.0f, 20);
            dl->PathFillConvex(color);
            break;
        }
        case Icon::Floppy:
        {
            const float w2 = s * 0.92f, h2 = s * 0.92f;
            dl->AddRect(ImVec2(c.x - w2, c.y - h2), ImVec2(c.x + w2, c.y + h2), color, 0, 0, lw * 1.6f);
            // верхняя металлическая шторка
            dl->AddRectFilled(ImVec2(c.x - w2 * 0.45f, c.y - h2),
                              ImVec2(c.x + w2 * 0.28f, c.y - h2 * 0.30f), color);
            dl->AddRectFilled(ImVec2(c.x - w2 * 0.28f, c.y - h2),
                              ImVec2(c.x + w2 * 0.28f, c.y - h2 * 0.30f),
                              IM_COL32(12, 12, 12, 255));
            // нижняя наклейка
            dl->AddRectFilled(ImVec2(c.x - w2 * 0.58f, c.y + h2 * 0.18f),
                              ImVec2(c.x + w2 * 0.58f, c.y + h2 * 0.78f), color);
            break;
        }
        default:
            break;
        }
    }
}
