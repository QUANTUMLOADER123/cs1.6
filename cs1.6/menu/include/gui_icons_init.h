// gui_icons_init.h — загрузка ОРИГИНАЛЬНОГО спрайта иконок gamesense.
//
// Спрайт tab_icons.png (384x48, 8 плиток 48x48) встроен в gui_icons_data.h.
// Здесь он декодируется один раз через stb_image и регистрируется:
//   * в headless-рендерере — как CPU-текстура для программного растеризатора
//   * в DLL — как GL-текстура для ImGui
#pragma once

#include "gui_widgets.h"
#include "gui_icons_data.h"
#include <cstdint>
#include <vector>

#ifndef GUI_ICONS_NO_STB
#define STB_IMAGE_IMPLEMENTATION_ONCE
#endif

namespace gui
{
    // Декодирует PNG и регистрирует его как CPU-текстуру.
    // Возвращает true при успехе. TexID — указатель-идентификатор.
    inline bool InitIconAtlasCPU()
    {
        int w = 0, h = 0, comp = 0;
        unsigned char* pix = stbi_load_from_memory(g_tab_icons_png,
                                                   (int)g_tab_icons_png_size,
                                                   &w, &h, &comp, 4);
        if (!pix || w <= 0 || h <= 0)
            return false;

        render::Texture t;
        t.w = w;
        t.h = h;
        t.px.resize((size_t)w * h);
        for (size_t i = 0; i < (size_t)w * h; ++i)
        {
            const unsigned char r = pix[i * 4 + 0];
            const unsigned char g = pix[i * 4 + 1];
            const unsigned char b = pix[i * 4 + 2];
            const unsigned char a = pix[i * 4 + 3];
            t.px[i] = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
        }
        stbi_image_free(pix);

        const ImTextureID tex_id = render::RegisterTexture(std::move(t));

        IconAtlas& at = Atlas();
        at.w = w;
        at.h = h;
        at.tile = h;              // плитки квадратные: 48x48
        at.tex = tex_id;
        at.ready = true;
        return true;
    }
}
