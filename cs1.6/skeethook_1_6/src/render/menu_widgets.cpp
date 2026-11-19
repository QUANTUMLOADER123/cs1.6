#include <cstddef>
#include <cmath>
#include <string>
#include <vector>
#include <crt/crt.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <render/menu_widgets.hpp>

bool menu::tab(const char* label, bool selected)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, label_end);
    const ImVec2 pos = window->DC.CursorPos;

    const float padding_x = 5.0f;
    const float padding_y = 5.0f;
    const float rounding = 5.0f;
    const float bottom_extension = 10.0f;
    const ImRect rect(pos, ImVec2(pos.x + label_size.x + padding_x * 2, pos.y + label_size.y + padding_y * 2));

    ImGui::ItemSize(ImVec2(rect.Max.x - rect.Min.x + 2.0f, rect.Max.y - rect.Min.y), style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);

    float& fade_alpha = tab_fade_alpha[id];
    fade_alpha += selected ? 3.0f * ImGui::GetIO().DeltaTime : -3.0f * ImGui::GetIO().DeltaTime;
    if (fade_alpha < 0.0f)
        fade_alpha = 0.0f;
    else if (fade_alpha > 1.0f)
        fade_alpha = 1.0f;

    const int current_alpha = static_cast<int>(ImLerp(0.0f, 255.0f, fade_alpha));

    auto color_it = tab_color_anim.find(id);
    if (color_it == tab_color_anim.end())
        color_it = tab_color_anim.emplace(id, ImColor(105, 105, 105, 255)).first;

    ImColor& current_color = color_it->second;
    ImColor target_color = ImColor(105, 105, 105, 255);
    if (selected)
        target_color = ImColor(main_color);

    const float speed = ImClamp(2.5f * ImGui::GetIO().DeltaTime, 0.01f, 1.0f);
    current_color = ImColor(
        ImLerp(current_color.Value.x * 255.0f, target_color.Value.x * 255.0f, speed) / 255.0f,
        ImLerp(current_color.Value.y * 255.0f, target_color.Value.y * 255.0f, speed) / 255.0f,
        ImLerp(current_color.Value.z * 255.0f, target_color.Value.z * 255.0f, speed) / 255.0f,
        ImLerp(current_color.Value.w * 255.0f, target_color.Value.w * 255.0f, speed) / 255.0f);

    const ImVec2 text_pos = ImVec2(rect.Min.x + padding_x, rect.Min.y + padding_y);

    if (current_alpha > 0)
    {
        const ImVec2 filled_max = ImVec2(rect.Max.x, rect.Max.y + bottom_extension);
        window->DrawList->AddRectFilled(rect.Min, filled_max, ImColor(8, 8, 8, current_alpha));

        const ImColor outline_color = ImColor(27, 27, 27, current_alpha);
        const ImColor accent_color = ImColor(main_color.x, main_color.y, main_color.z, current_alpha / 255.0f);

        window->DrawList->AddLine(ImVec2(rect.Min.x, rect.Max.y), ImVec2(rect.Min.x, rect.Min.y + rounding), outline_color);
        window->DrawList->AddBezierCubic(ImVec2(rect.Min.x, rect.Min.y + rounding), ImVec2(rect.Min.x, rect.Min.y), ImVec2(rect.Min.x + rounding, rect.Min.y), ImVec2(rect.Min.x + rounding, rect.Min.y), outline_color, 1.0f);

        window->DrawList->AddLine(ImVec2(rect.Max.x, rect.Max.y), ImVec2(rect.Max.x, rect.Min.y + rounding), outline_color);
        window->DrawList->AddBezierCubic(ImVec2(rect.Max.x, rect.Min.y + rounding), ImVec2(rect.Max.x, rect.Min.y), ImVec2(rect.Max.x - rounding, rect.Min.y), ImVec2(rect.Max.x - rounding, rect.Min.y), outline_color, 1.0f);

        window->DrawList->AddLine(ImVec2(rect.Min.x + rounding, rect.Min.y), ImVec2(rect.Max.x - rounding, rect.Min.y), accent_color);
        window->DrawList->AddBezierCubic(ImVec2(rect.Min.x, rect.Min.y + rounding), ImVec2(rect.Min.x, rect.Min.y), ImVec2(rect.Min.x + rounding, rect.Min.y), ImVec2(rect.Min.x + rounding, rect.Min.y), accent_color, 1.0f);
        window->DrawList->AddBezierCubic(ImVec2(rect.Max.x, rect.Min.y + rounding), ImVec2(rect.Max.x, rect.Min.y), ImVec2(rect.Max.x - rounding, rect.Min.y), ImVec2(rect.Max.x - rounding, rect.Min.y), accent_color, 1.0f);
    }

    window->DrawList->AddText(text_pos, current_color, label, label_end);

    return pressed;
}

bool menu::subtab(const char* label, bool selected)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, label_end);
    const ImVec2 pos = window->DC.CursorPos;

    const float padding_x = 10.0f;
    const float padding_y = 4.0f;
    const float text_padding = 10.0f;
    const ImRect rect(pos, ImVec2(pos.x + label_size.x + padding_x * 2, pos.y + label_size.y + padding_y * 2));

    ImGui::ItemSize(ImVec2(rect.Max.x - rect.Min.x + 2.0f, rect.Max.y - rect.Min.y), style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);

    auto color_it = subtab_color_anim.find(id);
    if (color_it == subtab_color_anim.end())
        color_it = subtab_color_anim.emplace(id, ImColor(105, 105, 105, 255)).first;

    ImColor& current_color = color_it->second;
    ImColor target_color = ImColor(105, 105, 105, 255);
    if (selected)
        target_color = ImColor(main_color);

    const float speed = ImClamp(3.5f * ImGui::GetIO().DeltaTime, 0.01f, 1.0f);
    current_color = ImColor(
        ImLerp(current_color.Value.x * 255.0f, target_color.Value.x * 255.0f, speed) / 255.0f,
        ImLerp(current_color.Value.y * 255.0f, target_color.Value.y * 255.0f, speed) / 255.0f,
        ImLerp(current_color.Value.z * 255.0f, target_color.Value.z * 255.0f, speed) / 255.0f,
        ImLerp(current_color.Value.w * 255.0f, target_color.Value.w * 255.0f, speed) / 255.0f);

    float& line_alpha = subtab_line_alpha[id];
    line_alpha = ImLerp(line_alpha, selected ? 255.0f : 100.0f, speed);

    const ImColor line_color = ImColor(main_color.x, main_color.y, main_color.z, line_alpha / 255.0f);

    window->DrawList->AddLine(ImVec2(rect.Min.x, rect.Min.y), ImVec2(rect.Min.x, rect.Max.y), line_color);

    const int white_line_count = 75;
    for (int i = 0; i < white_line_count; i++)
    {
        const float alpha = 12.0f - (i * (12.0f / white_line_count));
        window->DrawList->AddLine(ImVec2(rect.Min.x + 1 + i, rect.Min.y), ImVec2(rect.Min.x + 1 + i, rect.Max.y), ImColor(255, 255, 255, static_cast<int>(alpha)));
    }

    window->DrawList->AddText(ImVec2(rect.Min.x + text_padding, rect.Min.y + padding_y), current_color, label, label_end);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + rect.GetHeight() + style.ItemSpacing.y);

    return pressed;
}

void menu::menu_child(const char* title, const ImVec2& size)
{
    ImDrawList* parent_draw_list = ImGui::GetWindowDrawList();
    const ImVec2 parent_pos = ImGui::GetWindowPos();

    ImGui::BeginChild(title, size, ImGuiChildFlags_None);

    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 render_size = ImGui::GetWindowSize();

    ImRect clip_rect(pos, pos + render_size);
    parent_draw_list->PushClipRect(clip_rect.Min, clip_rect.Max);

    parent_draw_list->AddRectFilled(pos + ImVec2(0, 5), pos + ImVec2(render_size.x, 25), ImColor(11, 11, 11));

    const int fade_line_count = 150;
    const float gradient_length = render_size.x / 4.0f;
    for (int i = 0; i < fade_line_count; i++)
    {
        const float alpha = 0.075f - (i * (0.075f / fade_line_count));
        const ImVec2 start_left = pos + ImVec2(i * (gradient_length / fade_line_count), 25);
        const ImVec2 end_left = pos + ImVec2((i + 1) * (gradient_length / fade_line_count), 25);
        const ImVec2 start_right = pos + ImVec2(render_size.x - i * (gradient_length / fade_line_count), 25);
        const ImVec2 end_right = pos + ImVec2(render_size.x - (i + 1) * (gradient_length / fade_line_count), 25);

        parent_draw_list->AddLine(start_left, end_left, ImColor(1.0f, 1.0f, 1.0f, alpha));
        parent_draw_list->AddLine(start_right, end_right, ImColor(1.0f, 1.0f, 1.0f, alpha));
    }

    parent_draw_list->AddRect(pos + ImVec2(0, 5), pos + render_size, ImColor(255, 255, 255, static_cast<int>(0.50f * 23)));
    parent_draw_list->AddText(pos + ImVec2(5, 7), ImColor(255, 255, 255, 100), title);

    parent_draw_list->PopClipRect();

    ImGui::InvisibleButton("spacing", ImVec2(render_size.x, 30));
}

void menu::end_menu_child()
{
    ImGui::EndChild();
}

void menu::center_drop(int x, int y, const ImU32 color)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const float fx = static_cast<float>(x);
    const float fy = static_cast<float>(y);
    draw_list->AddRectFilled(ImVec2(fx, fy), ImVec2(fx + 2, fy + 2), color);
    draw_list->AddRectFilled(ImVec2(fx + 3, fy), ImVec2(fx + 5, fy + 2), color);
    draw_list->AddRectFilled(ImVec2(fx + 6, fy), ImVec2(fx + 8, fy + 2), color);
    draw_list->AddRectFilled(ImVec2(fx, fy + 3), ImVec2(fx + 2, fy + 5), color);
    draw_list->AddRectFilled(ImVec2(fx + 3, fy + 3), ImVec2(fx + 5, fy + 5), color);
    draw_list->AddRectFilled(ImVec2(fx + 6, fy + 3), ImVec2(fx + 8, fy + 5), color);
    draw_list->AddRectFilled(ImVec2(fx, fy + 6), ImVec2(fx + 2, fy + 8), color);
    draw_list->AddRectFilled(ImVec2(fx + 3, fy + 6), ImVec2(fx + 5, fy + 8), color);
    draw_list->AddRectFilled(ImVec2(fx + 6, fy + 6), ImVec2(fx + 8, fy + 8), color);
}

bool menu::checkbox(const char* label, bool* value)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, label_end);

    const float check_size = 8.0f;
    const float line_height = 16.0f;
    const float pad = 10.0f;
    const ImVec2 pos(window->WorkRect.Min.x + pad, window->DC.CursorPos.y + (line_height - check_size) * 0.5f);
    const ImRect check_bb(pos, pos + ImVec2(check_size, check_size));
    const ImRect total_bb(pos, ImVec2(pos.x + line_height, pos.y + line_height));

    ImGui::ItemSize(ImVec2(0.0f, line_height), style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
        *value = !*value;

    float& check_t = checkbox_check_anim[id];
    check_t = ImLerp(check_t, *value ? 1.0f : 0.0f, ImClamp(3.0f * ImGui::GetIO().DeltaTime, 0.01f, 1.0f));

    float& hover_t = checkbox_hover_anim[id];
    hover_t = ImLerp(hover_t, hovered ? 1.0f : 0.0f, ImClamp(3.0f * ImGui::GetIO().DeltaTime, 0.01f, 1.0f));

    if (check_t > 0.01f)
        window->DrawList->AddRectFilled(check_bb.Min, check_bb.Max, ImColor(main_color.x, main_color.y, main_color.z, check_t));
    else
        window->DrawList->AddRectFilled(check_bb.Min, check_bb.Max, ImColor(32, 32, 32));

    if (hover_t > 0.01f)
        window->DrawList->AddRectFilled(check_bb.Min, check_bb.Max, ImColor(main_color.x, main_color.y, main_color.z, 100.0f / 255.0f * hover_t));

    const ImColor gradient_top(8, 8, 8, 0);
    const ImColor gradient_bottom(8, 8, 8, static_cast<int>(120 * check_t));
    window->DrawList->AddRectFilledMultiColor(check_bb.Min, check_bb.Max, gradient_top, gradient_bottom, gradient_bottom, gradient_top);

    window->DrawList->AddRect(check_bb.Min, check_bb.Max, ImColor(0, 0, 0, 100));

    if (label_size.x > 0.0f)
    {
        const ImVec2 text_pos(check_bb.Max.x + 4.0f, pos.y + (check_size - label_size.y) * 0.5f);
        const ImColor text_col = ImColor(
            ImLerp(112.0f / 255.0f, 200.0f / 255.0f, check_t),
            ImLerp(112.0f / 255.0f, 200.0f / 255.0f, check_t),
            ImLerp(112.0f / 255.0f, 200.0f / 255.0f, check_t), 1.0f);
        window->DrawList->AddText(text_pos, text_col, label, label_end);

        if (hover_t > 0.01f)
        {
            const ImColor hover_col = ImColor(
                ImLerp(112.0f / 255.0f, 200.0f / 255.0f, hover_t),
                ImLerp(112.0f / 255.0f, 200.0f / 255.0f, hover_t),
                ImLerp(112.0f / 255.0f, 200.0f / 255.0f, hover_t), 100 * hover_t / 255.0f);
            window->DrawList->AddText(text_pos, hover_col, label, label_end);
        }
    }

    return pressed;
}

bool menu::combo(const char* label, int* current, const char* const items[], int items_count)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, label_end);

    const float pad = 10.0f;
    const float height = 20.0f;
    const float width = window->WorkRect.GetWidth() - pad * 2.0f;
    const ImVec2 label_pos(window->WorkRect.Min.x + pad, window->DC.CursorPos.y);

    window->DrawList->AddText(label_pos, ImColor(200, 200, 200), label, label_end);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + label_size.y + 4.0f);

    const ImVec2 pos(window->WorkRect.Min.x + pad, window->DC.CursorPos.y);
    const ImRect rect(pos, ImVec2(pos.x + width, pos.y + height));

    ImGui::ItemSize(rect.GetSize(), 0.0f);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);
    if (pressed)
        ImGui::OpenPopupEx(id, ImGuiPopupFlags_None);

    const bool is_open = ImGui::IsPopupOpen(id, ImGuiPopupFlags_None);

    const ImColor bg_from(23, 23, 23);
    const ImColor bg_to(15, 15, 15);
    window->DrawList->AddRectFilledMultiColor(rect.Min, rect.Max, bg_from, bg_to, bg_to, bg_from);

    const ImColor text_col = is_open ? ImColor(200, 200, 200) : ImColor(112, 112, 112);
    const char* preview = (*current >= 0 && *current < items_count) ? items[*current] : "";
    const ImVec2 preview_size = ImGui::CalcTextSize(preview);
    window->DrawList->PushClipRect(ImVec2(rect.Min.x + 4.0f, rect.Min.y), ImVec2(rect.Max.x - 20.0f, rect.Max.y), true);
    window->DrawList->AddText(ImVec2(rect.Min.x + 4.0f, rect.Min.y + (height - preview_size.y) * 0.5f), text_col, preview);
    window->DrawList->PopClipRect();

    const float arrow_x = rect.Max.x - 15.0f;
    const ImColor arrow_col = is_open ? ImColor(200, 200, 200) : ImColor(112, 112, 112);
    center_drop(static_cast<int>(arrow_x - 4.0f), static_cast<int>(rect.Min.y + (height - 8.0f) * 0.5f), arrow_col);

    window->DrawList->AddRect(rect.Min, rect.Max, ImColor(0, 0, 0, 100));

    bool changed = false;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(13.0f / 255.0f, 13.0f / 255.0f, 13.0f / 255.0f, 1.0f));
    if (ImGui::IsPopupOpen(id, ImGuiPopupFlags_None))
        ImGui::SetNextWindowPos(ImVec2(rect.Min.x, rect.Max.y), ImGuiCond_Always);
    if (ImGui::BeginPopupEx(id, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        ImDrawList* popup_draw_list = ImGui::GetWindowDrawList();

        const int max_visible = 6;
        int first = 0;
        if (items_count > max_visible)
        {
            int& scroll = listbox_scroll[id];
            const int max_scroll = items_count - max_visible;
            if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f)
                scroll = ImClamp(scroll + static_cast<int>(ImGui::GetIO().MouseWheel), 0, max_scroll);
            first = ImClamp(scroll, 0, max_scroll);
        }
        const int visible_count = ImMin(items_count - first, max_visible);

        const float full_height = static_cast<float>(visible_count) * 18.0f + 3.0f;
        ImGui::SetWindowSize(ImVec2(width, full_height));

        const ImVec2 field_min(rect.Min.x, rect.Max.y);
        popup_draw_list->AddRectFilled(field_min, ImVec2(rect.Max.x, field_min.y + full_height), ImColor(13, 13, 13));
        popup_draw_list->AddRect(field_min, ImVec2(rect.Max.x, field_min.y + full_height), ImColor(0, 0, 0, 150));

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));

        for (int offset = 0; offset < visible_count; offset++)
        {
            const int index = first + offset;
            const ImVec2 item_pos = ImGui::GetCursorScreenPos();

            ImGui::PushID(index);
            const bool clicked = ImGui::InvisibleButton("item", ImVec2(width, 18.0f));
            ImGui::PopID();
            if (clicked)
            {
                *current = index;
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            const bool selected = (*current == index);
            const ImVec2 item_text_size = ImGui::CalcTextSize(items[index]);
            const ImColor item_color = selected ? ImColor(main_color.x, main_color.y, main_color.z, 1.0f) : ImColor(150, 150, 150);
            popup_draw_list->AddText(ImVec2(item_pos.x + 5.0f, item_pos.y + (18.0f - item_text_size.y) * 0.5f), item_color, items[index]);
        }
        ImGui::PopStyleVar();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    return changed;
}

bool menu::multi_combo(const char* label, bool values[], const char* const items[], int items_count)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, label_end);

    const float pad = 10.0f;
    const float height = 20.0f;
    const float width = window->WorkRect.GetWidth() - pad * 2.0f;
    const ImVec2 label_pos(window->WorkRect.Min.x + pad, window->DC.CursorPos.y);

    window->DrawList->AddText(label_pos, ImColor(200, 200, 200), label, label_end);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + label_size.y + 4.0f);

    const ImVec2 pos(window->WorkRect.Min.x + pad, window->DC.CursorPos.y);
    const ImRect rect(pos, ImVec2(pos.x + width, pos.y + height));

    ImGui::ItemSize(rect.GetSize(), 0.0f);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);
    if (pressed)
        ImGui::OpenPopupEx(id, ImGuiPopupFlags_None);

    const bool is_open = ImGui::IsPopupOpen(id, ImGuiPopupFlags_None);

    char preview[256];
    int preview_length = 0;
    for (int index = 0; index < items_count; index++)
    {
        if (!values[index])
            continue;

        const int available = IM_ARRAYSIZE(preview) - preview_length;
        const int written = ImFormatString(preview + preview_length, static_cast<size_t>(available), preview_length ? ", %s" : "%s", items[index]);
        if (written >= available)
        {
            ImFormatString(preview + preview_length, static_cast<size_t>(available), "...");
            break;
        }
        preview_length += written;
    }
    if (preview_length == 0)
        ImFormatString(preview, IM_ARRAYSIZE(preview), "-");

    const ImColor bg_from(23, 23, 23);
    const ImColor bg_to(15, 15, 15);
    window->DrawList->AddRectFilledMultiColor(rect.Min, rect.Max, bg_from, bg_to, bg_to, bg_from);

    const ImColor text_col = is_open ? ImColor(200, 200, 200) : ImColor(112, 112, 112);
    const ImVec2 preview_size = ImGui::CalcTextSize(preview);
    const float preview_max_width = width - 26.0f;
    int preview_clamped = preview_length;
    while (preview_clamped > 0 && ImGui::CalcTextSize(preview, preview + preview_clamped).x > preview_max_width)
    {
        preview_clamped--;
    }
    const bool preview_truncated = preview_clamped < preview_length;

    window->DrawList->PushClipRect(ImVec2(rect.Min.x + 4.0f, rect.Min.y), ImVec2(rect.Max.x - 20.0f, rect.Max.y), true);
    window->DrawList->AddText(ImVec2(rect.Min.x + 4.0f, rect.Min.y + (height - preview_size.y) * 0.5f), text_col, preview, preview + preview_clamped);
    if (preview_truncated)
    {
        const ImVec2 dots_size = ImGui::CalcTextSize("...");
        window->DrawList->AddText(ImVec2(rect.Max.x - 24.0f - dots_size.x, rect.Min.y + (height - preview_size.y) * 0.5f), text_col, "...");
    }
    window->DrawList->PopClipRect();

    const float arrow_x = rect.Max.x - 15.0f;
    const ImColor arrow_col = is_open ? ImColor(200, 200, 200) : ImColor(112, 112, 112);
    center_drop(static_cast<int>(arrow_x - 4.0f), static_cast<int>(rect.Min.y + (height - 8.0f) * 0.5f), arrow_col);

    window->DrawList->AddRect(rect.Min, rect.Max, ImColor(0, 0, 0, 100));

    bool changed = false;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(13.0f / 255.0f, 13.0f / 255.0f, 13.0f / 255.0f, 1.0f));
    if (ImGui::IsPopupOpen(id, ImGuiPopupFlags_None))
        ImGui::SetNextWindowPos(ImVec2(rect.Min.x, rect.Max.y), ImGuiCond_Always);
    if (ImGui::BeginPopupEx(id, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        ImDrawList* popup_draw_list = ImGui::GetWindowDrawList();

        const int max_visible = 6;
        int first = 0;
        if (items_count > max_visible)
        {
            int& scroll = listbox_scroll[id];
            const int max_scroll = items_count - max_visible;
            if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f)
                scroll = ImClamp(scroll + static_cast<int>(ImGui::GetIO().MouseWheel), 0, max_scroll);
            first = ImClamp(scroll, 0, max_scroll);
        }
        const int visible_count = ImMin(items_count - first, max_visible);

        const float full_height = static_cast<float>(visible_count) * 18.0f + 3.0f;
        ImGui::SetWindowSize(ImVec2(width, full_height));

        const ImVec2 field_min(rect.Min.x, rect.Max.y);
        popup_draw_list->AddRectFilled(field_min, ImVec2(rect.Max.x, field_min.y + full_height), ImColor(13, 13, 13));
        popup_draw_list->AddRect(field_min, ImVec2(rect.Max.x, field_min.y + full_height), ImColor(0, 0, 0, 150));

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));

        for (int offset = 0; offset < visible_count; offset++)
        {
            const int index = first + offset;
            const ImVec2 item_pos = ImGui::GetCursorScreenPos();

            ImGui::PushID(index);
            const bool clicked = ImGui::InvisibleButton("item", ImVec2(width, 18.0f));
            ImGui::PopID();
            if (clicked)
            {
                values[index] = !values[index];
                changed = true;
            }

            const ImVec2 item_text_size = ImGui::CalcTextSize(items[index]);
            const ImColor item_color = values[index] ? ImColor(main_color.x, main_color.y, main_color.z, 1.0f) : ImColor(150, 150, 150);
            popup_draw_list->AddText(ImVec2(item_pos.x + 5.0f, item_pos.y + (18.0f - item_text_size.y) * 0.5f), item_color, items[index]);
        }
        ImGui::PopStyleVar();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    return changed;
}

bool menu::slider_float(const char* label, float* value, float min, float max, const char* format)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, label_end);

    char value_text[64];
    ImFormatString(value_text, IM_ARRAYSIZE(value_text), format, *value);
    const ImVec2 value_size = ImGui::CalcTextSize(value_text);

    const float pad = 10.0f;
    const float height = 8.0f;
    const float width = window->WorkRect.GetWidth() - pad * 2.0f;
    const float label_y = window->DC.CursorPos.y;

    window->DrawList->AddText(ImVec2(window->WorkRect.Min.x + pad, label_y), ImColor(200, 200, 200), label, label_end);
    window->DrawList->AddText(ImVec2(window->WorkRect.Max.x - pad - value_size.x, label_y), ImColor(200, 200, 200), value_text);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + label_size.y + 2.0f);

    const ImVec2 pos(window->WorkRect.Min.x + pad, window->DC.CursorPos.y);
    const ImRect rect(pos, ImVec2(pos.x + width, pos.y + height));

    ImGui::ItemSize(rect.GetSize(), 0.0f);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool held;
    ImGui::ButtonBehavior(rect, id, nullptr, &held);

    bool changed = false;
    if (held && rect.GetWidth() > 0.0f)
    {
        const float ratio = ImClamp((g.IO.MousePos.x - rect.Min.x) / rect.GetWidth(), 0.0f, 1.0f);
        const float new_value = min + ratio * (max - min);
        if (new_value != *value)
        {
            *value = new_value;
            changed = true;
        }
    }
    if (changed)
        ImGui::MarkItemEdited(id);

    const float ratio = (max > min) ? ImClamp((*value - min) / (max - min), 0.0f, 1.0f) : 0.0f;

    window->DrawList->AddRectFilled(rect.Min, rect.Max, ImColor(32, 32, 32));

    if (ratio > 0.0f)
    {
        const ImVec2 fill_max(rect.Min.x + rect.GetWidth() * ratio, rect.Max.y);
        window->DrawList->AddRectFilled(rect.Min, fill_max, ImColor(main_color.x, main_color.y, main_color.z, 1.0f));

        const ImColor gradient_from(8, 8, 8, 0);
        const ImColor gradient_to(8, 8, 8, 115);
        window->DrawList->AddRectFilledMultiColor(rect.Min, fill_max, gradient_from, gradient_to, gradient_to, gradient_from);
    }

    window->DrawList->AddRect(rect.Min, rect.Max, ImColor(0, 0, 0, 100));

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

    return changed;
}

bool menu::slider_int(const char* label, int* value, int min, int max, const char* format)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, label_end);

    char value_text[64];
    ImFormatString(value_text, IM_ARRAYSIZE(value_text), format, *value);
    const ImVec2 value_size = ImGui::CalcTextSize(value_text);

    const float pad = 10.0f;
    const float height = 8.0f;
    const float width = window->WorkRect.GetWidth() - pad * 2.0f;
    const float label_y = window->DC.CursorPos.y;

    window->DrawList->AddText(ImVec2(window->WorkRect.Min.x + pad, label_y), ImColor(200, 200, 200), label, label_end);
    window->DrawList->AddText(ImVec2(window->WorkRect.Max.x - pad - value_size.x, label_y), ImColor(200, 200, 200), value_text);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + label_size.y + 2.0f);

    const ImVec2 pos(window->WorkRect.Min.x + pad, window->DC.CursorPos.y);
    const ImRect rect(pos, ImVec2(pos.x + width, pos.y + height));

    ImGui::ItemSize(rect.GetSize(), 0.0f);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool held;
    ImGui::ButtonBehavior(rect, id, nullptr, &held);

    bool changed = false;
    if (held && rect.GetWidth() > 0.0f)
    {
        const float ratio = ImClamp((g.IO.MousePos.x - rect.Min.x) / rect.GetWidth(), 0.0f, 1.0f);
        const int new_value = static_cast<int>(std::lround(min + ratio * (max - min)));
        if (new_value != *value)
        {
            *value = new_value;
            changed = true;
        }
    }
    if (changed)
        ImGui::MarkItemEdited(id);

    const float ratio = (max > min) ? ImClamp(static_cast<float>(*value - min) / static_cast<float>(max - min), 0.0f, 1.0f) : 0.0f;

    window->DrawList->AddRectFilled(rect.Min, rect.Max, ImColor(32, 32, 32));

    if (ratio > 0.0f)
    {
        const ImVec2 fill_max(rect.Min.x + rect.GetWidth() * ratio, rect.Max.y);
        window->DrawList->AddRectFilled(rect.Min, fill_max, ImColor(main_color.x, main_color.y, main_color.z, 1.0f));

        const ImColor gradient_from(8, 8, 8, 0);
        const ImColor gradient_to(8, 8, 8, 115);
        window->DrawList->AddRectFilledMultiColor(rect.Min, fill_max, gradient_from, gradient_to, gradient_to, gradient_from);
    }

    window->DrawList->AddRect(rect.Min, rect.Max, ImColor(0, 0, 0, 100));

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

    return changed;
}

bool menu::color_picker(const char* label, float* color, bool alpha)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, label_end);

    const float pad = 10.0f;
    const float height = 20.0f;
    const float width = window->WorkRect.GetWidth() - pad * 2.0f;
    const ImVec2 label_pos(window->WorkRect.Min.x + pad, window->DC.CursorPos.y);

    window->DrawList->AddText(label_pos, ImColor(200, 200, 200), label, label_end);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + label_size.y + 4.0f);

    const ImVec2 pos(window->WorkRect.Min.x + pad, window->DC.CursorPos.y);
    const ImRect rect(pos, ImVec2(pos.x + width, pos.y + height));

    ImGui::ItemSize(rect.GetSize(), 0.0f);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);
    if (pressed)
        ImGui::OpenPopupEx(id, ImGuiPopupFlags_None);

    const ImColor bg_from(23, 23, 23);
    const ImColor bg_to(15, 15, 15);
    window->DrawList->AddRectFilledMultiColor(rect.Min, rect.Max, bg_from, bg_to, bg_to, bg_from);

    const ImVec2 swatch_min(rect.Max.x - 21.0f, rect.Min.y + 5.0f);
    const ImVec2 swatch_max(swatch_min.x + 16.0f, swatch_min.y + 10.0f);
    window->DrawList->AddRectFilled(swatch_min, swatch_max, ImColor(color[0], color[1], color[2], alpha ? color[3] : 1.0f));
    window->DrawList->AddRect(swatch_min, swatch_max, ImColor(0, 0, 0, 100));
    window->DrawList->AddRect(ImVec2(swatch_min.x - 1.0f, swatch_min.y - 1.0f), ImVec2(swatch_max.x + 1.0f, swatch_max.y + 1.0f), ImColor(main_color.x, main_color.y, main_color.z, 1.0f));

    window->DrawList->AddRect(rect.Min, rect.Max, ImColor(0, 0, 0, 100));

    bool changed = false;
    if (ImGui::BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
    {
        float rgba[4] = { color[0], color[1], color[2], alpha ? color[3] : 1.0f };
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_PickerHueBar;
        if (!alpha)
            flags |= ImGuiColorEditFlags_NoAlpha;

        if (ImGui::ColorPicker4("##picker", rgba, flags))
        {
            color[0] = rgba[0];
            color[1] = rgba[1];
            color[2] = rgba[2];
            if (alpha)
                color[3] = rgba[3];
            changed = true;
        }
        ImGui::EndPopup();
    }

    return changed;
}

bool menu::listbox(const std::vector<std::string>& items, int* selected)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID("##configs_listbox");

    const float pad = 10.0f;
    const float width = window->WorkRect.GetWidth() - pad * 2.0f;

    const int max_visible = 6;
    const int items_count = static_cast<int>(items.size());
    int& scroll = listbox_scroll[id];
    const int max_scroll = ImMax(items_count - max_visible, 0);
    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f)
        scroll = ImClamp(scroll + static_cast<int>(ImGui::GetIO().MouseWheel), 0, max_scroll);
    scroll = ImClamp(scroll, 0, max_scroll);

    const float visible_height = static_cast<float>(max_visible) * 20.0f;
    const ImVec2 pos(window->WorkRect.Min.x + pad, window->DC.CursorPos.y);
    const ImRect rect(pos, ImVec2(pos.x + width, pos.y + visible_height));

    ImGui::ItemSize(rect.GetSize(), 0.0f);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    const ImColor bg_from(23, 23, 23);
    const ImColor bg_to(15, 15, 15);
    window->DrawList->AddRectFilledMultiColor(rect.Min, rect.Max, bg_from, bg_to, bg_to, bg_from);

    bool changed = false;
    for (int offset = 0; offset < max_visible; offset++)
    {
        const int index = scroll + offset;
        if (index >= items_count)
            break;

        const ImRect item_rect(rect.Min + ImVec2(0.0f, static_cast<float>(offset) * 20.0f), rect.Min + ImVec2(width, static_cast<float>(offset + 1) * 20.0f));

        ImGui::PushID(index);
        bool hovered, held;
        const bool clicked = ImGui::ButtonBehavior(item_rect, window->GetID("item"), &hovered, &held);
        ImGui::PopID();
        if (clicked)
        {
            *selected = index;
            changed = true;
        }

        const bool picked = (*selected == index);
        if (hovered || picked)
            window->DrawList->AddRectFilled(item_rect.Min, item_rect.Max, picked ? ImColor(main_color.x, main_color.y, main_color.z, 0.25f) : ImColor(28, 28, 28));

        const ImColor item_color = picked ? ImColor(main_color.x, main_color.y, main_color.z, 1.0f) : (hovered ? ImColor(200, 200, 200) : ImColor(150, 150, 150));
        const ImVec2 text_size = ImGui::CalcTextSize(items[index].c_str());
        window->DrawList->AddText(ImVec2(item_rect.Min.x + 6.0f, item_rect.Min.y + (20.0f - text_size.y) * 0.5f), item_color, items[index].c_str());
    }

    window->DrawList->AddRect(rect.Min, rect.Max, ImColor(0, 0, 0, 100));

    return changed;
}

bool menu::button(const char* label)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label);

    const float pad = 10.0f;
    const float height = 22.0f;
    const float width = window->WorkRect.GetWidth() - pad * 2.0f;

    const ImVec2 pos(window->WorkRect.Min.x + pad, window->DC.CursorPos.y);
    const ImRect rect(pos, ImVec2(pos.x + width, pos.y + height));

    ImGui::ItemSize(rect.GetSize(), 0.0f);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);

    const ImColor bg_from(23, 23, 23);
    const ImColor bg_to(15, 15, 15);
    window->DrawList->AddRectFilledMultiColor(rect.Min, rect.Max, bg_from, bg_to, bg_to, bg_from);

    if (hovered)
        window->DrawList->AddRectFilled(rect.Min, rect.Max, ImColor(255, 255, 255, 12));

    window->DrawList->AddRect(rect.Min, rect.Max, ImColor(0, 0, 0, 100));
    window->DrawList->AddText(ImVec2(rect.Min.x + (width - label_size.x) * 0.5f, rect.Min.y + (height - label_size.y) * 0.5f), hovered ? ImColor(200, 200, 200) : ImColor(150, 150, 150), label);

    return pressed;
}

bool menu::text_input(const char* label, char* buffer, const size_t buffer_size)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, label_end);

    const float pad = 10.0f;
    const float height = 22.0f;
    const float width = window->WorkRect.GetWidth() - pad * 2.0f;
    const float label_y = window->DC.CursorPos.y;

    window->DrawList->AddText(ImVec2(window->WorkRect.Min.x + pad, label_y), ImColor(200, 200, 200), label, label_end);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + label_size.y + 2.0f);

    const ImVec2 pos(window->WorkRect.Min.x + pad, window->DC.CursorPos.y);
    const ImRect rect(pos, ImVec2(pos.x + width, pos.y + height));

    ImGui::ItemSize(rect.GetSize(), 0.0f);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);
    if (pressed)
    {
        text_input_active[id] = true;
        text_input_cursor[id] = static_cast<int>(CRT::StringLength(buffer));
    }
    else if (g.IO.MouseClicked[0] && !ImGui::IsMouseHoveringRect(rect.Min, rect.Max))
    {
        text_input_active[id] = false;
    }

    bool& active = text_input_active[id];
    int& cursor = text_input_cursor[id];
    bool changed = false;

    if (active)
    {
        const ImGuiIO& io = g.IO;
        const int length = static_cast<int>(CRT::StringLength(buffer));

        if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        {
            active = false;
            changed = true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Backspace, false) && cursor > 0)
        {
            int previous = cursor - 1;
            while (previous > 0 && (buffer[previous] & 0xC0) == 0x80)
                previous--;
            CRT::MemoryMove(buffer + previous, buffer + cursor, static_cast<size_t>(length - cursor) + 1);
            cursor = previous;
            changed = true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && cursor < length)
        {
            int next = cursor + 1;
            while (buffer[next] != 0 && (buffer[next] & 0xC0) == 0x80)
                next++;
            CRT::MemoryMove(buffer + cursor, buffer + next, static_cast<size_t>(length - next) + 1);
            changed = true;
        }

        int inserted = 0;
        for (int i = 0; i < io.InputQueueCharacters.Size; i++)
        {
            const unsigned int codepoint = static_cast<unsigned int>(io.InputQueueCharacters[i]);
            char bytes[4];
            int encoded = 0;
            if (codepoint < 0x80)
            {
                bytes[0] = static_cast<char>(codepoint);
                encoded = 1;
            }
            else if (codepoint < 0x800)
            {
                bytes[0] = static_cast<char>(0xC0 | (codepoint >> 6));
                bytes[1] = static_cast<char>(0x80 | (codepoint & 0x3F));
                encoded = 2;
            }
            else
            {
                bytes[0] = static_cast<char>(0xE0 | (codepoint >> 12));
                bytes[1] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                bytes[2] = static_cast<char>(0x80 | (codepoint & 0x3F));
                encoded = 3;
            }

            if (cursor + encoded < static_cast<int>(buffer_size) - 1)
            {
                const int current_length = length + inserted;
                CRT::MemoryMove(buffer + cursor + encoded, buffer + cursor, static_cast<size_t>(current_length - cursor) + 1);
                CRT::MemoryCopy(buffer + cursor, bytes, static_cast<size_t>(encoded));
                cursor += encoded;
                inserted += encoded;
                changed = true;
            }
        }
    }

    const ImColor bg_from(23, 23, 23);
    const ImColor bg_to(15, 15, 15);
    window->DrawList->AddRectFilledMultiColor(rect.Min, rect.Max, bg_from, bg_to, bg_to, bg_from);

    if (hovered && !active)
        window->DrawList->AddRectFilled(rect.Min, rect.Max, ImColor(255, 255, 255, 12));

    if (active)
        window->DrawList->AddRect(rect.Min, rect.Max, ImColor(main_color.x, main_color.y, main_color.z, 1.0f));
    else
        window->DrawList->AddRect(rect.Min, rect.Max, ImColor(0, 0, 0, 100));

    const ImVec2 text_pos(rect.Min.x + 6.0f, rect.Min.y + (height - ImGui::CalcTextSize("A").y) * 0.5f);
    window->DrawList->PushClipRect(rect.Min, rect.Max, true);
    window->DrawList->AddText(text_pos, ImColor(200, 200, 200), buffer);
    if (active && (static_cast<int>(ImGui::GetTime() * 2.0f) & 1))
    {
        const float caret_x = text_pos.x + ImGui::CalcTextSize(buffer, buffer + cursor).x;
        window->DrawList->AddLine(ImVec2(caret_x, text_pos.y), ImVec2(caret_x, text_pos.y + 14.0f), ImColor(200, 200, 200));
    }
    window->DrawList->PopClipRect();

    return changed;
}

bool menu::color_swatch(const char* label, float* color, bool alpha)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const float size = 10.0f;
    const float pad = 10.0f;
    const float line_height = 16.0f;
    const ImVec2 pos(window->WorkRect.Max.x - pad - size, window->DC.CursorPos.y + (line_height - size) * 0.5f);
    const ImRect rect(pos, ImVec2(pos.x + size, pos.y + size));

    ImGui::ItemSize(ImVec2(size, line_height), style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);
    if (pressed)
        ImGui::OpenPopupEx(id, ImGuiPopupFlags_None);

    window->DrawList->AddRectFilled(rect.Min, rect.Max, ImColor(color[0], color[1], color[2], alpha ? color[3] : 1.0f));
    window->DrawList->AddRect(rect.Min, rect.Max, ImColor(0, 0, 0, 100));
    if (hovered || held)
        window->DrawList->AddRect(ImVec2(rect.Min.x - 1.0f, rect.Min.y - 1.0f), ImVec2(rect.Max.x + 1.0f, rect.Max.y + 1.0f), ImColor(main_color.x, main_color.y, main_color.z, 1.0f));

    bool changed = false;
    if (ImGui::BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
    {
        float rgba[4] = { color[0], color[1], color[2], alpha ? color[3] : 1.0f };
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_PickerHueBar;
        if (!alpha)
            flags |= ImGuiColorEditFlags_NoAlpha;

        if (ImGui::ColorPicker4("##picker", rgba, flags))
        {
            color[0] = rgba[0];
            color[1] = rgba[1];
            color[2] = rgba[2];
            if (alpha)
                color[3] = rgba[3];
            changed = true;
        }
        ImGui::EndPopup();
    }

    return changed;
}