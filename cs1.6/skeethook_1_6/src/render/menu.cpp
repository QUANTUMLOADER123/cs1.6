#include <string>
#include <vector>
#include <crt/crt.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <features/vars.hpp>
#include <core/config/config.hpp>
#include <render/render.hpp>
#include <render/menu.hpp>
#include <features/players/chams/chams.hpp>
#include <features/world/skybox/skybox.hpp>
#include <features/world/model_changer/model_changer.hpp>
#include <features/world/removals/removals.hpp>
#include <features/world/anim_breakers/anim_breakers.hpp>
#include <features/world/killsounds/killsounds.hpp>
#include <features/antihit/antihit.hpp>
#include <features/ragebot/ragebot.hpp>
#include <features/legitbot/legitbot.hpp>

void menu::render()
{
    if (!opened)
        return;

    const char* tabs[] = { "assault", "visuals", "misc" };

    ImGui::PushFont(tahoma_font);

    ImGui::SetNextWindowSize(ImVec2(600.0f, 420.0f));
    ImGui::Begin("##menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

    const ImVec2 size(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x * 2, ImGui::GetWindowSize().y - ImGui::GetStyle().WindowPadding.y * 2);
    const ImVec2 origin(ImGui::GetWindowPos().x + ImGui::GetStyle().WindowPadding.x, ImGui::GetWindowPos().y + ImGui::GetStyle().WindowPadding.y);
    const ImVec2 frame_min(ImFloor(origin.x), ImFloor(origin.y));
    const ImVec2 frame_max(ImFloor(origin.x + size.x), ImFloor(origin.y + size.y));
    ImDrawList* draw = ImGui::GetWindowDrawList();

    draw->AddRectFilled(frame_min, frame_max, ImColor(8, 8, 8), 4.0f, ImDrawFlags_RoundCornersAll);
    draw->AddRect(frame_min + ImVec2(1, 1), frame_max - ImVec2(1, 1), ImColor(27, 27, 27, 255), 4.0f, ImDrawFlags_RoundCornersAll);
    draw->AddRect(ImVec2(frame_min.x + 10, frame_min.y + 35), ImVec2(frame_max.x - 480, frame_max.y - 18), ImColor(26, 26, 26, 255));

    draw->AddLine(ImVec2(frame_min.x, frame_max.y - 18), ImVec2(frame_max.x, frame_max.y - 18), ImColor(27, 27, 27, 255));
    draw->AddLine(ImVec2(frame_min.x, frame_min.y + 25), ImVec2(frame_max.x, frame_min.y + 25), ImColor(27, 27, 27, 255));

    const int fade_line_count = 60;
    const float fade_stop = size.x;
    const float center_point = fade_stop / 2.0f;
    for (int i = 0; i < fade_line_count; i++)
    {
        const float alpha = 1.0f - (i * (1.0f / fade_line_count));
        const ImVec2 start = ImVec2(frame_min.x + fade_stop - ImFloor(i * (center_point / fade_line_count)), frame_min.y + 25);
        const ImVec2 end = ImVec2(frame_min.x + fade_stop - ImFloor((i + 1) * (center_point / fade_line_count)), frame_min.y + 25);
        draw->AddLine(start, end, ImColor(main_color.x, main_color.y, main_color.z, alpha));
    }

    draw->AddText(ImVec2(frame_min.x + 10, frame_min.y + 7), ImColor(main_color), "evalate");

    draw->AddText(ImVec2(frame_min.x + 10, frame_max.y - 15), ImColor(255, 255, 255, 100), "Build:");
    draw->AddText(ImVec2(frame_min.x + 41, frame_max.y - 15), ImColor(main_color), "Live");

    const char* welcome = "welcome back";
    const ImVec2 welcome_size = ImGui::CalcTextSize(welcome);
    draw->AddText(ImVec2(frame_max.x - ImCeil(welcome_size.x) - 20, frame_max.y - 15), ImColor(main_color), welcome);

    ImGui::SetCursorPosX(112);
    ImGui::SetCursorPosY(10);
    ImGui::PushID("##tabs");
    for (int index = 0; index < 3; index++)
    {
        if (tab(tabs[index], selected_tab == index))
            selected_tab = index;
        ImGui::SameLine();
    }
    ImGui::PopID();

    ImGui::SetCursorPosX(30);
    ImGui::SetCursorPosY(54);

    switch (selected_tab)
    {
    case 0:
    {
        const char* subtabs[] = { "rage", "legit", "anti-hit" };
        if (selected_subtab > 2)
            selected_subtab = 0;

        ImGui::PushID("##sub");
        for (int index = 0; index < 3; index++)
        {
            ImGui::SetCursorPosX(30);
            ImGui::SetCursorPosY(54.0f + index * 25.0f);
            if (subtab(subtabs[index], selected_subtab == index))
                selected_subtab = index;
        }
        ImGui::PopID();

        if (selected_subtab == 0)
        {
            ImGui::SetCursorPosX(122);
            ImGui::SetCursorPosY(38);
            menu_child("Ragebot", ImVec2(226, 337));

            checkbox("enabled##ragebot", &vars::ragebot.enabled);

            if (vars::ragebot.enabled)
            {
                multi_combo("hitbox##rage", vars::ragebot.hitbox, ragebot::hitbox_items, 8);
                slider_float("hit chance", &vars::ragebot.hit_chance, 0.0f, 100.0f, "%.0f%%");
                slider_float("minimum damage", &vars::ragebot.minimum_damage, 0.0f, 100.0f, "%.0f");
                checkbox("penetration", &vars::ragebot.penetration);
                checkbox("backtrack", &vars::ragebot.backtrack);

                if (vars::ragebot.backtrack)
                    slider_float("backtrack ms", &vars::ragebot.backtrack_ms, 1.0f, 500.0f, "%.0f");

                checkbox("no spread", &vars::ragebot.nospread);
            }

            end_menu_child();

            ImGui::SetCursorPosX(358);
            ImGui::SetCursorPosY(38);
            menu_child("Autostop", ImVec2(224, 337));

            multi_combo("autostop", vars::ragebot.autostop, ragebot::autostop_items, 4);
            checkbox("autoscope", &vars::ragebot.autoscope);

            end_menu_child();
        }
        else if (selected_subtab == 1)
        {
            ImGui::SetCursorPosX(122);
            ImGui::SetCursorPosY(38);
            menu_child("Legitbot", ImVec2(226, 337));

            checkbox("enabled##legitbot", &vars::legitbot.enabled);

            if (vars::legitbot.enabled)
            {
                combo("activation", &vars::legitbot.activation, legitbot::activation_items, 2);
                multi_combo("hitbox##legit_hb", vars::legitbot.hitbox, legitbot::hitbox_items, 8);
                slider_float("fov##legit_fov", &vars::legitbot.fov, 1.0f, 90.0f, "%.0f");
                slider_float("smooth", &vars::legitbot.smooth, 1.0f, 20.0f, "%.0f");
            }

            end_menu_child();

            ImGui::SetCursorPosX(358);
            ImGui::SetCursorPosY(38);
            menu_child("Assist", ImVec2(224, 337));

            if (vars::legitbot.enabled)
            {
                checkbox("rcs", &vars::legitbot.rcs);
                checkbox("draw fov", &vars::legitbot.draw_fov);
            }

            end_menu_child();
        }
        else
        {
            ImGui::SetCursorPosX(122);
            ImGui::SetCursorPosY(38);
            menu_child("Anti-hit", ImVec2(226, 337));

            checkbox("enable", &vars::antihit.enabled);

            if (vars::antihit.enabled)
            {
                const char* type_items[] = { "none", "jitter", "spin", "3 way", "5 way", "swing", "random" };
                combo("jitter type", &vars::antihit.type, type_items, 7);
                slider_float("yaw", &vars::antihit.yaw, 0.0f, 360.0f, "%.0f");
                slider_float("pitch", &vars::antihit.pitch, -89.0f, 89.0f, "%.0f");

                if (vars::antihit.type != 0 && vars::antihit.type != 2)
                    slider_float("jitter amount", &vars::antihit.jitter_amount, 0.0f, 180.0f, "%.1f");

                if (vars::antihit.type == 2)
                    slider_float("spin speed", &vars::antihit.spin_speed, 1.0f, 90.0f, "%.1f");
            }

            end_menu_child();

            ImGui::SetCursorPosX(358);
            ImGui::SetCursorPosY(38);
            menu_child("Fake lag", ImVec2(224, 337));

            checkbox("fake lag", &vars::antihit.fake_lag);

            if (vars::antihit.fake_lag)
            {
                combo("fake lag type", &vars::antihit.fake_lag_type, antihit::fake_lag_type_items, 4);
                slider_int("fake lag ticks", &vars::antihit.fake_lag_ticks, 1, 16, "%d");
                combo("fake lag move", &vars::antihit.fake_lag_move, antihit::fake_lag_move_items, 4);
                checkbox("while shooting", &vars::antihit.fake_lag_while_shooting);
            }

            end_menu_child();
        }
        break;
    }
    case 1:
    {
        const char* subtabs[] = { "esp", "chams", "world" };
        if (selected_subtab > 2)
            selected_subtab = 0;

        ImGui::PushID("##sub");
        for (int index = 0; index < 3; index++)
        {
            ImGui::SetCursorPosX(30);
            ImGui::SetCursorPosY(54.0f + index * 25.0f);
            if (subtab(subtabs[index], selected_subtab == index))
                selected_subtab = index;
        }
        ImGui::PopID();

        if (selected_subtab == 0)
        {
            ImGui::SetCursorPosX(122);
            ImGui::SetCursorPosY(38);
            menu_child("ESP", ImVec2(226, 337));

            checkbox("esp", &vars::esp.enabled);

            if (vars::esp.enabled)
            {
                checkbox("box", &vars::esp.box);
                checkbox("corners", &vars::esp.corners);
                checkbox("name", &vars::esp.name);
                checkbox("weapon", &vars::esp.weapon);
            }

            end_menu_child();

            ImGui::SetCursorPosX(358);
            ImGui::SetCursorPosY(38);
            menu_child("Extras", ImVec2(224, 337));

            if (vars::esp.enabled)
            {
                checkbox("weapon icons", &vars::esp.weapon_icons);
                checkbox("distance##esp_dist", &vars::esp.distance);
                checkbox("skeleton", &vars::esp.skeleton);
                checkbox("only visible", &vars::esp.only_visible);
                checkbox("enemies only##esp_enem", &vars::esp.enemies_only);
            }

            end_menu_child();
        }
        else if (selected_subtab == 1)
        {
            ImGui::SetCursorPosX(122);
            ImGui::SetCursorPosY(38);
            menu_child("Chams", ImVec2(226, 337));

            checkbox("chams", &vars::chams.enabled);

            if (vars::chams.enabled)
            {
                combo("material##enemy", &vars::chams.material, chams::material_names, 4);
                checkbox("behind walls##chams_enemy", &vars::chams.ignore_z);
                checkbox("enemies only##chams_enem", &vars::chams.enemies_only);
            }

            end_menu_child();

            ImGui::SetCursorPosX(358);
            ImGui::SetCursorPosY(38);
            menu_child("Colors", ImVec2(224, 337));

            if (vars::chams.enabled)
            {
                color_picker("visible color", vars::chams.visible_color, true);
                color_picker("behind wall color", vars::chams.occluded_color, true);
            }

            checkbox("local", &vars::chams.local.enabled);

            if (vars::chams.local.enabled)
            {
                combo("material##chams_local", &vars::chams.local.material, chams::material_names, 4);
                checkbox("behind walls##chams_local_z", &vars::chams.local.ignore_z);
                color_picker("visible color##chams_local_v", vars::chams.local.visible_color, true);
                color_picker("behind wall color##chams_local_b", vars::chams.local.occluded_color, true);
            }

            checkbox("hands", &vars::chams.hands.enabled);

            if (vars::chams.hands.enabled)
            {
                combo("material##chams_hands", &vars::chams.hands.material, chams::material_names, 4);
                checkbox("behind walls##chams_hands_z", &vars::chams.hands.ignore_z);
                color_picker("visible color##chams_hands_v", vars::chams.hands.visible_color, true);
                color_picker("behind wall color##chams_hands_b", vars::chams.hands.occluded_color, true);
            }

            end_menu_child();
        }
        else
        {
            ImGui::SetCursorPosX(122);
            ImGui::SetCursorPosY(38);
            menu_child("World", ImVec2(226, 165));

            checkbox("third person", &vars::third_person.enabled);

            if (vars::third_person.enabled)
                slider_float("distance##tp", &vars::third_person.distance, 50.0f, 300.0f, "%.0f");

            checkbox("nightmode", &vars::nightmode.enabled);

            if (vars::nightmode.enabled)
                color_picker("map color", vars::nightmode.color, false);

            checkbox("aspect ratio", &vars::aspect_ratio.enabled);

            if (vars::aspect_ratio.enabled)
                slider_float("aspect", &vars::aspect_ratio.aspect, 0.5f, 2.4f, "%.3f");

            checkbox("bullet tracers", &vars::bullet_tracers.enabled);
            ImGui::SameLine();
            color_swatch("##bullet tracer color", vars::bullet_tracers.color, true);
            checkbox("bullet impacts", &vars::bullet_impacts.enabled);
            ImGui::SameLine();
            color_swatch("##bullet impact color", vars::bullet_impacts.color, true);

            end_menu_child();

            ImGui::SetCursorPosX(358);
            ImGui::SetCursorPosY(38);
            menu_child("Environment", ImVec2(224, 165));

            checkbox("skybox color", &vars::skybox.enabled);

            if (vars::skybox.enabled)
                color_picker("sky color", vars::skybox.color, false);

            checkbox("skybox changer", &vars::skybox.changer);

            if (vars::skybox.changer)
            {
                if (skybox::names.empty())
                    skybox::refresh();

                std::vector<const char*> sky_items;
                sky_items.reserve(skybox::names.size());
                for (const std::string& name : skybox::names)
                    sky_items.push_back(name.c_str());

                int current = skybox::selected();
                if (!sky_items.empty() && combo("sky", &current, sky_items.data(), static_cast<int>(sky_items.size())))
                    skybox::select(current);
            }

            end_menu_child();

            ImGui::SetCursorPosX(122);
            ImGui::SetCursorPosY(211);
            menu_child("Viewmodel", ImVec2(226, 165));

            checkbox("viewmodel changer", &vars::viewmodel.enabled);

            if (vars::viewmodel.enabled)
            {
                slider_float("fov##viewmodel", &vars::viewmodel.fov, 10.0f, 120.0f, "%.1f");
                slider_float("offset x", &vars::viewmodel.offset_x, -10.0f, 10.0f, "%.1f");
                slider_float("offset y", &vars::viewmodel.offset_y, -10.0f, 10.0f, "%.1f");
                slider_float("offset z", &vars::viewmodel.offset_z, -10.0f, 10.0f, "%.1f");
            }

            checkbox("fov changer##ctrl", &vars::fov_changer.enabled);

            if (vars::fov_changer.enabled)
                slider_float("fov##fov changer", &vars::fov_changer.fov, 10.0f, 120.0f, "%.0f");

            end_menu_child();

            ImGui::SetCursorPosX(358);
            ImGui::SetCursorPosY(211);
            menu_child("Removals", ImVec2(224, 165));

            multi_combo("removals", vars::removals.toggles, removals::items, 6);
            multi_combo("anim breakers", vars::anim_breakers.toggles, anim_breakers::items, 2);

            checkbox("custom scope overlay", &vars::removals.custom_scope);

            if (vars::removals.custom_scope)
                slider_float("scope size", &vars::removals.custom_scope_size, 0.5f, 4.0f, "%.1f");

            end_menu_child();
        }
        break;
    }
    case 2:
    {
        const char* subtabs[] = { "misc", "model", "configs", "lua" };
        if (selected_subtab > 3)
            selected_subtab = 0;

        ImGui::PushID("##sub");
        for (int index = 0; index < 4; index++)
        {
            ImGui::SetCursorPosX(30);
            ImGui::SetCursorPosY(54.0f + index * 25.0f);
            if (subtab(subtabs[index], selected_subtab == index))
                selected_subtab = index;
        }
        ImGui::PopID();

        if (selected_subtab == 0)
        {
            ImGui::SetCursorPosX(122);
            ImGui::SetCursorPosY(38);
            menu_child("Misc", ImVec2(226, 337));

            checkbox("bunny hop", &vars::misc.bunny_hop);
            checkbox("autostrafe", &vars::misc.autostrafe);
            slider_int("autostrafe climb", &vars::misc.autostrafe_speed, 1, 10, "%d");
            checkbox("quick stop", &vars::misc.quick_stop);
            checkbox("ground strafe", &vars::misc.ground_strafe);
checkbox("jumpbug", &vars::misc.jumpbug);
    checkbox("texturebug", &vars::misc.texturebug);

    if (vars::misc.texturebug)
    {
        slider_float("texturebug upmove", &vars::misc.texturebug_upmove, -25.0f, 0.0f, "%.2f");
    }

    end_menu_child();

            ImGui::SetCursorPosX(358);
            ImGui::SetCursorPosY(38);
            menu_child("Other", ImVec2(224, 337));

            checkbox("killsound", &vars::misc.killsound.enabled);

            if (vars::misc.killsound.enabled)
                combo("sound", &vars::misc.killsound.sound, killsounds::names, killsounds::count);

            end_menu_child();
        }
        else if (selected_subtab == 1)
        {
            ImGui::SetCursorPosX(122);
            ImGui::SetCursorPosY(38);
            menu_child("Model changer", ImVec2(226, 337));

            checkbox("model changer", &vars::model_changer.enabled);

            if (vars::model_changer.enabled)
            {
                combo("type", &vars::model_changer.type, model_changer::type_names, 2);

                if (vars::model_changer.type == 1)
                    text_input("path", vars::model_changer.path, sizeof(vars::model_changer.path));
                else
                    combo("model", &vars::model_changer.model, model_changer::model_names, 11);
            }

            end_menu_child();

            ImGui::SetCursorPosX(358);
            ImGui::SetCursorPosY(38);
            menu_child("Info", ImVec2(224, 337));

            end_menu_child();
        }
        else if (selected_subtab == 2)
        {
            ImGui::SetCursorPosX(122);
            ImGui::SetCursorPosY(38);
            menu_child("Configs", ImVec2(226, 337));

            if (!config_scanned)
            {
                config::refresh();
                config_scanned = true;
            }

            const std::vector<std::string>& items = config::names();
            const char* preview = (config::selected() >= 0 && config::selected() < static_cast<int>(items.size())) ? items[config::selected()].c_str() : "";

            if (listbox(items, &config::selected()))
            {
                config::selected() = ImClamp(config::selected(), 0, ImMax(static_cast<int>(items.size()) - 1, 0));
                if (config::selected() >= 0 && config::selected() < static_cast<int>(items.size()))
                {
                    CRT::StringCopyN(config_name, items[config::selected()].c_str(), sizeof(config_name) - 1);
                    config_name[sizeof(config_name) - 1] = '\0';
                }
            }

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.0f);
            text_input("name", config_name, sizeof(config_name));

            const bool present = config::exists(config_name);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);
            if (button(present ? "save" : "create|save"))
                config::save(config_name);

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
            if (button("load"))
                config::load(config_name);

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
            if (button("delete") && preview[0])
            {
                config::remove(preview);
                config_name[0] = '\0';
            }

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
            if (button("refresh"))
            {
                config::refresh();
                if (config_name[0])
                    config::load(config_name);
            }

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
            if (button("open directory"))
                config::open_directory();

            end_menu_child();

            ImGui::SetCursorPosX(358);
            ImGui::SetCursorPosY(38);
            menu_child("Info", ImVec2(224, 337));

            end_menu_child();
        }
        else
        {
            ImGui::SetCursorPosX(122);
            ImGui::SetCursorPosY(38);
            menu_child("Lua", ImVec2(226, 337));

            end_menu_child();

            ImGui::SetCursorPosX(358);
            ImGui::SetCursorPosY(38);
            menu_child("Info", ImVec2(224, 337));

            end_menu_child();
        }
        break;
    }
    }

    ImGui::End();
    ImGui::PopFont();
}