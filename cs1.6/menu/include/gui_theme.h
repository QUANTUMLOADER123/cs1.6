// gui_theme.h — точный порт палитры и геометрии gamesense (skeet.cc).
//
// Все значения сняты пиксельным анализом реального скриншота меню:
//   фон окна           #0C0C0C  (12,12,12)
//   колонки A/B        #171717  (23,23,23)
//   бордер колонок     #303030  (48,48,48)
//   рамка окна         #3C3C3C / #282828 (60,60,60 / 40,40,40)
//   текст              #CDCDCD  (205,205,205)
//   текст дим          #5A5A5A  (90,90,90)
//   акцент (галка)     #95C021  (149,192,33)
//   поле дропдауна     #242424  (36,36,36)
//   рамка дропдауна    #0C0C0C  (12,12,12)
//   радужная полоса    (91,151,215) -> (204,213,66), 2px, полная ширина
//   чекбокс            6×6 px
//   иконки сайдбара    ~38×38 px (крупные, жирные заливки)
//   сайдбар            88 px
//   каждая колонка     290 px
#pragma once

#include "imgui.h"

namespace gui
{
    namespace col
    {
        constexpr ImU32 BG          = IM_COL32( 12,  12,  12, 255); // фон окна
        constexpr ImU32 PANEL       = IM_COL32( 12,  12,  12, 255);
        constexpr ImU32 SIDEBAR     = IM_COL32( 12,  12,  12, 255);
        constexpr ImU32 BOX         = IM_COL32( 23,  23,  23, 255); // колонки A/B
        constexpr ImU32 BOX_BORDER  = IM_COL32( 48,  48,  48, 255);
        constexpr ImU32 FRAME_OUT   = IM_COL32( 60,  60,  60, 255); // внешняя рамка
        constexpr ImU32 FRAME_IN    = IM_COL32( 40,  40,  40, 255); // вторая рамка
        constexpr ImU32 SEPARATOR   = IM_COL32( 40,  40,  40, 255);
        constexpr ImU32 TEXT        = IM_COL32(205, 205, 205, 255);
        constexpr ImU32 TEXT_DIM    = IM_COL32( 90,  90,  90, 255);
        constexpr ImU32 TEXT_ACTIVE = IM_COL32(230, 230, 230, 255);
        constexpr ImU32 ACCENT      = IM_COL32(149, 192,  33, 255); // galka
        constexpr ImU32 ACCENT_2    = IM_COL32(124, 158,  32, 255); // второй акцент
        constexpr ImU32 CHECK_BOX   = IM_COL32( 71,  71,  71, 255); // незаполненный чекбокс
        constexpr ImU32 CHECK_BORDER= IM_COL32(  0,   0,   0, 255);
        constexpr ImU32 FIELD       = IM_COL32( 36,  36,  36, 255); // поле дропдауна
        constexpr ImU32 FIELD_BORDER= IM_COL32( 12,  12,  12, 255);
        constexpr ImU32 ICON        = IM_COL32( 90,  90,  90, 255); // неактивная иконка
        constexpr ImU32 ICON_ACTIVE = IM_COL32(205, 205, 205, 255);
        constexpr ImU32 ACTIVE_TAB  = IM_COL32( 23,  23,  23, 255); // фон активной вкладки
    }

    // Геометрия в логических пикселях (1× = 731×617 как на референсе).
    namespace geo
    {
        inline float SCALE      = 1.0f;

        inline float WINDOW_W   = 731.0f;
        inline float WINDOW_H   = 617.0f;
        inline float SIDEBAR_W  = 88.0f;
        inline float FRAME      = 1.0f;
        inline float COL_W      = 290.0f;
        inline float COL_GAP    = 22.0f;
        inline float COL_PAD    = 8.0f;
        inline float WIN_PAD_TOP= 9.0f;
        inline float WIN_PAD_L  = 101.0f;
        inline float CHECK_SIZE = 6.0f;
        inline float ITEM_H     = 14.0f;
        inline float TEXT_SIZE  = 9.0f;
        inline float LABEL_SIZE = 8.0f;
        inline float RAINBOW_H  = 2.0f;
        inline float ICON_SIZE  = 38.0f;
        inline float ICON_STEP  = 62.0f;
        inline float FIELD_H    = 13.0f;

        inline void SetScale(float s)
        {
            SCALE       = s;
            WINDOW_W    = 731.0f * s;
            WINDOW_H    = 617.0f * s;
            SIDEBAR_W   = 88.0f  * s;
            FRAME       = 1.0f   * s;
            COL_W       = 290.0f * s;
            COL_GAP     = 22.0f  * s;
            COL_PAD     = 8.0f   * s;
            WIN_PAD_TOP = 9.0f   * s;
            WIN_PAD_L   = 101.0f * s;
            CHECK_SIZE  = 6.0f   * s;
            ITEM_H      = 14.0f  * s;
            TEXT_SIZE   = 9.0f   * s;
            LABEL_SIZE  = 8.0f   * s;
            RAINBOW_H   = 2.0f   * s;
            ICON_SIZE   = 38.0f  * s;
            ICON_STEP   = 62.0f  * s;
            FIELD_H     = 13.0f  * s;
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
        s.WindowBorderSize  = 0.0f;
        s.ChildBorderSize   = 0.0f;
        s.FrameBorderSize   = 0.0f;
        s.WindowPadding     = ImVec2(0, 0);
        s.FramePadding      = ImVec2(4 * geo::SCALE, 2 * geo::SCALE);
        s.ItemSpacing       = ImVec2(0, 3 * geo::SCALE);
        s.ItemInnerSpacing  = ImVec2(4 * geo::SCALE, 2 * geo::SCALE);
        s.IndentSpacing     = 10 * geo::SCALE;
        s.ScrollbarSize     = 6 * geo::SCALE;

        ImVec4* c = s.Colors;
        auto C = [](ImU32 v) { return ImGui::ColorConvertU32ToFloat4(v); };
        c[ImGuiCol_WindowBg]             = C(col::BG);
        c[ImGuiCol_ChildBg]              = C(col::BOX);
        c[ImGuiCol_PopupBg]              = C(col::BOX);
        c[ImGuiCol_Border]               = C(col::BOX_BORDER);
        c[ImGuiCol_Text]                 = C(col::TEXT);
        c[ImGuiCol_TextDisabled]         = C(col::TEXT_DIM);
        c[ImGuiCol_FrameBg]              = C(col::FIELD);
        c[ImGuiCol_FrameBgHovered]       = C(IM_COL32(44, 44, 44, 255));
        c[ImGuiCol_FrameBgActive]        = C(IM_COL32(40, 40, 40, 255));
        c[ImGuiCol_TitleBg]              = C(col::BG);
        c[ImGuiCol_TitleBgActive]        = C(col::BG);
        c[ImGuiCol_CheckMark]            = C(col::ACCENT);
        c[ImGuiCol_SliderGrab]           = C(col::ACCENT);
        c[ImGuiCol_SliderGrabActive]     = C(IM_COL32(171, 216, 47, 255));
        c[ImGuiCol_Button]               = C(col::FIELD);
        c[ImGuiCol_ButtonHovered]        = C(IM_COL32(48, 48, 48, 255));
        c[ImGuiCol_ButtonActive]         = C(IM_COL32(40, 40, 40, 255));
        c[ImGuiCol_Header]               = C(col::FIELD);
        c[ImGuiCol_HeaderHovered]        = C(IM_COL32(48, 48, 48, 255));
        c[ImGuiCol_HeaderActive]         = C(IM_COL32(52, 52, 52, 255));
        c[ImGuiCol_Separator]            = C(col::SEPARATOR);
        c[ImGuiCol_ScrollbarBg]          = C(col::BG);
        c[ImGuiCol_ScrollbarGrab]        = C(IM_COL32(45, 45, 45, 255));
        c[ImGuiCol_ScrollbarGrabHovered] = C(IM_COL32(65, 65, 65, 255));
    }
}
