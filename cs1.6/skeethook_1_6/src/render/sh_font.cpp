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

    g_cell_w = (f32)g_font_8x10_w + 1.0f;
    g_cell_h = (f32)g_font_8x10_h;

    i32 cols = 16;
    i32 rows = (g_font_8x10_count + cols - 1) / cols;
    g_tex_w = cols * (i32)g_cell_w;
    g_tex_h = rows * (i32)g_cell_h;

    u8 *pixels = (u8 *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                 (SIZE_T)(g_tex_w * g_tex_h * 4));
    if (!pixels)
        return false;

    for (i32 i = 0; i < g_font_8x10_count; ++i)
    {
        i32 cx = (i % cols) * (i32)g_cell_w;
        i32 cy = (i / cols) * (i32)g_cell_h;

        for (i32 y = 0; y < g_font_8x10_h; ++y)
        {
            u8 row = g_font_8x10[i][y];
            for (i32 x = 0; x < g_font_8x10_w; ++x)
            {
                qbool on = (row & (1 << (g_font_8x10_w - 1 - x))) != 0;
                i32 px = cx + x;
                i32 py = cy + y;
                if (px >= g_tex_w || py >= g_tex_h)
                    continue;
                u8 *dst = pixels + (py * g_tex_w + px) * 4;
                dst[0] = 255;
                dst[1] = 255;
                dst[2] = 255;
                dst[3] = on ? 255 : 0;
            }
        }
    }

    p_gen(1, &g_atlas);
    p_bind(sh_gl_texture_2d, g_atlas);
    p_store(sh_gl_unpack_alignment, 1);
    p_img(sh_gl_texture_2d, 0, 4, g_tex_w, g_tex_h, 0, sh_gl_rgba,
          sh_gl_unsigned_byte, pixels);
    p_param(sh_gl_texture_2d, sh_gl_texture_min_filter, sh_gl_nearest);
    p_param(sh_gl_texture_2d, sh_gl_texture_mag_filter, sh_gl_nearest);
    p_param(sh_gl_texture_2d, sh_gl_texture_wrap_s, sh_gl_clamp);
    p_param(sh_gl_texture_2d, sh_gl_texture_wrap_t, sh_gl_clamp);
    p_bind(sh_gl_texture_2d, 0);

    HeapFree(GetProcessHeap(), 0, pixels);
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
    return (f32)n * (f32)g_font_8x10_w * scale;
}

void sh_font_draw(f32 x, f32 y, f32 scale, col4_t col, const char *str)
{
    if (!str || !g_atlas || !p_begin)
        return;

    i32 cols = 16;
    f32 cw = g_cell_w / (f32)g_tex_w;
    f32 ch = g_cell_h / (f32)g_tex_h;

    p_enable(sh_gl_texture_2d);
    sh_font_bind();
    p_env(sh_gl_texture_env, sh_gl_texture_env_mode, sh_gl_modulate);

    f32 pen = x;
    for (const char *p = str; *p; ++p)
    {
        u8 code = (u8)*p;
        if (code < g_font_8x10_first || code >= g_font_8x10_first + g_font_8x10_count)
            code = '?';
        if (code == ' ')
        {
            pen += (f32)g_font_8x10_w * scale;
            continue;
        }

        i32 idx = code - g_font_8x10_first;
        i32 cx = idx % cols;
        i32 cy = idx / cols;

        f32 u0 = (f32)cx * cw;
        f32 v0 = (f32)cy * ch;
        f32 u1 = u0 + (f32)g_font_8x10_w / (f32)g_tex_w;
        f32 v1 = v0 + (f32)g_font_8x10_h / (f32)g_tex_h;

        f32 w = (f32)g_font_8x10_w * scale;
        f32 h = (f32)g_font_8x10_h * scale;

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

        pen += w;
    }

    p_disable(sh_gl_texture_2d);
}
