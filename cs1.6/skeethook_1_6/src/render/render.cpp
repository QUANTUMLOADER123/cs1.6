#include <cstdio>
#include <imgui/imgui.h>
#include <imgui/backend/imgui_impl_opengl.h>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/interfaces/renderer/renderer.hpp>
#include <core/input.hpp>
#include <render/render.hpp>
#include <render/menu.hpp>
#include <render/menu_widgets.hpp>
#include <features/players/esp/esp.hpp>
#include <features/world/removals/removals.hpp>
#include <features/world/bullet_tracers/bullet_tracers.hpp>
#include <features/legitbot/legitbot.hpp>

static bool imgui_initialized = false;

ImFont* tahoma_font = nullptr;

void render::apply_style()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(6.0f, 2.0f);
    style.ScrollbarSize = 10.0f;
    style.GrabMinSize = 8.0f;

    const float accent_x = menu::main_color.x;
    const float accent_y = menu::main_color.y;
    const float accent_z = menu::main_color.z;

    colors[ImGuiCol_Text] = ImVec4(235.0f / 255.0f, 235.0f / 255.0f, 235.0f / 255.0f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(120.0f / 255.0f, 120.0f / 255.0f, 120.0f / 255.0f, 1.0f);
    colors[ImGuiCol_WindowBg] = ImVec4(8.0f / 255.0f, 8.0f / 255.0f, 8.0f / 255.0f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(12.0f / 255.0f, 12.0f / 255.0f, 12.0f / 255.0f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(35.0f / 255.0f, 35.0f / 255.0f, 35.0f / 255.0f, 1.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(20.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(28.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(34.0f / 255.0f, 34.0f / 255.0f, 34.0f / 255.0f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(12.0f / 255.0f, 12.0f / 255.0f, 12.0f / 255.0f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(16.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(12.0f / 255.0f, 12.0f / 255.0f, 12.0f / 255.0f, 1.0f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(12.0f / 255.0f, 12.0f / 255.0f, 12.0f / 255.0f, 1.0f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(40.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(accent_x, accent_y, accent_z, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(accent_x, accent_y, accent_z, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(accent_x, accent_y, accent_z, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(accent_x, accent_y, accent_z, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(20.0f / 255.0f, 20.0f / 255.0f, 20.0f / 255.0f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(30.0f / 255.0f, 30.0f / 255.0f, 30.0f / 255.0f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(accent_x, accent_y, accent_z, 0.45f);
    colors[ImGuiCol_Header] = ImVec4(accent_x, accent_y, accent_z, 0.35f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(accent_x, accent_y, accent_z, 0.55f);
    colors[ImGuiCol_HeaderActive] = ImVec4(accent_x, accent_y, accent_z, 0.75f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(accent_x, accent_y, accent_z, 0.2f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(accent_x, accent_y, accent_z, 0.5f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(accent_x, accent_y, accent_z, 0.8f);
    colors[ImGuiCol_PlotLines] = ImVec4(accent_x, accent_y, accent_z, 1.0f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(accent_x, accent_y, accent_z, 1.0f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(accent_x, accent_y, accent_z, 1.0f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(accent_x, accent_y, accent_z, 1.0f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(accent_x, accent_y, accent_z, 0.35f);
    colors[ImGuiCol_NavHighlight] = ImVec4(accent_x, accent_y, accent_z, 1.0f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
}

bool render::initialize(HDC device_context)
{
    window = WindowFromDC(device_context);
    if (!window)
        return false;

    ImGui::CreateContext();
    apply_style();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    if (!ImGui_ImplOpenGL_Init())
        return false;

    ImFontConfig config;

    tahoma_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", 13.0f, &config, io.Fonts->GetGlyphRangesCyrillic());

    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    imgui_initialized = true;

    return true;
}

void render::shutdown()
{
    if (!imgui_initialized)
        return;

    imgui_initialized = false;

    ImGui_ImplOpenGL_Shutdown();
    ImGui::DestroyContext();

    window = nullptr;
    tahoma_font = nullptr;
}

void render::draw(HDC device_context)
{
    if (!imgui_initialized && !initialize(device_context))
        return;

    if (!window)
        return;

    RECT client_rect;
    GetClientRect(window, &client_rect);

    width = client_rect.right;
    height = client_rect.bottom;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    io.DeltaTime = 1.0f / 60.0f;

    ImGui_ImplOpenGL_NewFrame();
    input::update();

    ImGui::NewFrame();

    esp::draw();
    removals::render();
    bullet_tracers::render();
    legitbot::render();
    menu::render();

    ImGui::Render();
    ImGui_ImplOpenGL_RenderDrawData(ImGui::GetDrawData());
}

bool render::wts(const vec3_t& world, vec2_t& screen)
{
    return renderer::world_to_screen(world, screen);
}
