// gui_menu.h — само меню: сайдбар, вкладки, колонки, содержимое.
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

        // Rage
        bool aimbot = false;
        bool silent = false;
        bool auto_fire = false;
        bool auto_wall = false;
        bool auto_scope = false;
        int  hitbox = 1;
        int  target_selection = 0;
        float fov = 180.0f;
        float smooth = 12.0f;
        bool resolver = false;
        bool backtrack = false;

        // Visuals
        bool esp = true;
        bool box = true;
        bool name = true;
        bool health = true;
        bool weapon = true;
        bool skeleton = false;
        bool chams = false;
        int  esp_style = 0;
        bool glow = false;
        bool radar = false;

        // Misc
        bool bhop = true;
        bool auto_strafe = false;
        bool fake_lag = false;
        int  fakelag_amount = 2;
        bool edge_jump = false;
        bool auto_peek = false;
        bool thirdperson = false;
        float thirdperson_dist = 120.0f;

        // Settings
        bool watermark = true;
        bool debug_console = false;
        bool save_on_exit = true;
        int  accent_index = 0;
        bool anti_screenshot = false;

        // Skins
        int  knife_model = 0;
        int  ak_skin = 0;
        int  m4_skin = 0;
        int  awp_skin = 0;
        bool stattrak = false;

        // состояние UI
        int  active_tab = 0;
        bool show_demo = false;
        char config_name[64] = "default";
        int  config_index = 0;
    };

    inline Settings& State()
    {
        static Settings s;
        return s;
    }

    // ---- вкладка Rage ----
    inline void TabRage(Settings& s, ImVec2 avail)
    {
        static const char* hitboxes[] = { "Head", "Neck", "Chest", "Stomach", "Pelvis" };
        static const char* targets[] = { "Closest to crosshair", "Lowest health", "Highest threat" };
        float col_w = (avail.x - geo::PAD) * 0.5f;
        float col_h = avail.y - geo::HEADER_H;

        if (BeginColumn("##rage_a", ImVec2(col_w, col_h)))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col::TEXT_DIM));
            ImGui::TextUnformatted("A");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));

            Checkbox("Enable aimbot", &s.aimbot);
            ImGui::Indent(16);
            Checkbox("Silent aim", &s.silent);
            Checkbox("Auto fire", &s.auto_fire);
            Checkbox("Auto wall", &s.auto_wall);
            Checkbox("Auto scope", &s.auto_scope);
            ImGui::Unindent(16);

            ImGui::Dummy(ImVec2(0, 6));
            ImGui::TextDisabled("Hitbox");
            Combo("##hitbox", &s.hitbox, hitboxes, IM_ARRAYSIZE(hitboxes), col_w - 30);

            ImGui::Dummy(ImVec2(0, 4));
            ImGui::TextDisabled("Target selection");
            Combo("##target", &s.target_selection, targets, IM_ARRAYSIZE(targets), col_w - 30);

            ImGui::Dummy(ImVec2(0, 6));
            ImGui::TextDisabled("FOV");
            ImGui::SetNextItemWidth(col_w - 30);
            ImGui::SliderFloat("##fov", &s.fov, 0.0f, 180.0f, "%.0f deg");
            ImGui::TextDisabled("Smooth");
            ImGui::SetNextItemWidth(col_w - 30);
            ImGui::SliderFloat("##smooth", &s.smooth, 1.0f, 30.0f, "%.1f");
        }
        EndColumn();

        ImGui::SameLine();
        if (BeginColumn("##rage_b", ImVec2(col_w, col_h)))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col::TEXT_DIM));
            ImGui::TextUnformatted("B");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::TextDisabled("Accuracy");
            Checkbox("Resolver", &s.resolver);
            Checkbox("Backtrack", &s.backtrack);

            ImGui::Dummy(ImVec2(0, 8));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::TextDisabled("State");
            ImGui::Text("Aimbot:     %s", s.aimbot ? "on" : "off");
            ImGui::Text("Silent:     %s", s.silent ? "on" : "off");
            ImGui::Text("Auto wall:  %s", s.auto_wall ? "on" : "off");
            ImGui::Text("Hitbox:     %s", hitboxes[s.hitbox]);
        }
        EndColumn();
    }

    // ---- вкладка Visuals ----
    inline void TabVisuals(Settings& s, ImVec2 avail)
    {
        float col_w = (avail.x - geo::PAD) * 0.5f;
        float col_h = avail.y - geo::HEADER_H;

        if (BeginColumn("##vis_a", ImVec2(col_w, col_h)))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col::TEXT_DIM));
            ImGui::TextUnformatted("A");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));

            Checkbox("Master switch", &s.esp);
            ImGui::Indent(16);
            Checkbox("Box", &s.box);
            Checkbox("Name", &s.name);
            Checkbox("Health", &s.health);
            Checkbox("Weapon", &s.weapon);
            Checkbox("Skeleton", &s.skeleton);
            Checkbox("Chams", &s.chams);
            ImGui::Unindent(16);

            ImGui::Dummy(ImVec2(0, 6));
            ImGui::TextDisabled("Box style");
            static const char* styles[] = { "Regular", "Corners", "3D", "Outline" };
            Combo("##espstyle", &s.esp_style, styles, IM_ARRAYSIZE(styles), col_w - 30);
        }
        EndColumn();

        ImGui::SameLine();
        if (BeginColumn("##vis_b", ImVec2(col_w, col_h)))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col::TEXT_DIM));
            ImGui::TextUnformatted("B");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));

            Checkbox("Glow", &s.glow);
            Checkbox("Radar reveal", &s.radar);

            ImGui::Dummy(ImVec2(0, 8));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::TextDisabled("Preview");
            ImU32 box_col = s.esp ? col::ACCENT : col::TEXT_DIM;
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRect(ImVec2(p.x + 10, p.y + 6),
                                                ImVec2(p.x + 90, p.y + 60), box_col, 0, 0, 1.5f);
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 12, p.y + 46),
                                                      ImVec2(p.x + 20, p.y + 58), col::ACCENT);
            ImGui::GetWindowDrawList()->AddText(ImVec2(p.x + 10, p.y + 64), col::TEXT, "player");
            ImGui::Dummy(ImVec2(0, 84));
        }
        EndColumn();
    }

    // ---- вкладка Misc ----
    inline void TabMisc(Settings& s, ImVec2 avail)
    {
        float col_w = (avail.x - geo::PAD) * 0.5f;
        float col_h = avail.y - geo::HEADER_H;

        if (BeginColumn("##misc_a", ImVec2(col_w, col_h)))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col::TEXT_DIM));
            ImGui::TextUnformatted("A");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::TextDisabled("Movement");
            Checkbox("Bunnyhop", &s.bhop);
            Checkbox("Auto strafe", &s.auto_strafe);
            Checkbox("Edge jump", &s.edge_jump);
            Checkbox("Auto peek", &s.auto_peek);

            ImGui::Dummy(ImVec2(0, 6));
            ImGui::TextDisabled("Fake lag");
            Checkbox("Enable fake lag", &s.fake_lag);
            ImGui::SetNextItemWidth(col_w - 30);
            ImGui::SliderInt("##fakelag", &s.fakelag_amount, 0, 16, "%d ticks");
        }
        EndColumn();

        ImGui::SameLine();
        if (BeginColumn("##misc_b", ImVec2(col_w, col_h)))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col::TEXT_DIM));
            ImGui::TextUnformatted("B");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::TextDisabled("Camera");
            Checkbox("Third person", &s.thirdperson);
            ImGui::SetNextItemWidth(col_w - 30);
            ImGui::SliderFloat("##tpdist", &s.thirdperson_dist, 40.0f, 300.0f, "%.0f units");
        }
        EndColumn();
    }

    // ---- вкладка Skins ----
    inline void TabSkins(Settings& s, ImVec2 avail)
    {
        float col_w = (avail.x - geo::PAD) * 0.5f;
        float col_h = avail.y - geo::HEADER_H;

        if (BeginColumn("##skin_a", ImVec2(col_w, col_h)))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col::TEXT_DIM));
            ImGui::TextUnformatted("A");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::TextDisabled("Knife");
            static const char* knives[] = { "Default", "Karambit", "M9 Bayonet", "Butterfly", "Flip" };
            Combo("##knife", &s.knife_model, knives, IM_ARRAYSIZE(knives), col_w - 30);

            ImGui::Dummy(ImVec2(0, 6));
            Checkbox("StatTrak counter", &s.stattrak);
        }
        EndColumn();

        ImGui::SameLine();
        if (BeginColumn("##skin_b", ImVec2(col_w, col_h)))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col::TEXT_DIM));
            ImGui::TextUnformatted("B");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::TextDisabled("AK-47");
            static const char* skins[] = { "Default", "Redline", "Vulcan", "Fire Serpent", "Case Hardened" };
            Combo("##ak", &s.ak_skin, skins, IM_ARRAYSIZE(skins), col_w - 30);
            ImGui::TextDisabled("M4A4");
            Combo("##m4", &s.m4_skin, skins, IM_ARRAYSIZE(skins), col_w - 30);
            ImGui::TextDisabled("AWP");
            Combo("##awp", &s.awp_skin, skins, IM_ARRAYSIZE(skins), col_w - 30);
        }
        EndColumn();
    }

    // ---- вкладка Settings ----
    inline void TabSettings(Settings& s, ImVec2 avail)
    {
        float col_w = (avail.x - geo::PAD) * 0.5f;
        float col_h = avail.y - geo::HEADER_H;

        if (BeginColumn("##set_a", ImVec2(col_w, col_h)))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col::TEXT_DIM));
            ImGui::TextUnformatted("A");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::TextDisabled("Interface");
            Checkbox("Watermark", &s.watermark);
            Checkbox("Debug console", &s.debug_console);
            Checkbox("Anti screenshot", &s.anti_screenshot);
            Checkbox("Save config on exit", &s.save_on_exit);

            ImGui::Dummy(ImVec2(0, 6));
            ImGui::TextDisabled("Accent");
            static const char* accents[] = { "Green", "Blue", "Red", "Purple", "Orange" };
            Combo("##accent", &s.accent_index, accents, IM_ARRAYSIZE(accents), col_w - 30);
        }
        EndColumn();

        ImGui::SameLine();
        if (BeginColumn("##set_b", ImVec2(col_w, col_h)))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(col::TEXT_DIM));
            ImGui::TextUnformatted("B");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::TextDisabled("Config");
            ImGui::SetNextItemWidth(col_w - 30);
            ImGui::InputText("##cfgname", s.config_name, sizeof(s.config_name));

            ImGui::Dummy(ImVec2(0, 6));
            if (ImGui::Button("Save config", ImVec2(col_w - 60, 24)))
                ImGui::OpenPopup("saved");
            if (ImGui::BeginPopup("saved"))
            {
                ImGui::TextUnformatted("Config saved.");
                ImGui::EndPopup();
            }
            ImGui::SameLine();
            ImGui::Button("Load", ImVec2(col_w - 60, 24));

            ImGui::Dummy(ImVec2(0, 10));
            ImGui::TextDisabled("Dear ImGui demo");
            Checkbox("Show demo window", &s.show_demo);

            ImGui::Dummy(ImVec2(0, 10));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 6));
            ImGui::TextDisabled("Build: test overlay | kiero + OpenGL");
            ImGui::TextDisabled("Insert — toggle menu");
            ImGui::TextDisabled("F12   — save screenshot");
        }
        EndColumn();
    }

    // ---- главное окно ----
    inline void Render(Settings& s)
    {
        if (!s.menu_open) return;

        ImGui::SetNextWindowSize(ImVec2(820.0f * geo::SCALE, 560.0f * geo::SCALE),
                                 ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(60.0f * geo::SCALE, 60.0f * geo::SCALE),
                                ImGuiCond_Always);

        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::Begin("##gamesense", nullptr, flags);
        ImGui::PopStyleVar(2);

        ImVec2 win_pos = ImGui::GetWindowPos();
        ImVec2 win_size = ImGui::GetWindowSize();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // --- левый сайдбар ---
        const float sb_w = geo::SIDEBAR_W;
        dl->AddRectFilled(win_pos, ImVec2(win_pos.x + sb_w, win_pos.y + win_size.y), col::SIDEBAR);
        dl->AddLine(ImVec2(win_pos.x + sb_w, win_pos.y),
                    ImVec2(win_pos.x + sb_w, win_pos.y + win_size.y), col::SEPARATOR);

        ImGui::SetCursorScreenPos(ImVec2(win_pos.x + 6 * geo::SCALE, win_pos.y + 8 * geo::SCALE));
        ImGui::BeginGroup();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4 * geo::SCALE));
        struct Entry { Icon icon; const char* tip; };
        static const Entry entries[] = {
            { Icon::Aim,      "Rage" },
            { Icon::Visuals,  "Visuals" },
            { Icon::Misc,     "Misc" },
            { Icon::Settings, "Settings" },
            { Icon::Skins,    "Skins" },
            { Icon::Profile,  "Player" },
            { Icon::Lua,      "Lua" },
        };
        for (int i = 0; i < IM_ARRAYSIZE(entries); ++i)
        {
            char id[16];
            snprintf(id, sizeof(id), "##sb%d", i);
            if (SidebarButton(id, entries[i].icon, s.active_tab == i, entries[i].tip))
                s.active_tab = i;
        }
        ImGui::PopStyleVar();
        ImGui::EndGroup();

        // подпись внизу сайдбара
        dl->AddText(ImVec2(win_pos.x + 10 * geo::SCALE, win_pos.y + win_size.y - 24 * geo::SCALE),
                    col::TEXT_ACTIVE, active_name(s.active_tab));

        // --- заголовок окна ---
        ImVec2 head_min(win_pos.x + sb_w, win_pos.y);
        ImVec2 head_max(win_pos.x + win_size.x, win_pos.y + geo::HEADER_H);
        dl->AddRectFilled(head_min, head_max, col::BG);
        dl->AddLine(ImVec2(head_min.x, head_max.y), ImVec2(head_max.x, head_max.y), col::SEPARATOR);
        dl->AddText(ImVec2(head_min.x + 12 * geo::SCALE, win_pos.y + 7 * geo::SCALE),
                    col::TEXT_ACTIVE, "counter-strike 1.6");
        dl->AddText(ImVec2(head_min.x + 118 * geo::SCALE, win_pos.y + 7 * geo::SCALE),
                    col::TEXT_DIM, "// gamesense");

        // кнопка закрытия
        ImGui::SetCursorScreenPos(ImVec2(head_max.x - 24 * geo::SCALE, win_pos.y + 7 * geo::SCALE));
        if (ImGui::InvisibleButton("##close", ImVec2(14 * geo::SCALE, 14 * geo::SCALE)))
            s.menu_open = false;
        if (ImGui::IsItemHovered())
            dl->AddRectFilled(ImVec2(head_max.x - 26 * geo::SCALE, win_pos.y + 5 * geo::SCALE),
                              ImVec2(head_max.x - 8 * geo::SCALE, win_pos.y + 23 * geo::SCALE),
                              IM_COL32(60, 30, 30, 255));
        DrawIcon(dl, Icon::Close, ImVec2(head_max.x - 17 * geo::SCALE, win_pos.y + 14 * geo::SCALE),
                 col::TEXT_DIM, 4.5f * geo::SCALE);

        // --- контент ---
        ImVec2 content_pos(head_min.x + geo::PAD, head_max.y + geo::PAD);
        ImGui::SetCursorScreenPos(content_pos);
        ImVec2 avail(win_size.x - sb_w - geo::PAD * 2,
                     win_size.y - geo::HEADER_H - geo::PAD * 2);

        ImGui::BeginChild("##content", avail, ImGuiChildFlags_None);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        switch (s.active_tab)
        {
        case 0: TabRage(s, avail); break;
        case 1: TabVisuals(s, avail); break;
        case 2: TabMisc(s, avail); break;
        case 3: TabSettings(s, avail); break;
        case 4: TabSkins(s, avail); break;
        default: TabRage(s, avail); break;
        }
        ImGui::PopStyleVar();
        ImGui::EndChild();

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
        case 5: return "Player";
        case 6: return "Lua";
        default: return "Rage";
        }
    }
}
