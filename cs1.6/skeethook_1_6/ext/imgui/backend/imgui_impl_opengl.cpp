#include <imgui/imgui.h>
#include <imgui/backend/imgui_impl_opengl.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>

static bool initialized = false;
static GLuint font_texture = 0;

static void create_font_texture()
{
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    io.Fonts->SetTexID(static_cast<ImTextureID>(font_texture));
}

bool ImGui_ImplOpenGL_Init()
{
    if (initialized)
        return true;

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_opengl";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    initialized = true;

    return true;
}

void ImGui_ImplOpenGL_Shutdown()
{
    if (font_texture)
    {
        glDeleteTextures(1, &font_texture);
        font_texture = 0;
    }

    initialized = false;
}

void ImGui_ImplOpenGL_NewFrame()
{
    if (!font_texture)
        create_font_texture();
}

void ImGui_ImplOpenGL_RenderDrawData(ImDrawData* draw_data)
{
    if (!draw_data || draw_data->CmdListsCount == 0)
        return;

    const int framebuffer_height = static_cast<int>(draw_data->DisplaySize.y);

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glEnable(GL_SCISSOR_TEST);
    glShadeModel(GL_SMOOTH);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, draw_data->DisplaySize.x, draw_data->DisplaySize.y, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    for (int list = 0; list < draw_data->CmdListsCount; list++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[list];
        const ImDrawVert* vertex_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* index_buffer = cmd_list->IdxBuffer.Data;

        for (int cmd = 0; cmd < cmd_list->CmdBuffer.Size; cmd++)
        {
            const ImDrawCmd* draw_cmd = &cmd_list->CmdBuffer[cmd];

            if (draw_cmd->UserCallback)
            {
                draw_cmd->UserCallback(cmd_list, draw_cmd);
                continue;
            }

            const ImVec4& clip_rect = draw_cmd->ClipRect;
            glScissor(static_cast<int>(clip_rect.x), static_cast<int>(framebuffer_height - clip_rect.w), static_cast<int>(clip_rect.z - clip_rect.x), static_cast<int>(clip_rect.w - clip_rect.y));

            const ImTextureID texture_id = draw_cmd->GetTexID();
            if (texture_id != 0)
            {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture_id));
            }
            else
            {
                glDisable(GL_TEXTURE_2D);
            }

            glBegin(GL_TRIANGLES);

            for (unsigned int i = 0; i < draw_cmd->ElemCount; i++)
            {
                const ImDrawVert* vertex = &vertex_buffer[index_buffer[i] + draw_cmd->VtxOffset];

                glColor4ub(vertex->col & 0xFF, (vertex->col >> 8) & 0xFF, (vertex->col >> 16) & 0xFF, (vertex->col >> 24) & 0xFF);
                glTexCoord2f(vertex->uv.x, vertex->uv.y);
                glVertex2f(vertex->pos.x, vertex->pos.y);
            }

            glEnd();

            index_buffer += draw_cmd->ElemCount;
        }
    }

    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();
}
