// gui_theme.h — стиль меню в духе gamesense/skeet
// Тёмная палитра, левый иконочный сайдбар, плоские чекбоксы, h-ориентированные табы.
#pragma once

#include "imgui.h"

namespace gui
{
    // Палитра (снята с референса: почти чёрный фон, серые акценты, зелёный чек).
    namespace col
    {
        constexpr ImU32 BG          = IM_COL32( 22,  22,  25, 255); // фон имени/окна
        constexpr ImU32 PANEL       = IM_COL32( 26,  26,  30, 255); // тело меню
        constexpr ImU32 SIDEBAR     = IM_COL32( 18,  18,  21, 255); // левая полоса
        constexpr ImU32 BOX         = IM_COL32( 32,  32,  37, 255); // колонки A/B
        constexpr ImU32 BOX_BORDER  = IM_COL32( 44,  44,  50, 255);
        constexpr ImU32 SEPARATOR   = IM_COL32( 40,  40,  46, 255);
        constexpr ImU32 TEXT        = IM_COL32(196, 196, 202, 255);
        constexpr ImU32 TEXT_DIM    = IM_COL32(120, 120, 128, 255);
        constexpr ImU32 TEXT_ACTIVE = IM_COL32(235, 235, 240, 255);
        constexpr ImU32 ACCENT      = IM_COL32(102, 187,  65, 255); // зелёный чек
        constexpr ImU32 ACCENT_DIM  = IM_COL32( 70, 120,  48, 255);
        constexpr ImU32 CHECK_BOX   = IM_COL32( 46,  46,  52, 255);
        constexpr ImU32 SELECT_BG   = IM_COL32( 40,  40,  46, 255);
        constexpr ImU32 SELECT_HOVER= IM_COL32( 52,  52,  60, 255);
    }

    // Геометрия. Значения задаются через SetScale(), чтобы можно было
    // рендерить в любом разрешении без размытия (шрифт грузится в том же масштабе).
    namespace geo
    {
        inline float SCALE      = 1.0f;
        inline float SIDEBAR_W  = 58.0f;
        inline float HEADER_H   = 28.0f;
        inline float PAD        = 10.0f;
        inline float GROUP_PAD  = 16.0f;
        inline float ITEM_H     = 18.0f;
        inline float CHECK_SIZE = 8.0f;
        inline float FONT_SIZE  = 13.0f;
        inline float TAB_H      = 22.0f;

        inline void SetScale(float s)
        {
            SCALE      = s;
            SIDEBAR_W  = 58.0f * s;
            HEADER_H   = 28.0f * s;
            PAD        = 10.0f * s;
            GROUP_PAD  = 16.0f * s;
            ITEM_H     = 18.0f * s;
            CHECK_SIZE = 8.0f * s;
            FONT_SIZE  = 13.0f * s;
            TAB_H      = 22.0f * s;
        }
    }

    inline void Apply()
    {
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding    = 0.0f;
        s.ChildRounding     = 0.0f;
        s.FrameRounding     = 0.0f;
        s.PopupRounding     = 0.0f;
        s.ScrollbarRounding = 0.0f;
        s.GrabRounding      = 0.0f;
        s.WindowBorderSize  = 1.0f;
        s.ChildBorderSize   = 1.0f;
        s.FrameBorderSize   = 1.0f;
        s.WindowPadding     = ImVec2(geo::PAD, geo::PAD);
        s.FramePadding      = ImVec2(6 * geo::SCALE, 3 * geo::SCALE);
        s.ItemSpacing       = ImVec2(6, 5);
        s.ItemInnerSpacing  = ImVec2(6, 4);
        s.IndentSpacing     = 14.0f;
        s.ScrollbarSize     = 8.0f;

        ImVec4* c = s.Colors;
        c[ImGuiCol_WindowBg]             = ImVec4(26/255.f, 26/255.f, 30/255.f, 1.00f);
        c[ImGuiCol_ChildBg]              = ImVec4(32/255.f, 32/255.f, 37/255.f, 1.00f);
        c[ImGuiCol_PopupBg]              = ImVec4(24/255.f, 24/255.f, 28/255.f, 1.00f);
        c[ImGuiCol_Border]               = ImVec4(44/255.f, 44/255.f, 50/255.f, 1.00f);
        c[ImGuiCol_Text]                 = ImVec4(196/255.f,196/255.f,202/255.f,1.00f);
        c[ImGuiCol_TextDisabled]         = ImVec4(120/255.f,120/255.f,128/255.f,1.00f);
        c[ImGuiCol_FrameBg]              = ImVec4(38/255.f, 38/255.f, 44/255.f, 1.00f);
        c[ImGuiCol_FrameBgHovered]       = ImVec4(48/255.f, 48/255.f, 56/255.f, 1.00f);
        c[ImGuiCol_FrameBgActive]        = ImVec4(40/255.f, 40/255.f, 47/255.f, 1.00f);
        c[ImGuiCol_TitleBg]              = ImVec4(18/255.f, 18/255.f, 21/255.f, 1.00f);
        c[ImGuiCol_TitleBgActive]        = ImVec4(22/255.f, 22/255.f, 25/255.f, 1.00f);
        c[ImGuiCol_MenuBarBg]            = ImVec4(22/255.f, 22/255.f, 25/255.f, 1.00f);
        c[ImGuiCol_ScrollbarBg]          = ImVec4(22/255.f, 22/255.f, 25/255.f, 1.00f);
        c[ImGuiCol_ScrollbarGrab]        = ImVec4(46/255.f, 46/255.f, 52/255.f, 1.00f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(60/255.f, 60/255.f, 68/255.f, 1.00f);
        c[ImGuiCol_CheckMark]            = ImVec4(102/255.f,187/255.f, 65/255.f, 1.00f);
        c[ImGuiCol_SliderGrab]           = ImVec4(102/255.f,187/255.f, 65/255.f, 1.00f);
        c[ImGuiCol_SliderGrabActive]     = ImVec4(120/255.f,205/255.f, 80/255.f, 1.00f);
        c[ImGuiCol_Button]               = ImVec4(38/255.f, 38/255.f, 44/255.f, 1.00f);
        c[ImGuiCol_ButtonHovered]        = ImVec4(50/255.f, 50/255.f, 58/255.f, 1.00f);
        c[ImGuiCol_ButtonActive]         = ImVec4(44/255.f, 44/255.f, 52/255.f, 1.00f);
        c[ImGuiCol_Header]               = ImVec4(44/255.f, 44/255.f, 52/255.f, 1.00f);
        c[ImGuiCol_HeaderHovered]        = ImVec4(54/255.f, 54/255.f, 62/255.f, 1.00f);
        c[ImGuiCol_HeaderActive]         = ImVec4(60/255.f, 60/255.f, 70/255.f, 1.00f);
        c[ImGuiCol_Separator]            = ImVec4(40/255.f, 40/255.f, 46/255.f, 1.00f);
        c[ImGuiCol_FrameBg]              = ImVec4(38/255.f, 38/255.f, 44/255.f, 1.00f);
    }
}
