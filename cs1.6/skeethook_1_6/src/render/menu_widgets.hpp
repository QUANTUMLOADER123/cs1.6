#pragma once

#include <imgui/imgui.h>

#include <map>
#include <string>
#include <vector>

namespace menu
{
    inline const ImVec4 main_color = ImVec4(111.0f / 255.0f, 108.0f / 255.0f, 123.0f / 255.0f, 1.0f);

    bool tab(const char* label, bool selected);
    bool subtab(const char* label, bool selected);
    void menu_child(const char* title, const ImVec2& size);
    void end_menu_child();
    bool checkbox(const char* label, bool* value);
    bool combo(const char* label, int* current, const char* const items[], int items_count);
    bool multi_combo(const char* label, bool values[], const char* const items[], int items_count);
    bool slider_float(const char* label, float* value, float min, float max, const char* format);
    bool slider_int(const char* label, int* value, int min, int max, const char* format);
    bool color_picker(const char* label, float* color, bool alpha);
    bool color_swatch(const char* label, float* color, bool alpha);
    bool listbox(const std::vector<std::string>& items, int* selected);
    bool button(const char* label);
    bool text_input(const char* label, char* buffer, const size_t buffer_size);
    void center_drop(int x, int y, const ImU32 color);

    inline std::map<ImGuiID, float> tab_fade_alpha;
    inline std::map<ImGuiID, ImColor> tab_color_anim;
    inline std::map<ImGuiID, ImColor> subtab_color_anim;
    inline std::map<ImGuiID, float> subtab_line_alpha;
    inline std::map<ImGuiID, float> checkbox_check_anim;
    inline std::map<ImGuiID, float> checkbox_hover_anim;
    inline std::map<ImGuiID, bool> text_input_active;
    inline std::map<ImGuiID, int> text_input_cursor;
    inline std::map<ImGuiID, int> listbox_scroll;
}