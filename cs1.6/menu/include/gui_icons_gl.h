// gui_icons_gl.h — загрузка спрайта иконок в GL для DLL-версии.
//
// Вызывается ОДИН раз после создания GL-контекста и ImGui-бэкенда.
// Пиксели берутся из gui_icons_init.h (stb_image, RGBA8888).
#pragma once

#include "gui_widgets.h"
#include "gui_icons_init.h"
#include <GL/gl.h>

namespace gui
{
    // Создаёт GL-текстуру из живых пикселей спрайта и прописывает её в IconAtlas.
    // Требует, чтобы GL-контекст уже был текущим.
    inline bool InitIconAtlasGL()
    {
        const unsigned char* px = SpritePixelsMutable();
        const ImVec2 sz = SpriteSizeMutable();
        if (!px || sz.x <= 0 || sz.y <= 0)
            return false;

        GLuint id = 0;
        glGenTextures(1, &id);
        if (!id)
            return false;
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)sz.x, (GLsizei)sz.y, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, px);

        IconAtlas& at = Atlas();
        at.tex = (ImTextureID)(intptr_t)id;

        at.w = (int)sz.x;
        at.h = (int)sz.y;
        at.tile = (int)sz.y;
        at.ready = true;
        return true;
    }
}
