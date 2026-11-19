/* БЭКЕНД РЕНДЕРА IMGUI ДЛЯ GOLDSRC.

   Бэкенд из gold-sdk.zip оказался нерабочим в нашей обвязке: фон и
   рамки рисовались, а текст был невидим - все вершины текста шли
   с пустой текстурой.

   Причины, которые пришлось устранить:

   1. Текстура шрифта создавалась в ImGui_ImplOpenGL_NewFrame(),
      то есть ПОСЛЕ того как ImGui::NewFrame() уже сформировал
      draw-list. В лучшем случае это работало на следующий кадр,
      в худшем - команды текста уходили с TextureId = 0 и GL
      выключал текстурирование: получались чёрные прямоугольники
      вместо букв.
      => текстура создаётся в Init(), один раз и ДО первого кадра.

   2. glPushAttrib/glPopAttrib в GoldSrc ненадёжны: движок держит
      своё состояние и после нас его не восстанавливает. Поэтому
      сохраняем и восстанавливаем нужные флаги РУКАМИ.

   3. Не сбрасывался glColor - движок оставлял свой, и все вершины
      ImGui красились в чужой цвет.

   4. Не отключался GL_FOG. В CS 1.6 туман включён для карты, и он
      применялся к нашим вершинам: текст "растворялся" в фоне.

   5. Не выставлялся glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
      GL_MODULATE). По умолчанию в движке может стоять GL_DECAL или
      GL_COMBINE - тогда альфа шрифта игнорируется.
*/

#include <imgui/imgui.h>
#include <imgui/backend/imgui_impl_opengl.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>

#ifndef GL_TEXTURE_ENV
#define GL_TEXTURE_ENV 0x2300
#endif
#ifndef GL_TEXTURE_ENV_MODE
#define GL_TEXTURE_ENV_MODE 0x2200
#endif
#ifndef GL_MODULATE
#define GL_MODULATE 0x2100
#endif
#ifndef GL_FOG
#define GL_FOG 0x0B60
#endif
#ifndef GL_SCISSOR_TEST
#define GL_SCISSOR_TEST 0x0C11
#endif
#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif

static bool g_initialized;
static GLuint g_font_tex;

/* какую текстуру вернуть движку после возни с атласом */
static GLuint sh_restore_binding;

static void sh_upload_font(void)
{
    ImGuiIO& io = ImGui::GetIO();
    unsigned char *pixels = 0;
    int w = 0;
    int h = 0;

    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);

    if (!pixels || w <= 0 || h <= 0)
        return;

    /* СОХРАНЯЕМ ПРИВЯЗКУ ТЕКСТУРЫ ДВИЖКА.

       GoldSrc кэширует текущую текстуру у себя и не ждёт, что её
       кто-то переключит. Если после заливки атласа оставить
       glBindTexture(GL_TEXTURE_2D, 0), движок продолжит рисовать
       стены/оружие АТЛАСОМ ШРИФТА или пустой текстурой - на скрине
       это выглядит как "все текстуры пропали, всё белое".

       Поэтому запоминаем, что было привязано, и возвращаем. */
    {
        GLint prev_tex = 0;

        glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);

        glGenTextures(1, &g_font_tex);
        glBindTexture(GL_TEXTURE_2D, g_font_tex);

        sh_restore_binding = (GLuint)prev_tex;
    }

    /* GL_CLAMP, не GL_CLAMP_TO_EDGE: последнего нет в старых
       ICD, а GoldSrc ходит через GL 1.1 */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    /* СБРОС ПАРАМЕТРОВ РАСПАКОВКИ.

       GoldSrc активно грузит текстуры и выставляет glPixelStorei
       под себя, в том числе GL_UNPACK_ROW_LENGTH. Если эта величина
       останется ненулевой, драйвер прочитает наш атлас с ЧУЖИМ
       шагом строки: получится мусор или вообще ничего - текст
       "исчезает", хотя глифы в атласе есть.

       Поэтому выставляем все четыре параметра явно, а не только
       выравнивание. */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);

    /* возвращаем движку его текстуру, а не 0 */
    glBindTexture(GL_TEXTURE_2D, sh_restore_binding);

    io.Fonts->SetTexID((ImTextureID)(unsigned long)g_font_tex);
}

bool ImGui_ImplOpenGL_Init(void)
{
    if (g_initialized)
        return true;

    ImGuiIO& io = ImGui::GetIO();

    io.BackendRendererName = "gold_src_opengl";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    /* ВАЖНО: атлас должен быть построен ДО GetTexDataAsRGBA32.
       Если AddFont... вызывали после, атлас пустой. */
    if (!io.Fonts->TexID)
        io.Fonts->Build();

    sh_upload_font();

    g_initialized = true;

    return g_font_tex != 0;
}

void ImGui_ImplOpenGL_Shutdown(void)
{
    if (g_font_tex)
    {
        glDeleteTextures(1, &g_font_tex);
        g_font_tex = 0;
    }

    g_initialized = false;
}

/* Перезалить атлас шрифта после пересборки (смена масштаба).
   Старую текстуру удаляем: иначе они копятся при каждом ресайзе. */
void ImGui_ImplOpenGL_RefreshFontTexture(void)
{
    GLint bound = 0;

    /* если атлас всё ещё привязан - отвязываем перед удалением:
       glDeleteTextures на привязанной текстуре оставляет GL в
       состоянии "привязана к имени, которого уже нет", и
       следующая отрисовка идёт пустой текстурой - всё белое. */
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);

    if (g_font_tex && (GLuint)bound == g_font_tex)
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        bound = 0;
    }

    if (g_font_tex)
    {
        GLuint old = g_font_tex;

        g_font_tex = 0;
        glDeleteTextures(1, &old);
    }

    ImGui::GetIO().Fonts->SetTexID(0);

    sh_upload_font();

    /* восстановить привязку движка */
    if (bound && (GLuint)bound != g_font_tex)
        glBindTexture(GL_TEXTURE_2D, (GLuint)bound);
}

void ImGui_ImplOpenGL_NewFrame(void)
{
    if (!g_font_tex)
        sh_upload_font();
}

void ImGui_ImplOpenGL_RenderDrawData(ImDrawData *draw_data)
{
    if (!draw_data || draw_data->CmdListsCount == 0)
        return;

    /* РЕАЛЬНЫЙ ВЬЮПОРТ, А НЕ io->DisplaySize.

       Это была причина и "огромного меню", и невидимого текста.

       io->DisplaySize приходит из GetScreenInfo движка (например
       2560x1440), а GL-вьюпорт при этом может быть 1916x1028 -
       масштаб/оконный режим/DPI. На скрине меню 911 px вместо
       задуманных 1216, то есть рисовалось с коэффициентом 0.748
       по ОБЕИМ осям: 1916/2560 = 0.7485.

       Чем это ломало текст: glScissor пересчитывается через
       fb_h (из DisplaySize), а вьюпорт другой высоты - область
       отсечения уезжает вверх, и все мелкие квады (буквы,
       чекбоксы, строки вкладок) режутся ЦЕЛИКОМ. Большой фон
       окна частично выживает - ровно то, что видно на скрине:
       чёрный прямоугольник без единого символа.

       Поэтому берём размеры у GL и масштабируем координаты ImGui
       в реальные пиксели. */
    GLint vp[4] = { 0, 0, 0, 0 };

    glGetIntegerv(GL_VIEWPORT, vp);

    if (vp[2] <= 0 || vp[3] <= 0)
    {
        vp[2] = (GLint)draw_data->DisplaySize.x;
        vp[3] = (GLint)draw_data->DisplaySize.y;
    }

    const float fb_w = (float)vp[2];
    const float fb_h = (float)vp[3];

    /* во сколько раз реальные пиксели отличаются от логических */
    float kx = 1.0f;
    float ky = 1.0f;

    if (draw_data->DisplaySize.x > 0.0f && draw_data->DisplaySize.y > 0.0f)
    {
        kx = fb_w / draw_data->DisplaySize.x;
        ky = fb_h / draw_data->DisplaySize.y;

        /* защита от безумных значений (свёрнутое окно и т.п.) */
        if (kx < 0.2f || kx > 5.0f)
            kx = 1.0f;

        if (ky < 0.2f || ky > 5.0f)
            ky = 1.0f;
    }

    /* --- сохраняем то, что трогаем; glPushAttrib в GoldSrc
           восстанавливает не всё, поэтому явно --- */
    /* ЕЩЁ ОДНА ПРИЧИНА БЕЛЫХ ТЕКСТУР.

       ИСХОДНО: после прохода ImGui остаётся привязанным АТЛАС
       ШРИФТА. GoldSrc кэширует свои привязки и не переустанавливает
       их перед каждой отрисовкой - он считает, что текстура стены
       всё ещё активна. В результате стены, оружие и модели рисуются
       атласом шрифта: на экране "всё белое".

       Поэтому запоминаем привязку ДО и возвращаем ПОСЛЕ. */
    GLint prev_binding = 0;

    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_binding);

    GLboolean was_blend = glIsEnabled(GL_BLEND);
    GLboolean was_tex = glIsEnabled(GL_TEXTURE_2D);
    GLboolean was_depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean was_cull = glIsEnabled(GL_CULL_FACE);
    GLboolean was_light = glIsEnabled(GL_LIGHTING);
    GLboolean was_alpha = glIsEnabled(GL_ALPHA_TEST);
    GLboolean was_fog = glIsEnabled(GL_FOG);
    GLboolean was_scissor = glIsEnabled(GL_SCISSOR_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);          /* иначе туман карты съедает текст */
    glDisable(GL_COLOR_LOGIC_OP);
    glEnable(GL_SCISSOR_TEST);

    glShadeModel(GL_SMOOTH);
    glColor4ub(255, 255, 255, 255);   /* сброс чужого цвета */

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, fb_w, fb_h, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    for (int n = 0; n < draw_data->CmdListsCount; ++n)
    {
        const ImDrawList *list = draw_data->CmdLists[n];
        const ImDrawVert *vtx = list->VtxBuffer.Data;
        const ImDrawIdx *idx = list->IdxBuffer.Data;

        for (int c = 0; c < list->CmdBuffer.Size; ++c)
        {
            const ImDrawCmd *cmd = &list->CmdBuffer[c];

            if (cmd->UserCallback)
            {
                cmd->UserCallback(list, cmd);
                continue;
            }

            if (cmd->ElemCount == 0)
                continue;

            {
                /* scissor в реальных пикселях, снизу вверх.

                   ClipRect задан в логических координатах ImGui,
                   поэтому умножаем на коэффициент пересчёта.
                   Раньше fb_h брался из DisplaySize, и вся область
                   отсечения уезжала - мелкие квады исчезали. */
                int sx = (int)(cmd->ClipRect.x * kx);
                int sy = (int)(fb_h - cmd->ClipRect.w * ky);
                int sw = (int)((cmd->ClipRect.z - cmd->ClipRect.x) * kx);
                int sh = (int)((cmd->ClipRect.w - cmd->ClipRect.y) * ky);

                if (sw < 0) sw = 0;
                if (sh < 0) sh = 0;
                if (sy < 0) sy = 0;

                glScissor(sx, sy, sw, sh);
            }

            {
                ImTextureID tex = cmd->GetTexID();

                if (tex != 0)
                {
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D,
                                  (GLuint)(unsigned long)tex);
                }
                else
                {
                    glDisable(GL_TEXTURE_2D);
                }
            }

            glBegin(GL_TRIANGLES);

            for (unsigned int i = 0; i < cmd->ElemCount; ++i)
            {
                const ImDrawVert *v =
                    &vtx[idx[i] + cmd->VtxOffset];

                glColor4ub((GLubyte)(v->col & 0xFF),
                           (GLubyte)((v->col >> 8) & 0xFF),
                           (GLubyte)((v->col >> 16) & 0xFF),
                           (GLubyte)((v->col >> 24) & 0xFF));

                glTexCoord2f(v->uv.x, v->uv.y);
                /* логические координаты -> реальные пиксели */
                glVertex2f(v->pos.x * kx, v->pos.y * ky);
            }

            glEnd();

            idx += cmd->ElemCount;
        }
    }

    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    /* --- вернуть состояние движка --- */
    if (was_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (was_tex) glEnable(GL_TEXTURE_2D); else glDisable(GL_TEXTURE_2D);
    if (was_depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (was_cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (was_light) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
    if (was_alpha) glEnable(GL_ALPHA_TEST); else glDisable(GL_ALPHA_TEST);
    if (was_fog) glEnable(GL_FOG); else glDisable(GL_FOG);

    if (was_scissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);

    /* вернуть движку ЕГО текстуру */
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev_binding);
}
