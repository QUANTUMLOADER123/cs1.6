#pragma once

struct ImDrawData;

auto ImGui_ImplOpenGL_Init() -> bool;
auto ImGui_ImplOpenGL_Shutdown() -> void;
auto ImGui_ImplOpenGL_NewFrame() -> void;
auto ImGui_ImplOpenGL_RefreshFontTexture() -> void;
auto ImGui_ImplOpenGL_RenderDrawData(ImDrawData* draw_data) -> void;
