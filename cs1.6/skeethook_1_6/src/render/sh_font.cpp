#include "../../include/sh_font.h"
#include "../../include/sh_render.h"

#include <windows.h>
#include <math.h>

typedef void(APIENTRY *gl_gen_textures_fn)(i32 n, u32 *ids);
typedef void(APIENTRY *gl_bind_texture_fn)(u32 target, u32 id);
typedef void(APIENTRY *gl_tex_image2d_fn)(u32 target, i32 level, i32 internal,
                                          i32 w, i32 h, i32 border, u32 fmt,
                                          u32 type, const void *pixels);
typedef void(APIENTRY *gl_tex_parameteri_fn)(u32 target, u32 name, i32 v);
typedef void(APIENTRY *gl_pixel_storei_fn)(u32 name, i32 v);
typedef void(APIENTRY *gl_tex_env_fn)(u32 target, u32 name, i32 v);
typedef void(APIENTRY *gl_matrix_mode_fn)(u32 mode);
typedef void(APIENTRY *gl_load_identity_fn)(void);
typedef void(APIENTRY *gl_enable_fn)(u32 cap);
typedef void(APIENTRY *gl_disable_fn)(u32 cap);
typedef void(APIENTRY *gl_begin_fn)(u32 mode);
typedef void(APIENTRY *gl_end_fn)(void);
typedef void(APIENTRY *gl_vertex2f_fn)(f32 x, f32 y);
typedef void(APIENTRY *gl_tex2f_fn)(f32 s, f32 t);
typedef void(APIENTRY *gl_color4ub_fn)(u8 r, u8 g, u8 b, u8 a);

static gl_gen_textures_fn p_gen;
static gl_bind_texture_fn p_bind;
static gl_tex_image2d_fn p_img;
static gl_tex_parameteri_fn p_param;
static gl_pixel_storei_fn p_store;
static gl_tex_env_fn p_env;
static gl_matrix_mode_fn p_mmode;
static gl_load_identity_fn p_ident;
static gl_enable_fn p_enable;
static gl_disable_fn p_disable;
static gl_begin_fn p_begin;
static gl_end_fn p_end;
static gl_vertex2f_fn p_v2f;
static gl_tex2f_fn p_t2f;
static gl_color4ub_fn p_c4ub;

static HMODULE g_gl;
static u32 g_atlas;
static i32 g_tex_w;
static i32 g_tex_h;
static f32 g_cell_w;
static f32 g_cell_h;

#define sh_gl_texture_2d 0x0DE1
#define sh_gl_quads 0x0007
#define sh_gl_rgba 0x1908
#define sh_gl_unsigned_byte 0x1401
#define sh_gl_unpack_alignment 0x0CF5
#define sh_gl_texture_min_filter 0x2801
#define sh_gl_texture_mag_filter 0x2800
#define sh_gl_texture_wrap_s 0x2802
#define sh_gl_texture_wrap_t 0x2803
#define sh_gl_nearest 0x2600
#define sh_gl_clamp 0x812F
#define sh_gl_modulate 0x2100
#define sh_gl_blend_bit 0x0BE2
#define sh_gl_projection 0x1701
#define sh_gl_modelview 0x1700
#define sh_gl_texture_env 0x2200
#define sh_gl_texture_env_mode 0x2200

static void *sh_proc(const char *n)
{
    if (!g_gl)
        g_gl = LoadLibraryA("opengl32.dll");
    return g_gl ? (void *)GetProcAddress(g_gl, n) : 0;
}

/* РЕНДЕР ШРИФТА ЧЕРЕЗ GDI.

   Раньше тут был встроенный пиксельный шрифт 8x10 из
   sh_font_data.cpp - отсюда и "пиксельный шрифт", который просили
   убрать. Теперь атлас рисует сама Windows: берём Tahoma, включаем
   сглаживание (CLEARTYPE_QUALITY) и раскладываем 95 глифов в
   текстуру 512x256.

   Тонкость: в DLL нет ресурсов и нет гарантии, что рядом лежит
   .ttf, поэтому используем системный шрифт по имени - Tahoma есть
   на любой Windows начиная с 2000, и выглядит заметно лучше
   дефолтного MS Sans Serif.

   Создание шрифта и DC делаем ОДИН раз при инициализации: этот код
   вызывается из HUD_Redraw, а GDI в каждом кадре - это дорого. */

#define sh_font_first 32
#define sh_font_count 95
#define sh_font_atlas_cols 16
#define sh_font_px 14

static u8 *sh_font_rasterize(i32 *out_w, i32 *out_h, i32 *out_cell_w,
                             i32 *out_cell_h)
{
    HDC dc = CreateCompatibleDC(0);
    HBITMAP bmp = 0;
    HFONT font = 0;
    HFONT old_font = 0;
    HBITMAP old_bmp = 0;
    u8 *bits = 0;
    u8 *pixels = 0;

    i32 cell_w = sh_font_px + 2;
    i32 cell_h = sh_font_px + 4;
    i32 tex_w = sh_font_atlas_cols * cell_w;
    i32 rows = (sh_font_count + sh_font_atlas_cols - 1) / sh_font_atlas_cols;
    i32 tex_h = rows * cell_h;

    BITMAPINFO bi;
    void *raw = 0;

    if (!dc)
        return 0;

    /* 32-битный DIB сверху вниз: у него alpha=0 по умолчанию, а GDI
       пишет только RGB - поэтому фон заливаем чёрным и потом берём
       ЯРКОСТЬ как альфу (шрифт белый на чёрном). */
    for (i32 i = 0; i < (i32)sizeof(bi); ++i)
        ((u8 *)&bi)[i] = 0;

    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = tex_w;
    bi.bmiHeader.biHeight = -tex_h;      /* минус = сверху вниз */
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = 0;      /* BI_RGB */

    bmp = CreateDIBSection(dc, &bi, 0, &raw, 0, 0);

    if (!bmp || !raw)
        goto done;

    old_bmp = (HBITMAP)SelectObject(dc, bmp);

    /* чёрный фон */
    {
        RECT rc;

        rc.left = 0;
        rc.top = 0;
        rc.right = tex_w;
        rc.bottom = tex_h;
        SetBkColor(dc, RGB(0, 0, 0));
        SetBkMode(dc, OPAQUE);
        ExtTextOutA(dc, 0, 0, 0, &rc, 0, 0, 0);
    }

    font = CreateFontA(-sh_font_px, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0,
                       DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
                       "Tahoma");

    if (!font)
        font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    if (!font)
        goto done;

    old_font = (HFONT)SelectObject(dc, font);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));

    for (i32 i = 0; i < sh_font_count; ++i)
    {
        char ch = (char)(sh_font_first + i);
        i32 cx = (i % sh_font_atlas_cols) * cell_w;
        i32 cy = (i / sh_font_atlas_cols) * cell_h;

        if (ch == ' ')
            continue;

        TextOutA(dc, cx + 1, cy + 1, &ch, 1);
    }

    GdiFlush();

    bits = (u8 *)raw;

    pixels = (u8 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                             (SIZE_T)(tex_w * tex_h * 4));

    if (!pixels)
        goto done;

    /* BGR от GDI -> RGBA с альфой из яркости */
    for (i32 i = 0; i < tex_w * tex_h; ++i)
    {
        const u8 *src = bits + i * 4;

        u8 lum = src[0] > src[1] ? src[0] : src[1];

        if (src[2] > lum)
            lum = src[2];

        pixels[i * 4 + 0] = 255;
        pixels[i * 4 + 1] = 255;
        pixels[i * 4 + 2] = 255;
        pixels[i * 4 + 3] = lum;
    }

done:
    if (old_font)
        SelectObject(dc, old_font);

    if (font)
        DeleteObject(font);

    if (old_bmp)
        SelectObject(dc, old_bmp);

    if (bmp)
        DeleteObject(bmp);

    DeleteDC(dc);

    if (pixels)
    {
        *out_w = tex_w;
        *out_h = tex_h;
        *out_cell_w = cell_w;
        *out_cell_h = cell_h;
    }

    return pixels;
}

qbool sh_font_init(void)
{
    p_gen = (gl_gen_textures_fn)sh_proc("glGenTextures");
    p_bind = (gl_bind_texture_fn)sh_proc("glBindTexture");
    p_img = (gl_tex_image2d_fn)sh_proc("glTexImage2D");
    p_param = (gl_tex_parameteri_fn)sh_proc("glTexParameteri");
    p_store = (gl_pixel_storei_fn)sh_proc("glPixelStorei");
    p_env = (gl_tex_env_fn)sh_proc("glTexEnvi");
    p_mmode = (gl_matrix_mode_fn)sh_proc("glMatrixMode");
    p_ident = (gl_load_identity_fn)sh_proc("glLoadIdentity");
    p_enable = (gl_enable_fn)sh_proc("glEnable");
    p_disable = (gl_disable_fn)sh_proc("glDisable");
    p_begin = (gl_begin_fn)sh_proc("glBegin");
    p_end = (gl_end_fn)sh_proc("glEnd");
    p_v2f = (gl_vertex2f_fn)sh_proc("glVertex2f");
    p_t2f = (gl_tex2f_fn)sh_proc("glTexCoord2f");
    p_c4ub = (gl_color4ub_fn)sh_proc("glColor4ub");

    if (!p_gen || !p_bind || !p_img || !p_begin || !p_end || !p_v2f || !p_t2f)
        return false;

    {
        i32 tw = 0, th = 0, cw = 0, ch = 0;

        u8 *pixels = sh_font_rasterize(&tw, &th, &cw, &ch);

        if (!pixels)
            return false;

        g_tex_w = tw;
        g_tex_h = th;
        g_cell_w = (f32)cw;
        g_cell_h = (f32)ch;

        p_gen(1, &g_atlas);
        p_bind(sh_gl_texture_2d, g_atlas);
        p_store(sh_gl_unpack_alignment, 1);
        p_img(sh_gl_texture_2d, 0, 4, g_tex_w, g_tex_h, 0, sh_gl_rgba,
              sh_gl_unsigned_byte, pixels);
        /* линейная фильтрация: иначе сглаженный GDI-шрифт снова
           станет рваным при масштабировании */
        p_param(sh_gl_texture_2d, sh_gl_texture_min_filter, 0x2601);
        p_param(sh_gl_texture_2d, sh_gl_texture_mag_filter, 0x2601);
        p_param(sh_gl_texture_2d, sh_gl_texture_wrap_s, sh_gl_clamp);
        p_param(sh_gl_texture_2d, sh_gl_texture_wrap_t, sh_gl_clamp);
        p_bind(sh_gl_texture_2d, 0);

        HeapFree(GetProcessHeap(), 0, pixels);
    }

    return true;
}

void sh_font_shutdown(void)
{
    if (g_atlas && p_gen)
    {
        typedef void(APIENTRY * del_fn)(i32, const u32 *);
        del_fn del = (del_fn)sh_proc("glDeleteTextures");
        if (del)
            del(1, &g_atlas);
        g_atlas = 0;
    }
}

void sh_font_bind(void)
{
    if (!p_bind || !g_atlas)
        return;
    p_bind(sh_gl_texture_2d, g_atlas);
}

f32 sh_font_width(const char *str, f32 scale)
{
    if (!str)
        return 0.0f;

    i32 n = 0;

    while (str[n])
        ++n;

    return (f32)n * (f32)sh_font_px * 0.56f * scale;
}

void sh_font_draw(f32 x, f32 y, f32 scale, col4_t col, const char *str)
{
    if (!str || !g_atlas || !p_begin)
        return;

    f32 cw = g_cell_w / (f32)g_tex_w;
    f32 ch = g_cell_h / (f32)g_tex_h;

    p_enable(sh_gl_texture_2d);
    sh_font_bind();
    p_env(sh_gl_texture_env, sh_gl_texture_env_mode, sh_gl_modulate);

    f32 w = (f32)sh_font_px * scale;
    f32 h = (f32)sh_font_px * scale;
    f32 pen = x;

    for (const char *p = str; *p; ++p)
    {
        u8 code = (u8)*p;

        if (code < sh_font_first || code >= sh_font_first + sh_font_count)
            code = '?';

        if (code == ' ')
        {
            pen += w * 0.56f;
            continue;
        }

        {
            i32 idx = code - sh_font_first;
            i32 cx = idx % sh_font_atlas_cols;
            i32 cy = idx / sh_font_atlas_cols;

            f32 u0 = (f32)cx * cw;
            f32 v0 = (f32)cy * ch;
            f32 u1 = u0 + (f32)g_cell_w / (f32)g_tex_w;
            f32 v1 = v0 + (f32)g_cell_h / (f32)g_tex_h;

            p_c4ub(col.r, col.g, col.b, col.a);
            p_begin(sh_gl_quads);
            p_t2f(u0, v0);
            p_v2f(pen, y);
            p_t2f(u1, v0);
            p_v2f(pen + w, y);
            p_t2f(u1, v1);
            p_v2f(pen + w, y + h);
            p_t2f(u0, v1);
            p_v2f(pen, y + h);
            p_end();

            pen += w * 0.56f;
        }
    }

    p_disable(sh_gl_texture_2d);
}
