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
        DrawColumnFrame(c.pos, ImVec2(geo::COL_W * geo::SCALE, geo::COL_H * geo::SCALE), letter);
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
        CheckboxAbs(a.pos, a.y, "Silent aim", &s.silent, 10 * geo::SCALE);
        CheckboxAbs(a.pos, a.y, "Auto fire", &s.auto_fire, 10 * geo::SCALE);
        CheckboxAbs(a.pos, a.y, "Auto wall", &s.auto_wall, 10 * geo::SCALE);
        CheckboxAbs(a.pos, a.y, "Auto scope", &s.auto_scope, 10 * geo::SCALE);
        CheckboxAbs(a.pos, a.y, "Double tap", &s.double_tap, 10 * geo::SCALE);

        ColTo(a, 159);
        LabelAbs(a.pos, a.y, "Hitbox");
        ColTo(a, 174);
        ComboAbs(a.pos, a.y, "##hitbox", &s.hitbox, hitboxes, IM_ARRAYSIZE(hitboxes));

        ColTo(a, 204);
        LabelAbs(a.pos, a.y, "Target selection");
        ColTo(a, 219);
        ComboAbs(a.pos, a.y, "##target", &s.target, targets, IM_ARRAYSIZE(targets));

        ColTo(a, 249);
        LabelAbs(a.pos, a.y, "FOV");
        ColTo(a, 264);
        SliderAbs(a.pos, a.y, "##fov", &s.fov, 0.0f, 180.0f, "%.0f");

        ColTo(a, 294);
        LabelAbs(a.pos, a.y, "Smooth");
        ColTo(a, 309);
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
        CheckboxAbs(a.pos, a.y, "Jitter", &s.aa_jitter, 10 * geo::SCALE);
        CheckboxAbs(a.pos, a.y, "Desync", &s.aa_desync, 10 * geo::SCALE);
        ColTo(a, 123);
        LabelAbs(a.pos, a.y, "Pitch");
        ColTo(a, 138);
        ComboAbs(a.pos, a.y, "##aa_pitch", &s.aa_pitch, pitches, IM_ARRAYSIZE(pitches));
        ColTo(a, 174);
        LabelAbs(a.pos, a.y, "Yaw base");
        ColTo(a, 189);
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
        CheckboxAbs(a.pos, a.y, "Box", &s.box, 8 * geo::SCALE);
        CheckboxAbs(a.pos, a.y, "Name", &s.name, 8 * geo::SCALE);
        CheckboxAbs(a.pos, a.y, "Health", &s.health, 8 * geo::SCALE);
        CheckboxAbs(a.pos, a.y, "Weapon", &s.weapon, 8 * geo::SCALE);
        CheckboxAbs(a.pos, a.y, "Skeleton", &s.skeleton, 8 * geo::SCALE);
        ColTo(a, 159);
        LabelAbs(a.pos, a.y, "Box style");
        ColTo(a, 174);
        ComboAbs(a.pos, a.y, "##esp_style", &s.esp_style, styles, IM_ARRAYSIZE(styles));

        ColCtx b = BeginCol(geo::COL_X_B, "B");
        ColTo(b, 51);
        CheckboxAbs(b.pos, b.y, "Chams", &s.chams);
        CheckboxAbs(b.pos, b.y, "Glow", &s.glow, 8 * geo::SCALE);
        CheckboxAbs(b.pos, b.y, "Radar", &s.radar, 8 * geo::SCALE);
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
        CheckboxAbs(a.pos, a.y, "Auto strafe", &s.auto_strafe, 8 * geo::SCALE);
        CheckboxAbs(a.pos, a.y, "Edge jump", &s.edge_jump, 8 * geo::SCALE);
        CheckboxAbs(a.pos, a.y, "Auto peek", &s.auto_peek, 8 * geo::SCALE);
        ColTo(a, 123);
        LabelAbs(a.pos, a.y, "Fake lag");
        ColTo(a, 141);
        CheckboxAbs(a.pos, a.y, "Enable", &s.fake_lag);

        ColCtx b = BeginCol(geo::COL_X_B, "B");
        ColTo(b, 51);
        CheckboxAbs(b.pos, b.y, "Watermark", &s.watermark);
        CheckboxAbs(b.pos, b.y, "Debug console", &s.debug_console, 8 * geo::SCALE);
        CheckboxAbs(b.pos, b.y, "Save on exit", &s.save_on_exit, 8 * geo::SCALE);
    }

    inline void TabSettings(Settings& s)
    {
        static const char* accents[] = { "Green", "Blue", "Red", "Purple", "Orange" };

        ColCtx a = BeginCol(geo::COL_X_A, "A");
        ColTo(a, 51);
        CheckboxAbs(a.pos, a.y, "Watermark", &s.watermark);
        CheckboxAbs(a.pos, a.y, "Debug console", &s.debug_console, 8 * geo::SCALE);
        CheckboxAbs(a.pos, a.y, "Save on exit", &s.save_on_exit, 8 * geo::SCALE);
        ColTo(a, 105);
        LabelAbs(a.pos, a.y, "Accent");
        ColTo(a, 120);
        ComboAbs(a.pos, a.y, "##accent", &s.accent, accents, IM_ARRAYSIZE(accents));

        ColCtx b = BeginCol(geo::COL_X_B, "B");
        ColTo(b, 51);
        LabelAbs(b.pos, b.y, "Config name");
        CheckboxAbs(b.pos, b.y, "Load on start", &s.lua_autorun);
        ColTo(b, 105);
        LabelAbs(b.pos, b.y, "Menu key");
        ColTo(b, 123);
        CheckboxAbs(b.pos, b.y, "Insert", &s.menu_open);
    }

    inline void TabSkins(Settings& s)
    {
        static const char* models[]  = { "cuddleleader", "models/player/ctm_gsg9", "custom" };

        ColCtx a = BeginCol(geo::COL_X_A, "A");
        ColTo(a, 51);
        CheckboxAbs(a.pos, a.y, "Replace weapon models", &s.replace_weapon_models);
        CheckboxAbs(a.pos, a.y, "Replace player models", &s.replace_player_models);
        ColTo(a, 88);
        LabelAbs(a.pos, a.y, "T model");
        ColTo(a, 89);
        ComboAbs(a.pos, a.y, "##tmodel", &s.t_model, models, IM_ARRAYSIZE(models));
        ColTo(a, 128);
        LabelAbs(a.pos, a.y, "CT model");
        ColTo(a, 129);
        ComboAbs(a.pos, a.y, "##ctmodel", &s.ct_model, models, IM_ARRAYSIZE(models));
        ColTo(a, 169);
        CheckboxAbs(a.pos, a.y, "Replace localplayer model", &s.replace_localplayer_model);
        ColTo(a, 187);
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
        CheckboxAbs(a.pos, a.y, "Mark as friend", &s.friend_mark, 8 * geo::SCALE);

        ColCtx b = BeginCol(geo::COL_X_B, "B");
        ColTo(b, 51);
        CheckboxAbs(b.pos, b.y, "Show friends only", &s.friends_only);
    }

    inline void TabLua(Settings& s)
    {
        ColCtx a = BeginCol(geo::COL_X_A, "A");
        ColTo(a, 51);
        CheckboxAbs(a.pos, a.y, "Autorun scripts", &s.lua_autorun);
        CheckboxAbs(a.pos, a.y, "Show errors", &s.show_errors, 8 * geo::SCALE);

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

        // (дизеринг рисуется растеризатором как фон окна)

        // --- рамка: 1px #3C3C3C, 3px #282828, 1px #3C3C3C ---
        const float fw = geo::FRAME_W, fh = geo::FRAME_H;
        dl->AddRect(ImVec2(wp.x, wp.y), ImVec2(wp.x + W - 1, wp.y + H - 1), col::FRAME_OUT);
        dl->AddRect(ImVec2(wp.x + fw, wp.y + fw), ImVec2(wp.x + W - fw - 1, wp.y + H - fw - 1),
                    col::FRAME_IN, 0.0f, 0, 2.0f * fh);

        // --- радуга y=7..8: верхняя половина светлая, нижняя тёмная ---
        {
            const float y0 = wp.y + geo::RAINBOW_Y * K;
            const float y1 = y0 + geo::RAINBOW_H * K;
            const int segs = 200;
            for (int i = 0; i < segs; ++i)
            {
                const float t0 = (float)i / segs, t1 = (float)(i + 1) / segs;
                auto hue = [](float t) -> ImU32 {
                    // (136,119,211) -> (204,226,54) с переходом через розовый
                    auto lerp = [](float a, float b, float u) { return (int)(a + (b - a) * u); };
                    int r, g, b;
                    if (t < 0.33f)      { const float u = t / 0.33f;         r = lerp(136, 204, u); g = lerp(119,  71, u); b = lerp(211, 205, u); }
                    else if (t < 0.66f) { const float u = (t - 0.33f) / 0.33f; r = lerp(204, 204, u); g = lerp( 71, 174, u); b = lerp(205, 104, u); }
                    else                { const float u = (t - 0.66f) / 0.34f; r = lerp(204, 204, u); g = lerp(174, 226, u); b = lerp(104,  54, u); }
                    return IM_COL32(r, g, b, 255);
                };
                const float x0 = wp.x + W * t0, x1 = wp.x + W * t1 + 1;
                dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y0 + geo::RAINBOW_H * K * 0.5f), hue(t0));
                const ImU32 c = hue(t0);
                dl->AddRectFilled(ImVec2(x0, y0 + geo::RAINBOW_H * K * 0.5f), ImVec2(x1, y1),
                                  IM_COL32((c >> 0 & 0xff) / 2 + 6, (c >> 8 & 0xff) / 2 + 6,
                                           (c >> 16 & 0xff) / 2 + 6, 255));
            }
        }

        // --- сайдбар: внутренняя рамка иконок x 0..80, снизу до y 530 ---
        const float sbx1 = 80 * K;
        dl->AddLine(ImVec2(wp.x + sbx1, wp.y + 11 * K), ImVec2(wp.x + sbx1, wp.y + 530 * K),
                    col::BOX_BORDER);
        // разделители подписи
        dl->AddLine(ImVec2(wp.x + 6 * K, wp.y + 531 * K), ImVec2(wp.x + 80 * K, wp.y + 531 * K),
                    col::BOX_BORDER);
        dl->AddLine(ImVec2(wp.x + 6 * K, wp.y + 596 * K), ImVec2(wp.x + 80 * K, wp.y + 596 * K),
                    col::BOX_BORDER);
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
            const float iy = wp.y + (geo::ICON_Y0 + geo::ICON_STEP * i) * K;
            const float ix = wp.x + geo::ICON_X * K;
            const ImVec2 p(ix, iy);
            const float sz = geo::ICON_SIZE * K;

            ImGui::SetCursorScreenPos(p);
            char id[16];
            snprintf(id, sizeof(id), "##sb%d", i);
            ImGui::InvisibleButton(id, ImVec2(sz, sz));
            const bool hovered = ImGui::IsItemHovered();
            const bool clicked = ImGui::IsItemClicked();

            const bool selected = (s.active_tab == i);
            if (selected)
            {
                // активная вкладка — светлее иконка и лёгкая подсветка
                dl->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), IM_COL32(24, 24, 24, 255));
            }
            const ImU32 tint = selected ? col::ICON_ACTIVE
                                        : (hovered ? col::ICON_HOVER : col::ICON);
            DrawIcon(dl, icons[i], ImVec2(p.x + sz * 0.5f, p.y + sz * 0.5f), sz, tint);

            if (clicked)
                s.active_tab = i;
        }

        // подпись активной вкладки внизу сайдбара
        {
            const char* nm = active_name(s.active_tab);
            dl->AddText(ImGui::GetFont(), 16.0f * K,
                        ImVec2(wp.x + 26 * K, wp.y + 556.0f * K),
                        col::TEXT_WHITE, nm);
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
