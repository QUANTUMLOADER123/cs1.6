// gui_menu.h — меню gamesense, геометрия 1в1 с референса 731×617.
//
// Раскладка (все значения сняты с пикселей):
//   окно          731×617, фон #0C0C0C
//   рамка         1px #3C3C3C + 2px #282828 + 1px #3C3C3C
//   радуга        2px по всей ширине под рамкой
//   сайдбар       88px, иконки 62px шаг, первая на y≈30
//   колонка A     x 101, ширина 290
//   колонка B     x 415, ширина 290
//   подпись вкладки внизу сайдбара, 14px, белая
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
        int knife = 0, ak = 0, m4 = 0, awp = 0;
        bool stattrak = false;

        // Settings
        bool watermark = true, debug_console = false, save_on_exit = true;
        int  accent = 0;
        char config[64] = "default";
    };

    inline Settings& State()
    {
        static Settings s;
        return s;
    }

    // ------------------------------------------------------------------
    // Контент вкладок
    // ------------------------------------------------------------------

    inline void TabRage(Settings& s, float w, float h)
    {
        static const char* hitboxes[] = { "Head", "Neck", "Chest", "Stomach", "Pelvis" };
        static const char* targets[]  = { "Crosshair", "Lowest HP", "Highest threat" };

        ImVec2 base = ImGui::GetCursorScreenPos();
        ImVec2 csz(geo::COL_W, h);

        if (BeginColumn("##rage_a", base, csz, "A"))
        {
            Checkbox("Enable aimbot", &s.aimbot);
            Checkbox("Silent aim", &s.silent, 8 * geo::SCALE);
            Checkbox("Auto fire", &s.auto_fire, 8 * geo::SCALE);
            Checkbox("Auto wall", &s.auto_wall, 8 * geo::SCALE);
            Checkbox("Auto scope", &s.auto_scope, 8 * geo::SCALE);
            Checkbox("Double tap", &s.double_tap, 8 * geo::SCALE);

            ImGui::Dummy(ImVec2(0, 6 * geo::SCALE));
            Label("Hitbox");
            Combo("##hitbox", &s.hitbox, hitboxes, IM_ARRAYSIZE(hitboxes), geo::COL_W - 2 * geo::COL_PAD);

            Label("Target selection");
            Combo("##target", &s.target, targets, IM_ARRAYSIZE(targets), geo::COL_W - 2 * geo::COL_PAD);

            ImGui::Dummy(ImVec2(0, 6 * geo::SCALE));
            Label("FOV");
            ImGui::SetNextItemWidth(geo::COL_W - 2 * geo::COL_PAD);
            ImGui::SliderFloat("##fov", &s.fov, 0.0f, 180.0f, "%.0f");
            Label("Smooth");
            ImGui::SetNextItemWidth(geo::COL_W - 2 * geo::COL_PAD);
            ImGui::SliderFloat("##smooth", &s.smooth, 1.0f, 30.0f, "%.1f");
        }
        EndColumn();

        ImGui::SetCursorScreenPos(ImVec2(base.x + geo::COL_W + geo::COL_GAP, base.y));
        if (BeginColumn("##rage_b", ImVec2(base.x + geo::COL_W + geo::COL_GAP, base.y), csz, "B"))
        {
            Checkbox("Resolver", &s.resolver);
            Checkbox("Backtrack", &s.backtrack);
        }
        EndColumn();
    }

    inline void TabVisuals(Settings& s, float w, float h)
    {
        static const char* styles[] = { "Regular", "Corners", "3D", "Outline" };
        ImVec2 base = ImGui::GetCursorScreenPos();
        ImVec2 csz(geo::COL_W, h);

        if (BeginColumn("##vis_a", base, csz, "A"))
        {
            Checkbox("Master switch", &s.esp);
            Checkbox("Box", &s.box, 8 * geo::SCALE);
            Checkbox("Name", &s.name, 8 * geo::SCALE);
            Checkbox("Health", &s.health, 8 * geo::SCALE);
            Checkbox("Weapon", &s.weapon, 8 * geo::SCALE);
            Checkbox("Skeleton", &s.skeleton, 8 * geo::SCALE);
            Checkbox("Chams", &s.chams, 8 * geo::SCALE);

            ImGui::Dummy(ImVec2(0, 6 * geo::SCALE));
            Label("Box style");
            Combo("##style", &s.esp_style, styles, IM_ARRAYSIZE(styles), geo::COL_W - 2 * geo::COL_PAD);
        }
        EndColumn();

        ImVec2 pb(base.x + geo::COL_W + geo::COL_GAP, base.y);
        ImGui::SetCursorScreenPos(pb);
        if (BeginColumn("##vis_b", pb, csz, "B"))
        {
            Checkbox("Glow", &s.glow);
            Checkbox("Radar reveal", &s.radar);

            ImGui::Dummy(ImVec2(0, 8 * geo::SCALE));
            Label("Preview", col::TEXT_DIM);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            const float bw = 70 * geo::SCALE, bh = 100 * geo::SCALE;
            dl->AddRect(p, ImVec2(p.x + bw, p.y + bh), s.esp ? col::ACCENT : col::TEXT_DIM);
            dl->AddRectFilled(ImVec2(p.x - 3 * geo::SCALE, p.y + bh * 0.55f),
                              ImVec2(p.x - 1 * geo::SCALE, p.y + bh), col::ACCENT);
            ImGui::Dummy(ImVec2(bw, bh + 4 * geo::SCALE));
            Label("player", col::TEXT);
        }
        EndColumn();
    }

    inline void TabMisc(Settings& s, float w, float h)
    {
        ImVec2 base = ImGui::GetCursorScreenPos();
        ImVec2 csz(geo::COL_W, h);

        if (BeginColumn("##misc_a", base, csz, "A"))
        {
            Checkbox("Bunnyhop", &s.bhop);
            Checkbox("Auto strafe", &s.auto_strafe);
            Checkbox("Edge jump", &s.edge_jump);
            Checkbox("Auto peek", &s.auto_peek);

            ImGui::Dummy(ImVec2(0, 6 * geo::SCALE));
            Checkbox("Fake lag", &s.fake_lag);
            Label("Ticks");
            ImGui::SetNextItemWidth(geo::COL_W - 2 * geo::COL_PAD);
            ImGui::SliderInt("##ticks", &s.fakelag_ticks, 0, 16, "%d");
        }
        EndColumn();

        ImVec2 pb(base.x + geo::COL_W + geo::COL_GAP, base.y);
        ImGui::SetCursorScreenPos(pb);
        if (BeginColumn("##misc_b", pb, csz, "B"))
        {
            Checkbox("Third person", &s.thirdperson);
            Label("Distance");
            ImGui::SetNextItemWidth(geo::COL_W - 2 * geo::COL_PAD);
            ImGui::SliderFloat("##tpdist", &s.tp_dist, 40.0f, 300.0f, "%.0f");
        }
        EndColumn();
    }

    inline void TabSkins(Settings& s, float w, float h)
    {
        static const char* knives[] = { "Default", "Karambit", "M9 Bayonet", "Butterfly", "Flip" };
        static const char* skins[]  = { "Default", "Redline", "Vulcan", "Fire Serpent", "Case Hardened" };
        ImVec2 base = ImGui::GetCursorScreenPos();
        ImVec2 csz(geo::COL_W, h);

        if (BeginColumn("##skin_a", base, csz, "A"))
        {
            Label("Knife");
            Combo("##knife", &s.knife, knives, IM_ARRAYSIZE(knives), geo::COL_W - 2 * geo::COL_PAD);
            ImGui::Dummy(ImVec2(0, 6 * geo::SCALE));
            Checkbox("StatTrak counter", &s.stattrak);
        }
        EndColumn();

        ImVec2 pb(base.x + geo::COL_W + geo::COL_GAP, base.y);
        ImGui::SetCursorScreenPos(pb);
        if (BeginColumn("##skin_b", pb, csz, "B"))
        {
            Label("AK-47");
            Combo("##ak", &s.ak, skins, IM_ARRAYSIZE(skins), geo::COL_W - 2 * geo::COL_PAD);
            Label("M4A4");
            Combo("##m4", &s.m4, skins, IM_ARRAYSIZE(skins), geo::COL_W - 2 * geo::COL_PAD);
            Label("AWP");
            Combo("##awp", &s.awp, skins, IM_ARRAYSIZE(skins), geo::COL_W - 2 * geo::COL_PAD);
        }
        EndColumn();
    }

    inline void TabSettings(Settings& s, float w, float h)
    {
        static const char* accents[] = { "Green", "Blue", "Red", "Purple", "Orange" };
        ImVec2 base = ImGui::GetCursorScreenPos();
        ImVec2 csz(geo::COL_W, h);

        if (BeginColumn("##set_a", base, csz, "A"))
        {
            Checkbox("Watermark", &s.watermark);
            Checkbox("Debug console", &s.debug_console);
            Checkbox("Save on exit", &s.save_on_exit);
            ImGui::Dummy(ImVec2(0, 6 * geo::SCALE));
            Label("Accent");
            Combo("##accent", &s.accent, accents, IM_ARRAYSIZE(accents), geo::COL_W - 2 * geo::COL_PAD);
        }
        EndColumn();

        ImVec2 pb(base.x + geo::COL_W + geo::COL_GAP, base.y);
        ImGui::SetCursorScreenPos(pb);
        if (BeginColumn("##set_b", pb, csz, "B"))
        {
            Label("Config name");
            ImGui::SetNextItemWidth(geo::COL_W - 2 * geo::COL_PAD);
            ImGui::InputText("##cfg", s.config, sizeof(s.config));
            ImGui::Dummy(ImVec2(0, 4 * geo::SCALE));
            ImGui::Button("Save", ImVec2(70 * geo::SCALE, 16 * geo::SCALE));
            ImGui::SameLine(0, 6 * geo::SCALE);
            ImGui::Button("Load", ImVec2(70 * geo::SCALE, 16 * geo::SCALE));

            ImGui::Dummy(ImVec2(0, 8 * geo::SCALE));
            Checkbox("Show demo", &s.show_demo);
        }
        EndColumn();
    }

    // ------------------------------------------------------------------
    // Окно
    // ------------------------------------------------------------------
    inline void Render(Settings& s)
    {
        if (!s.menu_open) return;

        const float K = geo::SCALE;
        ImGui::SetNextWindowSize(ImVec2(geo::WINDOW_W, geo::WINDOW_H), ImGuiCond_Always);
        // центрируем окно по кадру: ImGui знает DisplaySize
        const ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos(ImVec2((ds.x - geo::WINDOW_W) * 0.5f,
                                       (ds.y - geo::WINDOW_H) * 0.5f), ImGuiCond_Always);

        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(col::BG));
        ImGui::Begin("##gamesense", nullptr, flags);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // --- рамка: 1px #3C3C3C, 2px #282828, 1px #3C3C3C ---
        const float f = geo::FRAME;
        dl->AddRect(wp, ImVec2(wp.x + ws.x, wp.y + ws.y), col::FRAME_OUT);
        dl->AddRect(ImVec2(wp.x + f, wp.y + f), ImVec2(wp.x + ws.x - f, wp.y + ws.y - f),
                    col::FRAME_IN, 0.0f, 0, 2.0f * f);
        dl->AddRect(ImVec2(wp.x + 2 * f, wp.y + 2 * f),
                    ImVec2(wp.x + ws.x - 2 * f, wp.y + ws.y - 2 * f), col::FRAME_OUT);

        // --- радужная полоса под рамкой: 2px, во всю ширину, поверх рамки ---
        {
            const float y0 = wp.y + 1 * f;
            const float y1 = y0 + geo::RAINBOW_H;
            const int segs = 128;
            const float x0 = wp.x, x1 = wp.x + ws.x;
            for (int i = 0; i < segs; ++i)
            {
                const float t0 = (float)i / segs, t1 = (float)(i + 1) / segs;
                // (91,151,215) -> (204,213,66), как на референсе
                auto lerp = [](int a, int b, float t) { return (int)(a + (b - a) * t); };
                const ImU32 c = IM_COL32(lerp(91, 204, t0), lerp(151, 213, t0),
                                            lerp(215, 66, t0), 255);
                dl->AddRectFilled(ImVec2(x0 + (x1 - x0) * t0, y0),
                                  ImVec2(x0 + (x1 - x0) * t1 + 1, y1), c);
            }
        }

        // --- сайдбар ---
        const float sb_w = geo::SIDEBAR_W;
        // вертикальная граница сайдбара
        dl->AddLine(ImVec2(wp.x + sb_w, wp.y + 9 * f), ImVec2(wp.x + sb_w, wp.y + ws.y),
                    col::SEPARATOR);

        const float icon_step = geo::ICON_STEP;
        float iy = wp.y + 30 * K;
        static const Icon icons[] = {
            Icon::Headshot, Icon::NoSpread, Icon::Crosshair, Icon::SunMoon,
            Icon::Gears, Icon::Knife, Icon::Player, Icon::Floppy
        };
        for (int i = 0; i < IM_ARRAYSIZE(icons); ++i)
        {
            ImGui::SetCursorScreenPos(ImVec2(wp.x + (sb_w - icon_step) * 0.5f, iy));
            char id[16];
            snprintf(id, sizeof(id), "##sb%d", i);
            if (SidebarButton(id, icons[i], s.active_tab == i, icon_step))
                s.active_tab = (i < 5) ? i : s.active_tab;
            iy += icon_step;
        }

        // подпись активной вкладки внизу
        {
            const char* nm = active_name(s.active_tab);
            const ImVec2 ts = ImGui::CalcTextSize(nm);
            dl->AddRectFilled(ImVec2(wp.x + 1, wp.y + ws.y - 34 * K),
                              ImVec2(wp.x + sb_w, wp.y + ws.y - 1), col::ACTIVE_TAB);
            dl->AddText(ImVec2(wp.x + (sb_w - ts.x) * 0.5f, wp.y + ws.y - 22 * K),
                        IM_COL32(240, 240, 240, 255), nm);
        }

        // --- колонки ---
        const float col_h = ws.y - geo::WIN_PAD_TOP - 8 * K;
        ImVec2 cpos(wp.x + geo::WIN_PAD_L, wp.y + geo::WIN_PAD_TOP);
        ImGui::SetCursorScreenPos(cpos);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 3 * K));
        switch (s.active_tab)
        {
        case 0: TabRage(s, geo::COL_W, col_h); break;
        case 1: TabVisuals(s, geo::COL_W, col_h); break;
        case 2: TabMisc(s, geo::COL_W, col_h); break;
        case 3: TabSettings(s, geo::COL_W, col_h); break;
        case 4: TabSkins(s, geo::COL_W, col_h); break;
        default: TabRage(s, geo::COL_W, col_h); break;
        }
        ImGui::PopStyleVar();

        ImGui::End();
    }

    inline const char* active_name(int tab)
    {
        switch (tab)
        {
        case 0: return "Rage";
        case 1: return "Visuals";
        case 2: return "Misc";
        case 3: return "Settings";
        case 4: return "Skins";
        case 5: return "Knife";
        case 6: return "Player";
        case 7: return "Configs";
        default: return "Rage";
        }
    }
}
