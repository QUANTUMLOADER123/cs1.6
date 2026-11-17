// gui_icons_init.h — загрузка ОРИГИНАЛЬНОГО спрайта иконок gamesense.
//
// Спрайт tab_icons.png (384x48, 8 плиток 48x48) встроен в gui_icons_data.h,
// декодируется один раз через stb_image и регистрируется:
//   * в headless-рендерере  — как CPU-текстура (render::Texture)
//   * в DLL                 — как GL-текстура для ImGui (AddTexture)
//
// Обе ветки дают один и тот же ImTextureID в IconAtlas, поэтому DrawIcon
// работает одинаково.
#pragma once

#include "gui_widgets.h"
#include "gui_icons_data.h"
#include <cstdint>

#ifndef GUI_ICONS_NO_STB
#define STB_IMAGE_IMPLEMENTATION_ONCE
#endif

// headless-растеризатор подключает gui_render.h ДО этого файла.
#if defined(GUI_RENDER_H)
#define GUI_HAS_CPU_RENDER 1
#else
#define GUI_HAS_CPU_RENDER 0
#endif

namespace gui
{
    // Живые RGBA-пиксели спрайта: нужны DLL-бэкенду для загрузки в GL.
    inline const unsigned char*& SpritePixelsMutable()
    {
        static const unsigned char* p = nullptr;
        return p;
    }
    inline ImVec2& SpriteSizeMutable()
    {
        static ImVec2 s(0, 0);
        return s;
    }

    // Прямой доступ к декодированным RGBA-пикселям спрайта.
    struct DecodedSprite
    {
        int w = 0, h = 0;
        const unsigned char* rgba = nullptr;   // w*h*4, из stb_image (освобождать stbi_image_free)
    };
    inline DecodedSprite DecodeIconSprite()
    {
        DecodedSprite d;
        int comp = 0;
        d.rgba = stbi_load_from_memory(g_tab_icons_png, (int)g_tab_icons_png_size,
                                       &d.w, &d.h, &comp, 4);
        return d;
    }

    // Регистрирует спрайт как текстуру. Возвращает false при ошибке.
    inline bool InitIconAtlasCPU()
    {
        DecodedSprite sp = DecodeIconSprite();
        if (!sp.rgba || sp.w <= 0 || sp.h <= 0)
            return false;

        IconAtlas& at = Atlas();
        at.w = sp.w;
        at.h = sp.h;
        at.tile = sp.h;            // плитки квадратные: 48x48

#if GUI_HAS_CPU_RENDER
        // headless: заводим CPU-текстуру для программного растеризатора
        render::Texture t;
        t.w = sp.w;
        t.h = sp.h;
        t.px.resize((size_t)sp.w * sp.h);
        for (size_t i = 0; i < (size_t)sp.w * sp.h; ++i)
        {
            // Спрайт gamesense — ЧЁРНАЯ маска: RGB ~0, информация только в альфе.
            // Растеризатор/GL умножают текстуру на цвет вершины (tint), поэтому
            // RGB обязан быть белым, иначе иконка получается почти чёрной.
            const unsigned char a = sp.rgba[i * 4 + 3];
            t.px[i] = ((uint32_t)a << 24) | 0x00ffffffu;
        }
        at.tex = render::RegisterTexture(std::move(t));
        at.ready = true;
#endif
        // В DLL (GUI_HAS_CPU_RENDER == 0) текстуру создаёт сам ImGui-бэкенд:
        // пиксели отдаются через SpritePixels() и грузятся в GL в gui_icons_gl.h.
        SpritePixelsMutable() = sp.rgba;
        SpriteSizeMutable() = ImVec2((float)sp.w, (float)sp.h);
        return true;
    }
}
