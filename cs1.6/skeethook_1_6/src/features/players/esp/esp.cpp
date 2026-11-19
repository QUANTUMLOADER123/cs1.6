#include <cfloat>
#include <cstdio>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/structs/cl_entity.hpp>
#include <cstrike1_5/structs/studio.hpp>
#include <cstrike1_5/interfaces/engine/engine.hpp>
#include <cstrike1_5/interfaces/studio/studio.hpp>
#include <cstrike1_5/interfaces/trace/trace.hpp>
#include <cstrike1_5/classes/c_base_player.hpp>
#include <cstrike1_5/classes/c_player.hpp>
#include <features/vars.hpp>
#include <core/console.hpp>
#include <render/render.hpp>
#include <render/sprites.hpp>
#include <features/players/esp/esp.hpp>

void esp::draw()
{
    if (!vars::esp.enabled)
        return;

    cl_entity_t* entity_list = engine::entities();
    if (!entity_list)
        return;

    const int local_index = engine::local_player_index();
    const int max_clients = engine::max_clients();

    c_base_player local(engine::entity(local_index));
    const vec3_t eye = local.get_eye_position();
    const bool has_eye = eye.is_valid();
    const int local_team = engine::player_team(local_index);

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

    for (int index = 1; index <= max_clients; index++)
    {
        if (index == local_index)
            continue;

        c_base_player player(engine::entity(index));
        if (!player.base || !player.is_player() || !player.is_alive() || !player.get_origin().is_valid() || (player.base->curstate.effects & ef_nodraw) || !studio::has_model(index))
            continue;

        const int entity_team = engine::player_team(index);
        if (vars::esp.enemies_only && entity_team != 0 && entity_team == local_team)
            continue;

        if (vars::esp.only_visible)
        {
            if (!has_eye)
                continue;

            const vec3_t to_target = player.get_origin() - eye;
            const float distance = to_target.length();
            if (distance < 1.0f)
                continue;

            const vec3_t aim_end = eye + (to_target / distance) * (distance + 48.0f);

            vec3_t hit;
            const bool visible = studio::hitbox_hit(index, eye, aim_end, hit) ? trace::visible(eye, hit) : trace::visible(eye, player.get_origin());
            if (!visible)
                continue;
        }

        vec3_t world_min;
        vec3_t world_max;
        if (!studio::hitbox_bounds(index, world_min, world_max))
            continue;

        const vec3_t world_corners[8] =
        {
            vec3_t(world_min.x, world_min.y, world_min.z),
            vec3_t(world_min.x, world_min.y, world_max.z),
            vec3_t(world_min.x, world_max.y, world_min.z),
            vec3_t(world_min.x, world_max.y, world_max.z),
            vec3_t(world_max.x, world_min.y, world_min.z),
            vec3_t(world_max.x, world_min.y, world_max.z),
            vec3_t(world_max.x, world_max.y, world_min.z),
            vec3_t(world_max.x, world_max.y, world_max.z),
        };

        float screen_min_x = FLT_MAX, screen_min_y = FLT_MAX;
        float screen_max_x = -FLT_MAX, screen_max_y = -FLT_MAX;
        int projected = 0;

        for (const vec3_t& corner : world_corners)
        {
            vec2_t point;
            if (!render::wts(corner, point))
                continue;

            screen_min_x = point.x < screen_min_x ? point.x : screen_min_x;
            screen_min_y = point.y < screen_min_y ? point.y : screen_min_y;
            screen_max_x = point.x > screen_max_x ? point.x : screen_max_x;
            screen_max_y = point.y > screen_max_y ? point.y : screen_max_y;
            projected++;
        }

        if (projected < 2)
            continue;

        const ImVec2 box_min(screen_min_x, screen_min_y);
        const ImVec2 box_max(screen_max_x, screen_max_y);

        const float box_height = box_max.y - box_min.y;
        if (box_height < 2.0f || box_height > static_cast<float>(render::height))
            continue;

        if (box_max.x < 0.0f || box_min.x > static_cast<float>(render::width))
            continue;

        c_player esp_player(player.base, engine::player_info(index));
        const float meters = esp_player.get_distance(eye) * 0.0254f;

        if (vars::esp.box)
        {
            draw_list->AddRect(ImVec2(box_min.x - 1.0f, box_min.y - 1.0f), ImVec2(box_max.x + 1.0f, box_max.y + 1.0f), IM_COL32(0, 0, 0, 128));
            draw_list->AddRect(box_min, box_max, IM_COL32(255, 255, 255, 255));
            draw_list->AddRect(ImVec2(box_min.x + 1.0f, box_min.y + 1.0f), ImVec2(box_max.x - 1.0f, box_max.y - 1.0f), IM_COL32(0, 0, 0, 128));
        }

        if (vars::esp.corners)
            corners(draw_list, box_min, box_max);

        if (vars::esp.name)
            name(draw_list, &esp_player, box_min, box_max);

        if (vars::esp.weapon || vars::esp.weapon_icons)
            weapon(draw_list, &esp_player, box_min, box_max);

        const float weapon_offset = vars::esp.weapon_icons ? 40.0f : (vars::esp.weapon ? 20.0f : 2.0f);
        if (vars::esp.distance)
            distance(draw_list, meters, box_min, box_max, box_max.y + weapon_offset);

        if (vars::esp.skeleton)
            skeleton(draw_list, player.base);
    }
}


void esp::name(ImDrawList* draw_list, c_player* player, const ImVec2& box_min, const ImVec2& box_max)
{
    const char* text = player->get_name();
    if (!tahoma_font || !text[0])
        return;

    const ImVec2 text_size = tahoma_font->CalcTextSizeA(tahoma_font->FontSize, FLT_MAX, 0.0f, text);
    const float x = floorf(box_min.x + (box_max.x - box_min.x - text_size.x) * 0.5f);
    const float y = floorf(box_min.y - text_size.y - 2.0f);

    draw_list->AddText(tahoma_font, tahoma_font->FontSize, ImVec2(x + 1.0f, y + 1.0f), IM_COL32(0, 0, 0, 255), text);
    draw_list->AddText(tahoma_font, tahoma_font->FontSize, ImVec2(x, y), IM_COL32(255, 255, 255, 255), text);
}

void esp::weapon(ImDrawList* draw_list, c_player* player, const ImVec2& box_min, const ImVec2& box_max)
{
    const char* model_path = engine::model_name(player->get_weapon_model_index());

    static bool last_icons = false;
    if (last_icons != vars::esp.weapon_icons)
    {
        console::log("[esp] weapon icons toggled %d", vars::esp.weapon_icons ? 1 : 0);
        last_icons = vars::esp.weapon_icons;
    }

    if (!tahoma_font || !model_path || !model_path[0])
        return;

    const char* token = model_path;
    for (const char* cursor = model_path; *cursor; cursor++)
        if (*cursor == '/')
            token = cursor + 1;

    if ((token[0] == 'w' || token[0] == 'p') && token[1] == '_')
        token += 2;

    if (vars::esp.weapon_icons)
    {
        const bool icon_drawn = sprites::draw(draw_list, token, box_min, box_max);
        if (icon_drawn)
            return;

        const char* nickname = player->get_name();
        if (!nickname || !nickname[0])
            return;

        const ImVec2 text_size = tahoma_font->CalcTextSizeA(tahoma_font->FontSize, FLT_MAX, 0.0f, nickname);
        const float x = floorf(box_min.x + (box_max.x - box_min.x - text_size.x) * 0.5f);
        const float y = floorf(box_max.y + 2.0f);

        draw_list->AddText(tahoma_font, tahoma_font->FontSize, ImVec2(x + 1.0f, y + 1.0f), IM_COL32(0, 0, 0, 255), nickname);
        draw_list->AddText(tahoma_font, tahoma_font->FontSize, ImVec2(x, y), IM_COL32(255, 255, 255, 255), nickname);
        return;
    }

    char text[32];
    int length = 0;
    while (token[length] && token[length] != '.' && length < 31)
    {
        const char character = token[length];
        text[length] = (character >= 'a' && character <= 'z') ? character - 32 : character;
        length++;
    }

    text[length] = '\0';
    if (!text[0])
        return;

    const ImVec2 text_size = tahoma_font->CalcTextSizeA(tahoma_font->FontSize, FLT_MAX, 0.0f, text);
    const float x = floorf(box_min.x + (box_max.x - box_min.x - text_size.x) * 0.5f);
    const float y = floorf(box_max.y + 2.0f);

    draw_list->AddText(tahoma_font, tahoma_font->FontSize, ImVec2(x + 1.0f, y + 1.0f), IM_COL32(0, 0, 0, 255), text);
    draw_list->AddText(tahoma_font, tahoma_font->FontSize, ImVec2(x, y), IM_COL32(255, 255, 255, 255), text);
}

void esp::distance(ImDrawList* draw_list, float meters, const ImVec2& box_min, const ImVec2& box_max, float y)
{
    if (!tahoma_font)
        return;

    char text[32];
    snprintf(text, sizeof(text), "%.0fm", meters);

    const ImVec2 text_size = tahoma_font->CalcTextSizeA(tahoma_font->FontSize, FLT_MAX, 0.0f, text);
    const float x = floorf(box_min.x + (box_max.x - box_min.x - text_size.x) * 0.5f);

    draw_list->AddText(tahoma_font, tahoma_font->FontSize, ImVec2(x + 1.0f, y + 1.0f), IM_COL32(0, 0, 0, 255), text);
    draw_list->AddText(tahoma_font, tahoma_font->FontSize, ImVec2(x, y), IM_COL32(255, 255, 255, 255), text);
}

void esp::corners(ImDrawList* draw_list, const ImVec2& box_min, const ImVec2& box_max)
{
    const float length = fminf(fabsf(box_max.x - box_min.x) * 0.25f, 24.0f);
    const ImU32 black = IM_COL32(0, 0, 0, 160);
    const ImU32 white = IM_COL32(255, 255, 255, 255);

    const ImVec2 segments[8][2] =
    {
        { ImVec2(box_min.x, box_min.y), ImVec2(box_min.x + length, box_min.y) },
        { ImVec2(box_min.x, box_min.y), ImVec2(box_min.x, box_min.y + length) },
        { ImVec2(box_max.x, box_min.y), ImVec2(box_max.x - length, box_min.y) },
        { ImVec2(box_max.x, box_min.y), ImVec2(box_max.x, box_min.y + length) },
        { ImVec2(box_min.x, box_max.y), ImVec2(box_min.x + length, box_max.y) },
        { ImVec2(box_min.x, box_max.y), ImVec2(box_min.x, box_max.y - length) },
        { ImVec2(box_max.x, box_max.y), ImVec2(box_max.x - length, box_max.y) },
        { ImVec2(box_max.x, box_max.y), ImVec2(box_max.x, box_max.y - length) },
    };

    for (const auto& segment : segments)
    {
        draw_list->AddLine(segment[0], segment[1], black, 3.0f);
        draw_list->AddLine(segment[0], segment[1], white, 1.0f);
    }
}

void esp::skeleton(ImDrawList* draw_list, cl_entity_t* entity)
{
    studiohdr_t* header = studio::header(entity->index);
    const int count = studio::bone_count(entity->index);
    if (!header || count <= 1)
        return;

    mstudiobone_t* bones = studio::bone_list(header);

    const ImU32 color = IM_COL32(255, 255, 255, 220);

    for (int index = 1; index < count; index++)
    {
        const int parent = bones[index].parent;
        if (parent < 0 || parent >= count)
            continue;

        const bone_matrix_t* child = studio::bone(entity->index, index);
        const bone_matrix_t* parent_bone = studio::bone(entity->index, parent);
        if (!child || !parent_bone)
            continue;

        const vec3_t child_origin = child->origin();
        const vec3_t parent_origin = parent_bone->origin();
        if (!child_origin.is_valid() || !parent_origin.is_valid())
            continue;

        const vec3_t world_segment = child_origin - parent_origin;
        if (world_segment.length_sqr() < 0.01f)
            continue;

        vec2_t child_screen;
        vec2_t parent_screen;
        if (!render::wts(child_origin, child_screen) || !render::wts(parent_origin, parent_screen))
            continue;

        const float screen_x = child_screen.x - parent_screen.x;
        const float screen_y = child_screen.y - parent_screen.y;
        if (screen_x * screen_x + screen_y * screen_y < 1.0f)
            continue;

        draw_list->AddLine(ImVec2(child_screen.x, child_screen.y), ImVec2(parent_screen.x, parent_screen.y), color, 1.0f);
    }
}