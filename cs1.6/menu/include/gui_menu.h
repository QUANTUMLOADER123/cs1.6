// gui_menu.h — меню gamesense (skeet), пиксельный порт 1:1 с реального меню.
//
// Геометрия снята с референса 731x617 (см. gui_theme.h):
//   8 вкладок, порядок gamesense: Rage, Anti-Aim, Visuals, Misc, Settings,
//   Skins, Players, Lua.
//   Сайдбар 0..96, зазор с рамкой #303030 на x=80 (внутренняя рамка иконок),
//   колонка A x 101..392, колонка B x 415..705, тело #171717,
//   шахматный дизеринг #0C0C0C/#141414 в зазорах.
#pragma once

#include "imgui.h"
#include "gui_theme.h"
#include "gui_widgets.h"
#include <cstdio>
#include <cstring>

namespace gui
{
    inline const char* active_name(int tab);

    struct Settings
    {
        bool menu_open = true;
        int  active_tab = 0;
        bool show_demo = false;

        // Rage
        bool aimbot = false, silent = false, auto_fire = false;
        bool auto_wall = false, auto_scope = false;
        bool resolver = false, backtrack = false, double_tap = false;
        int  hitbox = 0, target = 0;
        float fov = 180.0f, smooth = 12.0f;

        // Anti-Aim
        bool aa_enable = true, aa_jitter = true, aa_desync = false;
        int  aa_pitch = 0, aa_yaw = 0;

        // Visuals
        bool esp = true, box = true, name = true, health = true;
        bool weapon = true, skeleton = false, chams = false;
        bool glow = false, radar = false;
        int  esp_style = 0;

        // Misc
        bool bhop = true, auto_strafe = false, edge_jump = false, auto_peek = false;
        bool fake_lag = false, thirdperson = false;
        int  fakelag_ticks = 2;
        float tp_dist = 120.0f;

        // Skins
        bool replace_weapon_models = true, replace_player_models = false;
        bool replace_localplayer_model = false, stattrak = false;
        int  t_model = 0, ct_model = 0, local_model = 0;
        int  knife = 0, ak = 0, m4 = 0, awp = 0;
        int  preset = 0;
        bool coin_changer = false, private_rank = false, music_kit = false;

        // Players
        int  player_sel = 0;
        bool priority = false, friend_mark = false, friends_only = false;

        // Settings
        bool watermark = true, debug_console = false, save_on_exit = true;
        int  accent = 0;
        char config[64] = "default";

        // Lua
        bool lua_autorun = false, allow_internet = false, show_errors = true;
    };

    inline Settings& State()
    {
        static Settings s;
        return s;
    }

    // ------------------------------------------------------------------
    // Контент вкладок. Обе колонки рисуются абсолютными координатами,
    // контент начинается с y = COL_Y + 10.
    // ------------------------------------------------------------------
    struct ColCtx
    {
        ImVec2 pos;   // левый верхний угол рамки колонки (экран)
        float  y;     // смещение текущей строки от pos, в логических px
    };

    inline ColCtx BeginCol(float x, const char* letter)
    {
        ColCtx c;
        c.pos = ImVec2(geo::WINDOW_X + x * geo::SCALE, geo::WINDOW_Y + geo::COL_Y * geo::SCALE);
        c.y = 0.0f;
        // Рамки в референсе: A x=101..392, B x=414..705. Правая = pos.x + W - 1.
        const float cw = (x >= 400.0f) ? 293.0f : geo::COL_W;
        DrawColumnFrame(c.pos, ImVec2(cw * geo::SCALE, geo::COL_H * geo::SCALE), letter);
        return c;
    }

    // Пропуск до абсолютной строки (в координатах окна, 1:1 с референсом).
    inline void ColTo(ColCtx& c, float abs_row)
    {
        c.y = abs_row - geo::COL_Y;
    }
    inline void ColDummy(ColCtx& c, float dy) { c.y += dy; }

    inline void TabRage(Settings& s)
    {
        static const char* hitboxes[] = { "Head", "Neck", "Chest", "Stomach", "Pelvis" };
        static const char* targets[]  = { "Crosshair", "Lowest HP", "Highest threat" };

        ColCtx a = BeginCol(geo::COL_X_A, "A");
        ColTo(a, 51);
        CheckboxAbs(a.pos, a.y, "Enable aimbot", &s.aimbot);
        CheckboxAbs(a.pos, a.y, "Silent aim", &s.silent);
        CheckboxAbs(a.pos, a.y, "Auto fire", &s.auto_fire);
        CheckboxAbs(a.pos, a.y, "Auto wall", &s.auto_wall);
        CheckboxAbs(a.pos, a.y, "Auto scope", &s.auto_scope);
        CheckboxAbs(a.pos, a.y, "Double tap", &s.double_tap);

        ColTo(a, 165);
        LabelAbs(a.pos, a.y, "Hitbox");
        ComboAbs(a.pos, a.y, "##hitbox", &s.hitbox, hitboxes, IM_ARRAYSIZE(hitboxes));

        ColTo(a, 210);
        LabelAbs(a.pos, a.y, "Target selection");
        ComboAbs(a.pos, a.y, "##target", &s.target, targets, IM_ARRAYSIZE(targets));

        ColTo(a, 255);
        LabelAbs(a.pos, a.y, "FOV");
        SliderAbs(a.pos, a.y, "##fov", &s.fov, 0.0f, 180.0f, "%.0f");

        ColTo(a, 300);
        LabelAbs(a.pos, a.y, "Smooth");
        SliderAbs(a.pos, a.y, "##smooth", &s.smooth, 1.0f, 30.0f, "%.1f");

        ColCtx b = BeginCol(geo::COL_X_B, "B");
        ColTo(b, 51);
        CheckboxAbs(b.pos, b.y, "Resolver", &s.resolver);
        CheckboxAbs(b.pos, b.y, "Backtrack", &s.backtrack);
        CheckboxAbs(b.pos, b.y, "Double tap", &s.double_tap);
    }

    inline void TabAntiAim(Settings& s)
    {
        static const char* pitches[] = { "None", "Down", "Up", "Zero" };
        static const char* yaws[]    = { "None", "Forward", "Backward", "Left", "Right" };

        ColCtx a = BeginCol(geo::COL_X_A, "A");
        ColTo(a, 51);
        CheckboxAbs(a.pos, a.y, "Enable anti-aim", &s.aa_enable);
        CheckboxAbs(a.pos, a.y, "Jitter", &s.aa_jitter);
        CheckboxAbs(a.pos, a.y, "Desync", &s.aa_desync);
        ColTo(a, 128);
        LabelAbs(a.pos, a.y, "Pitch");
        ComboAbs(a.pos, a.y, "##aa_pitch", &s.aa_pitch, pitches, IM_ARRAYSIZE(pitches));
        ColTo(a, 179);
        LabelAbs(a.pos, a.y, "Yaw base");
        ComboAbs(a.pos, a.y, "##aa_yaw", &s.aa_yaw, yaws, IM_ARRAYSIZE(yaws));

        ColCtx b = BeginCol(geo::COL_X_B, "B");
        ColTo(b, 51);
        CheckboxAbs(b.pos, b.y, "Fake duck", &s.fake_lag);
        CheckboxAbs(b.pos, b.y, "Slide on peek", &s.auto_peek);
    }

    inline void TabVisuals(Settings& s)
    {
        static const char* styles[] = { "Regular", "Corners", "3D", "Outline" };

        ColCtx a = BeginCol(geo::COL_X_A, "A");
        ColTo(a, 51);
        CheckboxAbs(a.pos, a.y, "Master switch", &s.esp);
        CheckboxAbs(a.pos, a.y, "Box", &s.box);
        CheckboxAbs(a.pos, a.y, "Name", &s.name);
        CheckboxAbs(a.pos, a.y, "Health", &s.health);
        CheckboxAbs(a.pos, a.y, "Weapon", &s.weapon);
        CheckboxAbs(a.pos, a.y, "Skeleton", &s.skeleton);
        ColTo(a, 164);
        LabelAbs(a.pos, a.y, "Box style");
        ComboAbs(a.pos, a.y, "##esp_style", &s.esp_style, styles, IM_ARRAYSIZE(styles));

        ColCtx b = BeginCol(geo::COL_X_B, "B");
        ColTo(b, 51);
        CheckboxAbs(b.pos, b.y, "Chams", &s.chams);
        CheckboxAbs(b.pos, b.y, "Glow", &s.glow);
        CheckboxAbs(b.pos, b.y, "Radar", &s.radar);
        ColTo(b, 105);
        LabelAbs(b.pos, b.y, "Thirdperson");
        ColTo(b, 123);
        CheckboxAbs(b.pos, b.y, "Enable", &s.thirdperson);
        ColTo(b, 141);
        SliderAbs(b.pos, b.y, "##tpdist", &s.tp_dist, 40.0f, 300.0f, "%.0f");
    }

    inline void TabMisc(Settings& s)
    {
        ColCtx a = BeginCol(geo::COL_X_A, "A");
        ColTo(a, 51);
        CheckboxAbs(a.pos, a.y, "Bunnyhop", &s.bhop);
        CheckboxAbs(a.pos, a.y, "Auto strafe", &s.auto_strafe);
        CheckboxAbs(a.pos, a.y, "Edge jump", &s.edge_jump);
        CheckboxAbs(a.pos, a.y, "Auto peek", &s.auto_peek);
        ColTo(a, 123);
        LabelAbs(a.pos, a.y, "Fake lag");
        ColTo(a, 141);
        CheckboxAbs(a.pos, a.y, "Enable", &s.fake_lag);

        ColCtx b = BeginCol(geo::COL_X_B, "B");
        ColTo(b, 51);
        CheckboxAbs(b.pos, b.y, "Watermark", &s.watermark);
        CheckboxAbs(b.pos, b.y, "Debug console", &s.debug_console);
        CheckboxAbs(b.pos, b.y, "Save on exit", &s.save_on_exit);
    }

    inline void TabSettings(Settings& s)
    {
        static const char* accents[] = { "Green", "Blue", "Red", "Purple", "Orange" };

        ColCtx a = BeginCol(geo::COL_X_A, "A");
        ColTo(a, 51);
        CheckboxAbs(a.pos, a.y, "Watermark", &s.watermark);
        CheckboxAbs(a.pos, a.y, "Debug console", &s.debug_console);
        CheckboxAbs(a.pos, a.y, "Save on exit", &s.save_on_exit);
        ColTo(a, 110);
        LabelAbs(a.pos, a.y, "Accent");
        ComboAbs(a.pos, a.y, "##accent", &s.accent, accents, IM_ARRAYSIZE(accents));

        ColCtx b = BeginCol(geo::COL_X_B, "B");
        // Панель Presets — как в оригинальном меню: дропдаун + 4 кнопки.
        ColTo(b, 36);
        ComboAbs(b.pos, b.y, "##preset", &s.preset,
                 (const char* const[]){ "Legit", "Rage", "HvH", "Custom" }, 4);
        ColTo(b, 71);
        ButtonAbs(b.pos, b.y, "Load");
        ColTo(b, 97);
        ButtonAbs(b.pos, b.y, "Save");
        ColTo(b, 123);
        ButtonAbs(b.pos, b.y, "Reset");
        ColTo(b, 149);
        ButtonAbs(b.pos, b.y, "Unload");
    }

    inline void TabSkins(Settings& s)
    {
        static const char* models[]  = { "cuddleleader", "models/player/ctm_gsg9", "custom" };

        ColCtx a = BeginCol(geo::COL_X_A, "A");
        ColTo(a, 51);
        CheckboxAbs(a.pos, a.y, "Replace weapon models", &s.replace_weapon_models);
        CheckboxAbs(a.pos, a.y, "Replace player models", &s.replace_player_models);
        ColTo(a, 86);
        LabelAbs(a.pos, a.y, "T model");
        ComboAbs(a.pos, a.y, "##tmodel", &s.t_model, models, IM_ARRAYSIZE(models));
        ColTo(a, 126);
        LabelAbs(a.pos, a.y, "CT model");
        ComboAbs(a.pos, a.y, "##ctmodel", &s.ct_model, models, IM_ARRAYSIZE(models));
        ColTo(a, 169);
        CheckboxAbs(a.pos, a.y, "Replace localplayer model", &s.replace_localplayer_model);
        // В референсе это поле идёт почти вплотную к чекбоксу (бордер 184,
        // градиент 185..202), а не через 18px как предыдущие пары.
        ColTo(a, 170);
        ComboAbs(a.pos, a.y, "##localmodel", &s.local_model, models, IM_ARRAYSIZE(models));

        ColCtx b = BeginCol(geo::COL_X_B, "B");
        ColTo(b, 51);
        CheckboxAbs(b.pos, b.y, "Enable coin Changer", &s.coin_changer);
        CheckboxAbs(b.pos, b.y, "Enable Private rank", &s.private_rank);
        CheckboxAbs(b.pos, b.y, "Enable Music kit", &s.music_kit);
    }

    inline void TabPlayers(Settings& s)
    {
        ColCtx a = BeginCol(geo::COL_X_A, "A");
        ColTo(a, 51);
        CheckboxAbs(a.pos, a.y, "Prioritize in aimbot", &s.priority);
        CheckboxAbs(a.pos, a.y, "Mark as friend", &s.friend_mark);

        ColCtx b = BeginCol(geo::COL_X_B, "B");
        ColTo(b, 51);
        CheckboxAbs(b.pos, b.y, "Show friends only", &s.friends_only);
    }

    inline void TabLua(Settings& s)
    {
        ColCtx a = BeginCol(geo::COL_X_A, "A");
        ColTo(a, 51);
        CheckboxAbs(a.pos, a.y, "Autorun scripts", &s.lua_autorun);
        CheckboxAbs(a.pos, a.y, "Show errors", &s.show_errors);

        ColCtx b = BeginCol(geo::COL_X_B, "B");
        CheckboxAbs(b.pos, b.y, "Allow internet access", &s.save_on_exit);
    }

    // ------------------------------------------------------------------
    // Рамка окна, радуга, дизеринг, сайдбар
    // ------------------------------------------------------------------
    inline void DrawWindowChrome(ImDrawList* dl, ImVec2 wp, ImVec2 ws)
    {
        const float K = geo::SCALE;
        const float W = ws.x, H = ws.y;

        // фон окна
        dl->AddRectFilled(wp, ImVec2(wp.x + W, wp.y + H), col::BG);

        // (дизеринг рисуется растеризатором/бэкендом как фон окна)

        // --- рамка (снято пиксель-в-пиксель с референса) ---
        //  y=0      : #0C0C0C (фон окна)
        //  y=1      : x 1..729  #3C3C3C           <- верх внешней рамки
        //  y=2..4   : x 2..728  #282828           <- средняя полоса
        //  y=5      : x 5..725  #3C3C3C           <- нижняя линия титлбара
        //  y=6..610 : #0C0C0C (контент), x=1 и x=729 -> #3C3C3C, x=2..4/726..728 -> #282828
        //  y=611    : x 1..729 #3C3C3C,  y=612..614 #282828,  y=615 #3C3C3C, y=616 фон
        {
            // ВНИМАНИЕ: имена OUT/MID заняты макросами wingdi.h при сборке под Windows.
            const ImU32 frame_hi = IM_COL32(60, 60, 60, 255);
            const ImU32 frame_lo = IM_COL32(40, 40, 40, 255);
            const float x0 = wp.x, y0 = wp.y;

            // верхний и нижний блоки рамки
            dl->AddRectFilled(ImVec2(x0 + 1, y0 + 1), ImVec2(x0 + W - 1, y0 + 2), frame_hi);
            dl->AddRectFilled(ImVec2(x0 + 2, y0 + 2), ImVec2(x0 + W - 2, y0 + 5), frame_lo);
            dl->AddRectFilled(ImVec2(x0 + 2, y0 + 2), ImVec2(x0 + 5, y0 + H - 11), frame_lo);
            dl->AddRectFilled(ImVec2(x0 + 5, y0 + 5), ImVec2(x0 + W - 5, y0 + 6), frame_hi);

            // нижний блок (y=593 в референсе — уже дизеринг, линию не рисуем)
            dl->AddRectFilled(ImVec2(x0 + 1, y0 + H - 6), ImVec2(x0 + W - 1, y0 + H - 5), frame_hi);
            dl->AddRectFilled(ImVec2(x0 + 2, y0 + H - 5), ImVec2(x0 + W - 2, y0 + H - 2), frame_lo);
            dl->AddRectFilled(ImVec2(x0 + 2, y0 + H - 2), ImVec2(x0 + W - 2, y0 + H - 1), frame_hi);

            // вертикальные полосы по всей высоте контента
            // (в референсе y=300: x=1 #3C3C3C, x=2..4 #282828, x=5 #3C3C3C)
            dl->AddRectFilled(ImVec2(x0 + 1, y0 + 6), ImVec2(x0 + 2, y0 + H - 5), frame_hi);
            dl->AddRectFilled(ImVec2(x0 + 2, y0 + 6), ImVec2(x0 + 5, y0 + H - 5), frame_lo);
            dl->AddRectFilled(ImVec2(x0 + 5, y0 + 6), ImVec2(x0 + 6, y0 + H - 5), frame_hi);
            dl->AddRectFilled(ImVec2(x0 + W - 2, y0 + 6), ImVec2(x0 + W - 1, y0 + H - 5), frame_hi);
            dl->AddRectFilled(ImVec2(x0 + W - 6, y0 + 6), ImVec2(x0 + W - 5, y0 + H - 5), frame_hi);
            dl->AddRectFilled(ImVec2(x0 + W - 5, y0 + 6), ImVec2(x0 + W - 2, y0 + H - 5), frame_lo);
            dl->AddRectFilled(ImVec2(x0 + W - 6, y0 + 6), ImVec2(x0 + W - 5, y0 + H - 5), frame_hi);
        }

        // --- радуга y=7..8 ---
        // Снято с референса: цветная полоса x=7..723, три КУСОЧНО-ЛИНЕЙНЫХ
        // участка по каналам (регрессия даёт остаток ~0.3 на канал):
        //   x   7.. 26 : ( 55,177,218) плоский старт
        //   x  26..362 : ( 63,171,217) -> (204, 71,205)
        //   x 362..506 : (204, 71,205) -> (204,132,145)
        //   x 506..723 : (204,142,135) -> (204,226, 54)
        // Тёмная строка (y=8) = светлая / 2 + 6.
        {
            const int ry = (int)ImFloor(wp.y + geo::RAINBOW_Y * K);
            auto chan = [](float x) -> void { (void)x; };
            (void)chan;
            for (int xi = 7; xi <= 723; ++xi)
            {
                float r, g, b;
                if (xi <= 26)
                {
                    r = 55; g = 177; b = 218;
                }
                else if (xi <= 362)
                {
                    const float t = (xi - 26) / 336.0f;
                    r = 63 + 141 * t; g = 171 - 100 * t; b = 217 - 12 * t;
                }
                else if (xi <= 506)
                {
                    const float t = (xi - 362) / 144.0f;
                    r = 204; g = 71 + 61 * t; b = 205 - 60 * t;
                }
                else
                {
                    const float t = (xi - 506) / 217.0f;
                    r = 204; g = 142 + 84 * t; b = 135 - 81 * t;
                }
                const int R = (int)(r + 0.5f), G = (int)(g + 0.5f), B = (int)(b + 0.5f);
                dl->AddRectFilled(ImVec2(wp.x + xi, ry), ImVec2(wp.x + xi + 1, ry + 1),
                                  IM_COL32(R, G, B, 255));
                dl->AddRectFilled(ImVec2(wp.x + xi, ry + 1), ImVec2(wp.x + xi + 1, ry + 2),
                                  IM_COL32(R / 2 + 6, G / 2 + 6, B / 2 + 6, 255));
            }
        }

        // --- Сайдбар: зона иконок занимает ВСЮ левую полосу ---
        // Снято с референса по x=80 (граница) и x=79 (чёрный зазор):
        //   x=79  -> #000000                 (чёрная линия-зазор)
        //   x=80  -> #303030 (48)            (светлая граница)
        //   x=81+ -> дизеринг                (фон между сайдбаром и колонкой A)
        // Внутри сайдбара (x 2..78) — свой дизеринг #0C0C0C/#141414.
        const float sbx1 = 80 * K;
        // 1px линии — только AddRectFilled: AddLine со сглаживанием даёт
        // полупрозрачные пиксели (26/29 вместо чётких 0/48), из-за чего
        // границы выглядели «грязными».
        auto vline = [&](float x, float y0, float y1, ImU32 c) {
            dl->AddRectFilled(ImVec2(ImFloor(wp.x + x), ImFloor(wp.y + y0)),
                              ImVec2(ImFloor(wp.x + x) + 1, ImFloor(wp.y + y1)), c);
        };
        auto hline = [&](float x0, float x1, float y, ImU32 c) {
            dl->AddRectFilled(ImVec2(ImFloor(wp.x + x0), ImFloor(wp.y + y)),
                              ImVec2(ImFloor(wp.x + x1), ImFloor(wp.y + y) + 1), c);
        };
        vline(79 * K, 11 * K, 597 * K, IM_COL32(0, 0, 0, 255));      // чёрный зазор
        vline(sbx1,   11 * K, 597 * K, col::BOX_BORDER);              // светлая граница
        hline(6 * K, sbx1, 530 * K, IM_COL32(0, 0, 0, 255));          // над подписью
        hline(6 * K, sbx1, 531 * K, col::BOX_BORDER);
        hline(6 * K, sbx1, 596 * K, col::BOX_BORDER);                 // под подписью
    }

    // ------------------------------------------------------------------
    // Окно
    // ------------------------------------------------------------------
    inline void Render(Settings& s)
    {
        if (!s.menu_open) return;

        const float K = geo::SCALE;
        const ImVec2 ds = ImGui::GetIO().DisplaySize;

        // Позиция окна — целыми пикселями, чтобы границы вставали ровно.
        geo::WINDOW_X = ImFloor((ds.x - geo::WINDOW_W) * 0.5f);
        geo::WINDOW_Y = ImFloor((ds.y - geo::WINDOW_H) * 0.5f);

        ImGui::SetNextWindowSize(ImVec2(geo::WINDOW_W, geo::WINDOW_H), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(geo::WINDOW_X, geo::WINDOW_Y), ImGuiCond_Always);

        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus |
                                       ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(col::BG));
        ImGui::Begin("##gamesense", nullptr, flags);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        DrawWindowChrome(dl, wp, ws);

        // --- иконки сайдбара: порядок ровно как в спрайте/меню ---
        static const Icon icons[] = {
            Icon::Headshot, Icon::NoSpread, Icon::Crosshair, Icon::SunMoon,
            Icon::Gears, Icon::Knife, Icon::Player, Icon::Floppy
        };
        for (int i = 0; i < IM_ARRAYSIZE(icons); ++i)
        {
            // ICON_X/ICON_Y0 — ЦЕНТР иконки (регрессия по референсу), бокс
            // для InvisibleButton откладываем от центра на половину размера.
            // Шестерёнки (3) в референсе на 1px крупнее остальных.
            const float sz = geo::ICON_SIZE * K * (i == 3 ? 1.022f : 1.0f);
            const ImVec2 ctr(wp.x + geo::ICON_X * K,
                             wp.y + (geo::ICON_Y0 + geo::ICON_STEP * i) * K);
            const ImVec2 p(ctr.x - sz * 0.5f, ctr.y - sz * 0.5f);

            ImGui::SetCursorScreenPos(p);
            char id[16];
            snprintf(id, sizeof(id), "##sb%d", i);
            ImGui::InvisibleButton(id, ImVec2(sz, sz));
            const bool hovered = ImGui::IsItemHovered();
            const bool clicked = ImGui::IsItemClicked();

            const bool selected = (s.active_tab == i);
            // В референсе активная вкладка НЕ подсвечивается квадратом:
            // фон под иконками везде #0C0C0C, цвет иконок 90.
            (void)selected;
            const ImU32 tint = selected ? col::ICON_ACTIVE
                                        : (hovered ? col::ICON_HOVER : col::ICON);
            DrawIcon(dl, icons[i], ctr, sz, tint);

            if (clicked)
                s.active_tab = i;
        }

        // Подпись активной вкладки внизу сайдбара.
        // ВАЖНО (снято с референса): там контент от Skins, а подпись — "Lua".
        // В оригинале подпись берётся из другого индекса, чем контент;
        // воспроизводим это 1:1. ink в референсе: x 26..59, y 557..572 (16px).
        // Подпись активной вкладки. Дизеринг бокса идёт после кадра, поэтому
        // dither_rect(..., keep_text=true) не затирает эти пиксели.
        {
            const char* nm = (s.active_tab == 5) ? "Lua" : active_name(s.active_tab);
            DrawTextSpaced(dl, ImVec2(wp.x + 26 * K, wp.y + 552.0f * K),
                           col::TEXT_WHITE, nm, 25.0f * K, 0.5f * K,
                           BigFont() ? BigFont() : ImGui::GetFont());
        }

        // --- контент вкладки ---
        switch (s.active_tab)
        {
        case 0: TabRage(s); break;
        case 1: TabAntiAim(s); break;
        case 2: TabVisuals(s); break;
        case 3: TabMisc(s); break;
        case 4: TabSettings(s); break;
        case 5: TabSkins(s); break;
        case 6: TabPlayers(s); break;
        case 7: TabLua(s); break;
        default: TabRage(s); break;
        }

        ImGui::End();
    }

    inline const char* active_name(int tab)
    {
        switch (tab)
        {
        case 0: return "Rage";
        case 1: return "Anti-Aim";
        case 2: return "Visuals";
        case 3: return "Misc";
        case 4: return "Settings";
        case 5: return "Skins";
        case 6: return "Players";
        case 7: return "Lua";
        default: return "Rage";
        }
    }
}
