// gui_widgets.h — кастомные контролы: чекбокс с квадратом, дропдаун, иконки, колонки A/B.
#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "gui_theme.h"
#include <cmath>
#include <string>
#include <vector>

namespace gui
{
    // ---- иконки из встроенного шрифта (без внешних файлов) ----
    enum class Icon
    {
        Aim = 1, Visuals, Misc, Settings, Skins, Profile, Lua, Close, Dot, Chevron, Gear, Save
    };

    inline void DrawIcon(ImDrawList* dl, Icon icon, ImVec2 c, ImU32 color, float r);

    // Маленький чекбокс в стиле референса: тёмный квадрат, зелёная заливка при включении.
    inline bool Checkbox(const char* label, bool* v)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);

        const float square = geo::CHECK_SIZE;
        const ImVec2 pos = window->DC.CursorPos;
        const ImRect total_bb(pos, ImVec2(pos.x + square + 8 + label_size.x,
                                          pos.y + ImMax(square, label_size.y)));
        ImGui::ItemSize(total_bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(total_bb, id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
        if (pressed)
        {
            *v = !(*v);
            ImGui::MarkItemEdited(id);
        }

        ImDrawList* dl = window->DrawList;
        const ImVec2 sq_min = ImVec2(pos.x, pos.y + (label_size.y - square) * 0.5f);
        const ImVec2 sq_max = ImVec2(sq_min.x + square, sq_min.y + square);

        // как на референсе: включённый — сплошной зелёный, выключенный — тёмный квадрат с рамкой
        if (*v)
        {
            dl->AddRectFilled(sq_min, sq_max, col::ACCENT);
            dl->AddRect(sq_min, sq_max, col::ACCENT);
        }
        else
        {
            dl->AddRectFilled(sq_min, sq_max, hovered ? IM_COL32(58, 58, 66, 255) : IM_COL32(40, 40, 46, 255));
            dl->AddRect(sq_min, sq_max, IM_COL32(72, 72, 80, 255));
        }

        dl->AddText(ImVec2(sq_max.x + 8, pos.y),
                    *v ? col::TEXT_ACTIVE : col::TEXT, label);

        return pressed;
    }

    // Дропдаун с шевроном, ширина фиксированная как на референсе.
    inline bool Combo(const char* label, int* current, const char* const items[], int count,
                      float width = 150.0f)
    {
        ImGui::PushID(label);
        bool changed = false;
        ImGui::SetNextItemWidth(width);
        if (ImGui::BeginCombo("##c", items[*current], ImGuiComboFlags_HeightSmall))
        {
            for (int i = 0; i < count; ++i)
            {
                const bool sel = (i == *current);
                if (ImGui::Selectable(items[i], sel))
                {
                    *current = i;
                    changed = true;
                }
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
        return changed;
    }

    // Группа-колонка с буквенным заголовком A / B (как на референсе).
    inline bool BeginColumn(const char* id, ImVec2 size)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(col::BOX));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(geo::GROUP_PAD, geo::GROUP_PAD));
        bool open = ImGui::BeginChild(id, size, ImGuiChildFlags_Borders);
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        return open;
    }

    inline void EndColumn()
    {
        ImGui::EndChild();
    }

    // Квадратная кнопка сайдбара с иконкой; возвращает true при клике.
    inline bool SidebarButton(const char* id, Icon icon, bool selected, const char* tooltip)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float size = geo::SIDEBAR_W - 12.0f * geo::SCALE;
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton(id, ImVec2(size, size));
        const bool hovered = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();

        if (selected || hovered)
        {
            dl->AddRectFilled(p, ImVec2(p.x + size, p.y + size),
                              selected ? IM_COL32(34, 34, 40, 255) : IM_COL32(28, 28, 33, 255));
        }
        if (selected)
        {
            dl->AddRectFilled(p, ImVec2(p.x + 2.0f * geo::SCALE, p.y + size), col::ACCENT);
        }

        const ImVec2 c = ImVec2(p.x + size * 0.5f, p.y + size * 0.5f);
        DrawIcon(dl, icon, c, selected ? col::TEXT_ACTIVE : col::TEXT_DIM, size * 0.24f);

        if (hovered && tooltip)
            ImGui::SetTooltip("%s", tooltip);

        ImGui::Dummy(ImVec2(0, 2));
        return clicked;
    }

    // Векторные иконки — рисуем примитивами, чтобы не тащить icon-шрифт.
    inline void DrawIcon(ImDrawList* dl, Icon icon, ImVec2 c, ImU32 color, float r)
    {
        switch (icon)
        {
        case Icon::Aim: // прицел
            dl->AddCircle(c, r, color, 24, 1.6f);
            dl->AddLine(ImVec2(c.x - r - 3, c.y), ImVec2(c.x - r + 2, c.y), color, 1.6f);
            dl->AddLine(ImVec2(c.x + r - 2, c.y), ImVec2(c.x + r + 3, c.y), color, 1.6f);
            dl->AddLine(ImVec2(c.x, c.y - r - 3), ImVec2(c.x, c.y - r + 2), color, 1.6f);
            dl->AddLine(ImVec2(c.x, c.y + r - 2), ImVec2(c.x, c.y + r + 3), color, 1.6f);
            break;
        case Icon::Visuals: // глаз
            dl->AddBezierQuadratic(ImVec2(c.x - r - 2, c.y), ImVec2(c.x, c.y - r),
                                   ImVec2(c.x + r + 2, c.y), color, 1.6f, 16);
            dl->AddBezierQuadratic(ImVec2(c.x - r - 2, c.y), ImVec2(c.x, c.y + r),
                                   ImVec2(c.x + r + 2, c.y), color, 1.6f, 16);
            dl->AddCircleFilled(c, r * 0.35f, color);
            break;
        case Icon::Misc: // солнце
            dl->AddCircle(c, r * 0.55f, color, 16, 1.6f);
            for (int i = 0; i < 8; ++i)
            {
                const float a = (float)i * IM_PI / 4.0f;
                const ImVec2 p1(c.x + cosf(a) * r * 0.8f, c.y + sinf(a) * r * 0.8f);
                const ImVec2 p2(c.x + cosf(a) * r, c.y + sinf(a) * r);
                dl->AddLine(p1, p2, color, 1.6f);
            }
            break;
        case Icon::Settings: // шестерёнка
        {
            dl->AddCircle(c, r * 0.55f, color, 16, 1.6f);
            for (int i = 0; i < 6; ++i)
            {
                const float a = (float)i * IM_PI / 3.0f;
                const ImVec2 p1(c.x + cosf(a) * r * 0.6f, c.y + sinf(a) * r * 0.6f);
                const ImVec2 p2(c.x + cosf(a) * r, c.y + sinf(a) * r);
                dl->AddLine(p1, p2, color, 2.4f);
            }
            break;
        }
        case Icon::Skins: // нож
            dl->AddBezierQuadratic(ImVec2(c.x - r, c.y + r * 0.6f),
                                   ImVec2(c.x, c.y - r * 0.9f),
                                   ImVec2(c.x + r, c.y - r * 0.2f), color, 1.8f, 12);
            dl->AddLine(ImVec2(c.x - r, c.y + r * 0.6f),
                        ImVec2(c.x - r * 0.4f, c.y + r), color, 2.2f);
            break;
        case Icon::Profile: // голова/плечи
            dl->AddCircle(ImVec2(c.x, c.y - r * 0.35f), r * 0.36f, color, 16, 1.6f);
            dl->AddBezierQuadratic(ImVec2(c.x - r * 0.75f, c.y + r * 0.85f),
                                   ImVec2(c.x, c.y + r * 0.05f),
                                   ImVec2(c.x + r * 0.75f, c.y + r * 0.85f), color, 1.6f, 12);
            break;
        case Icon::Lua: // скрипт
            dl->AddRect(ImVec2(c.x - r * 0.7f, c.y - r), ImVec2(c.x + r * 0.7f, c.y + r), color, 0, 0, 1.4f);
            dl->AddLine(ImVec2(c.x - r * 0.4f, c.y - r * 0.35f), ImVec2(c.x + r * 0.4f, c.y - r * 0.35f), color, 1.3f);
            dl->AddLine(ImVec2(c.x - r * 0.4f, c.y + r * 0.1f), ImVec2(c.x + r * 0.4f, c.y + r * 0.1f), color, 1.3f);
            break;
        case Icon::Close: // крестик
            dl->AddLine(ImVec2(c.x - r * 0.7f, c.y - r * 0.7f), ImVec2(c.x + r * 0.7f, c.y + r * 0.7f), color, 1.6f);
            dl->AddLine(ImVec2(c.x + r * 0.7f, c.y - r * 0.7f), ImVec2(c.x - r * 0.7f, c.y + r * 0.7f), color, 1.6f);
            break;
        default:
            break;
        }
    }
}
