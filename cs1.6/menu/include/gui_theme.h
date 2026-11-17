// gui_theme.h — ТОЧНЫЙ порт палитры и геометрии gamesense (skeet).
//
// Все значения сняты попиксельным анализом реального скриншота меню 731x617
// (image-search/gamesense-skeet-csgo-cheat-menu-gui-1.png).
//
// Вертикальная структура окна (y, 1:1):
//    0        bg #0C0C0C (внешний 1px)
//    1        #3C3C3C  (60,60,60)
//    2..4     #282828  (40,40,40)   — 3px
//    5        #3C3C3C
//    6        #0C0C0C
//    7..8     радуга 2px (136,119,211) сверху -> тёмная половина
//    9        #060606
//   10        #0C0C0C
//   11..30    шахматный дизеринг: #0C0C0C / #141414 (20,20,20)
//   31        верхняя граница колонок #303030 (48,48,48)
//   32..591   тело колонок #171717 (23,23,23)
//   591       нижняя граница колонок (48,48,48)
//   592..610  дизеринг
//   611..615   рамка #3C3C3C / #282828 / #282828 / #282828 / #3C3C3C
#pragma once

#include "imgui.h"

namespace gui
{
    namespace col
    {
        constexpr ImU32 BG          = IM_COL32( 12,  12,  12, 255);
        constexpr ImU32 DITHER      = IM_COL32( 20,  20,  20, 255); // шахматный фон
        constexpr ImU32 BOX         = IM_COL32( 23,  23,  23, 255); // тело колонок A/B
        constexpr ImU32 BOX_BORDER  = IM_COL32( 48,  48,  48, 255); // рамка колонок
        constexpr ImU32 FRAME_OUT   = IM_COL32( 60,  60,  60, 255);
        constexpr ImU32 FRAME_IN    = IM_COL32( 40,  40,  40, 255);
        constexpr ImU32 SEPARATOR   = IM_COL32( 48,  48,  48, 255);
        constexpr ImU32 TEXT        = IM_COL32(205, 205, 205, 255);
        constexpr ImU32 TEXT_DIM    = IM_COL32( 90,  90,  90, 255);
        constexpr ImU32 TEXT_WHITE  = IM_COL32(245, 245, 245, 255);
        constexpr ImU32 ACCENT      = IM_COL32(149, 192,  33, 255);
        constexpr ImU32 CHECK_OFF   = IM_COL32( 71,  71,  71, 255);
        constexpr ImU32 CHECK_BORDER= IM_COL32( 12,  12,  12, 255);
        // поле дропдауна: вертикальный градиент 31 -> 36
        constexpr ImU32 FIELD_TOP   = IM_COL32( 31,  31,  31, 255);
        constexpr ImU32 FIELD_BOT   = IM_COL32( 36,  36,  36, 255);
        constexpr ImU32 FIELD_GLYPH = IM_COL32(205, 205, 205, 255); // текст дропдауна
        constexpr ImU32 CHEVRON     = IM_COL32(152, 152, 152, 255);
        constexpr ImU32 ICON        = IM_COL32( 90,  90,  90, 255); // неактивная иконка
        constexpr ImU32 ICON_ACTIVE = IM_COL32( 90,  90,  90, 255); // в REF активная НЕ ярче
        constexpr ImU32 ICON_HOVER  = IM_COL32(150, 150, 150, 255);
    }

    // Геометрия в логических пикселях (1x = ровно 731x617, как в референсе).
    namespace geo
    {
        inline float SCALE      = 1.0f;

        inline float WINDOW_W   = 731.0f;
        inline float WINDOW_H   = 617.0f;

        inline float FRAME_W    = 1.0f;    // #3C3C3C по краю
        inline float FRAME_H    = 3.0f;    // #282828 под ним
        inline float RAINBOW_H  = 2.0f;    // y 7..8
        inline float RAINBOW_Y  = 7.0f;

        inline float SIDEBAR_W  = 97.0f;   // 0..96, x=97 — начало зазора
        inline float SIDEBAR_BORDER = 48.0f; // цвет рамки сайдбара

        inline float COL_X_A    = 101.0f;  // рамка колонки A
        inline float COL_X_B    = 415.0f;  // рамка колонки B
        inline float COL_W      = 292.0f;  // 101..392 включительно (рамка + 290 тела)
        inline float COL_Y      = 31.0f;
        inline float COL_H      = 560.0f;  // 31..590 (591 — нижняя граница)
        inline float COL_PAD    = 10.0f;   // контент от x=111 / y=41

        inline float ICON_SIZE  = 46.0f;   // ширина (совпала с REF 1:1)
        inline float ICON_STEP  = 64.08f;  // шаг по Y (центры 50.7, 114.8 ... 499.3)
        inline float ICON_X     = 43.7f;   // центр иконки по X
        // Вертикальный сжим иконок: в REF высота ink на 2-3px МЕНЬШЕ ширины
        // (спрайт 48x48, а ink 38x34). Подобрано по 8 иконкам.
        inline float ICON_ASPECT = 0.945f;
        inline float ICON_Y0    = 53.2f;   // центр ПЕРВОЙ иконки по Y

        inline float CHECK_SIZE = 6.0f;
        inline float CHECK_X    = 123.0f;  // абсолютный x чекбокса (123..128)
        inline float LABEL_X    = 143.0f;  // абсолютный x текста после чекбокса
        inline float FIELD_X    = 153.0f;  // абсолютный x дропдауна/поля
        inline float ITEM_H     = 18.0f;   // шаг строк (52,70,88,...)
        inline float CONTENT_Y0 = 53.0f;  // ink-top первой строки
        inline float TEXT_SIZE  = 8.0f;   // Verdana Bold 8px (как в gamesense)
        inline float FIELD_H    = 20.0f;   // 101..120

        // позиция окна на экране (ставится в Render)
        inline float WINDOW_X   = 0.0f;
        inline float WINDOW_Y   = 0.0f;

        inline float CAPTION_Y  = 557.0f;  // подпись вкладки (Lua)
        inline float CAPTION_H  = 40.0f;   // 596 - 557 + ...

        inline void SetScale(float s)
        {
            SCALE           = s;
            WINDOW_W        = 731.0f * s;
            WINDOW_H        = 617.0f * s;
            FRAME_W         = 1.0f   * s;
            FRAME_H         = 3.0f   * s;
            RAINBOW_H       = 2.0f   * s;
            RAINBOW_Y       = 7.0f   * s;
            SIDEBAR_W       = 97.0f  * s;
            SIDEBAR_BORDER  = 48.0f  * s;
            COL_X_A         = 101.0f * s;
            COL_X_B         = 415.0f * s;
            COL_W           = 292.0f * s;
            COL_Y           = 31.0f  * s;
            COL_H           = 560.0f * s;
            COL_PAD         = 10.0f  * s;
            ICON_SIZE       = 46.0f   * s;
            ICON_STEP       = 64.08f  * s;
            ICON_X          = 42.2f   * s;
            ICON_Y0         = 52.1f   * s;
            CHECK_SIZE      = 6.0f   * s;
            CHECK_X         = 122.0f * s;
            LABEL_X         = 143.0f * s;
            FIELD_X         = 153.0f * s;
            ITEM_H          = 18.0f  * s;
            CONTENT_Y0      = 53.0f  * s;
            TEXT_SIZE       = 11.0f   * s;
            FIELD_H         = 20.0f  * s;
            CAPTION_Y       = 557.0f * s;
            CAPTION_H       = 40.0f  * s;
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
        s.ItemSpacing       = ImVec2(0, 0);
        s.ItemInnerSpacing  = ImVec2(4 * geo::SCALE, 0);
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
        c[ImGuiCol_FrameBg]              = C(col::FIELD_TOP);
        c[ImGuiCol_FrameBgHovered]       = C(col::FIELD_TOP);
        c[ImGuiCol_FrameBgActive]        = C(col::FIELD_TOP);
        c[ImGuiCol_TitleBg]              = C(col::BG);
        c[ImGuiCol_TitleBgActive]        = C(col::BG);
        c[ImGuiCol_CheckMark]            = C(col::ACCENT);
        c[ImGuiCol_SliderGrab]           = C(col::ACCENT);
        c[ImGuiCol_SliderGrabActive]     = C(IM_COL32(171, 216, 47, 255));
        c[ImGuiCol_Button]               = C(col::FIELD_TOP);
        c[ImGuiCol_ButtonHovered]        = C(IM_COL32(48, 48, 48, 255));
        c[ImGuiCol_ButtonActive]         = C(IM_COL32(40, 40, 40, 255));
        c[ImGuiCol_Header]               = C(col::FIELD_TOP);
        c[ImGuiCol_HeaderHovered]        = C(IM_COL32(48, 48, 48, 255));
        c[ImGuiCol_HeaderActive]         = C(IM_COL32(52, 52, 52, 255));
        c[ImGuiCol_Separator]            = C(col::SEPARATOR);
        c[ImGuiCol_ScrollbarBg]          = C(col::BG);
        c[ImGuiCol_ScrollbarGrab]        = C(IM_COL32(45, 45, 45, 255));
        c[ImGuiCol_ScrollbarGrabHovered] = C(IM_COL32(65, 65, 65, 255));
    }
        // Подпись активной вкладки рисуется поверх дизеринга, поэтому её
        // содержимое и позицию запоминаем на время кадра.
        inline const char*& sidebar_label()
        {
            static const char* v = "";
            return v;
        }
        inline ImVec2& sidebar_label_pos()
        {
            static ImVec2 v(0.0f, 0.0f);
            return v;
        }
}
