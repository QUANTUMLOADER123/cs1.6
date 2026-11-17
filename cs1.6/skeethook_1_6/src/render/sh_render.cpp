#include "../../include/sh_render.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_font.h"

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#define sh_pi_f 3.14159265358979323846f

typedef void(APIENTRY *gl_vertex2f_fn)(f32 x, f32 y);
typedef void(APIENTRY *gl_color4ub_fn)(u8 r, u8 g, u8 b, u8 a);
typedef void(APIENTRY *gl_begin_fn)(u32 mode);
typedef void(APIENTRY *gl_end_fn)(void);
typedef void(APIENTRY *gl_line_width_fn)(f32 w);
typedef void(APIENTRY *gl_enable_fn)(u32 cap);
typedef void(APIENTRY *gl_disable_fn)(u32 cap);
typedef void(APIENTRY *gl_blend_fn)(u32 src, u32 dst);
typedef void(APIENTRY *gl_tex2f_fn)(f32 s, f32 t);

static gl_vertex2f_fn p_gl_vertex2f;
static gl_color4ub_fn p_gl_color4ub;
static gl_begin_fn p_gl_begin;
static gl_end_fn p_gl_end;
static gl_line_width_fn p_gl_line_width;
static gl_enable_fn p_gl_enable;
static gl_disable_fn p_gl_disable;
static gl_blend_fn p_gl_blend;
static gl_tex2f_fn p_gl_tex2f;

static HMODULE g_gl;
static i32 g_width;
static i32 g_height;
static i32 g_text_depth;

#define sh_gl_quads 0x0007
#define sh_gl_lines 0x0001
#define sh_gl_line_strip 0x0003
#define sh_gl_triangle_fan 0x0006
#define sh_gl_texture_2d 0x0DE1
#define sh_gl_blend_bit 0x0BE2
#define sh_gl_src_alpha 0x0302
#define sh_gl_one_minus_src_alpha 0x0303

void sh_render_init(void);
qbool sh_render_ready(void);

static void *sh_gl_proc(const char *name)
{
    if (!g_gl)
        g_gl = LoadLibraryA("opengl32.dll");
    if (!g_gl)
        return 0;
    return (void *)GetProcAddress(g_gl, name);
}

void sh_render_init(void)
{
    p_gl_vertex2f = (gl_vertex2f_fn)sh_gl_proc("glVertex2f");
    p_gl_color4ub = (gl_color4ub_fn)sh_gl_proc("glColor4ub");
    p_gl_begin = (gl_begin_fn)sh_gl_proc("glBegin");
    p_gl_end = (gl_end_fn)sh_gl_proc("glEnd");
    p_gl_line_width = (gl_line_width_fn)sh_gl_proc("glLineWidth");
    p_gl_enable = (gl_enable_fn)sh_gl_proc("glEnable");
    p_gl_disable = (gl_disable_fn)sh_gl_proc("glDisable");
    p_gl_blend = (gl_blend_fn)sh_gl_proc("glBlendFunc");
    p_gl_tex2f = (gl_tex2f_fn)sh_gl_proc("glTexCoord2f");
}

qbool sh_render_ready(void)
{
    return p_gl_begin && p_gl_vertex2f && p_gl_color4ub && p_gl_end;
}

void sh_render_begin(i32 width, i32 height)
{
    g_width = width;
    g_height = height;
    g_text_depth = 0;

    if (!sh_render_ready())
        return;

    p_gl_disable(sh_gl_texture_2d);
    p_gl_enable(sh_gl_blend_bit);
    p_gl_blend(sh_gl_src_alpha, sh_gl_one_minus_src_alpha);
}

void sh_render_end(void)
{
    if (!sh_render_ready())
        return;
    p_gl_disable(sh_gl_blend_bit);
    p_gl_enable(sh_gl_texture_2d);
}

i32 sh_screen_width(void) { return g_width; }
i32 sh_screen_height(void) { return g_height; }

f32 sh_scale(void)
{
    f32 s = (f32)g_height / 480.0f;
    if (s < 0.5f)
        s = 0.5f;
    return s;
}

f32 sh_xres(f32 x)
{
    return x * ((f32)g_width / 640.0f);
}

f32 sh_yres(f32 y)
{
    return y * ((f32)g_height / 480.0f);
}

void sh_draw_rect(f32 x, f32 y, f32 w, f32 h, col4_t col)
{
    if (!sh_render_ready())
        return;

    p_gl_color4ub(col.r, col.g, col.b, col.a);
    p_gl_begin(sh_gl_quads);
    p_gl_vertex2f(x, y);
    p_gl_vertex2f(x + w, y);
    p_gl_vertex2f(x + w, y + h);
    p_gl_vertex2f(x, y + h);
    p_gl_end();
}

void sh_draw_rect_outline(f32 x, f32 y, f32 w, f32 h, f32 thick, col4_t col)
{
    sh_draw_rect(x, y, w, thick, col);
    sh_draw_rect(x, y + h - thick, w, thick, col);
    sh_draw_rect(x, y + thick, thick, h - thick * 2.0f, col);
    sh_draw_rect(x + w - thick, y + thick, thick, h - thick * 2.0f, col);
}

void sh_draw_rect_gradient(f32 x, f32 y, f32 w, f32 h, col4_t top, col4_t bottom)
{
    if (!sh_render_ready())
        return;

    p_gl_begin(sh_gl_quads);
    p_gl_color4ub(top.r, top.g, top.b, top.a);
    p_gl_vertex2f(x, y);
    p_gl_vertex2f(x + w, y);
    p_gl_color4ub(bottom.r, bottom.g, bottom.b, bottom.a);
    p_gl_vertex2f(x + w, y + h);
    p_gl_vertex2f(x, y + h);
    p_gl_end();
}

void sh_draw_line(f32 x0, f32 y0, f32 x1, f32 y1, f32 thick, col4_t col)
{
    if (!sh_render_ready())
        return;

    p_gl_line_width(thick);
    p_gl_color4ub(col.r, col.g, col.b, col.a);
    p_gl_begin(sh_gl_lines);
    p_gl_vertex2f(x0, y0);
    p_gl_vertex2f(x1, y1);
    p_gl_end();
}

void sh_draw_triangle(vec2_t a, vec2_t b, vec2_t c, col4_t col)
{
    if (!sh_render_ready())
        return;

    p_gl_color4ub(col.r, col.g, col.b, col.a);
    p_gl_begin(sh_gl_triangle_fan);
    p_gl_vertex2f(a.x, a.y);
    p_gl_vertex2f(b.x, b.y);
    p_gl_vertex2f(c.x, c.y);
    p_gl_end();
}

void sh_draw_circle(f32 cx, f32 cy, f32 r, i32 seg, col4_t col)
{
    if (!sh_render_ready() || seg < 3)
        return;

    p_gl_color4ub(col.r, col.g, col.b, col.a);
    p_gl_begin(sh_gl_triangle_fan);
    p_gl_vertex2f(cx, cy);
    for (i32 i = 0; i <= seg; ++i)
    {
        f32 a = (f32)i * (sh_pi_f * 2.0f) / (f32)seg;
        p_gl_vertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    p_gl_end();
}

void sh_draw_circle_outline(f32 cx, f32 cy, f32 r, i32 seg, f32 thick, col4_t col)
{
    if (!sh_render_ready() || seg < 3)
        return;

    p_gl_line_width(thick);
    p_gl_color4ub(col.r, col.g, col.b, col.a);
    p_gl_begin(sh_gl_line_strip);
    for (i32 i = 0; i <= seg; ++i)
    {
        f32 a = (f32)i * (sh_pi_f * 2.0f) / (f32)seg;
        p_gl_vertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    p_gl_end();
}

void sh_text_push(void)
{
    ++g_text_depth;
}

void sh_text_pop(void)
{
    if (g_text_depth > 0)
        --g_text_depth;
}

static void sh_text_v(f32 x, f32 y, col4_t col, const char *fmt, va_list ap)
{
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    sh_font_draw(x, y, sh_scale(), col, buf);
}

void sh_text(f32 x, f32 y, col4_t col, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    sh_text_v(x, y, col, fmt, ap);
    va_end(ap);
}

void sh_text_center(f32 cx, f32 y, col4_t col, const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    f32 w = sh_font_width(buf, sh_scale());
    sh_font_draw(cx - w * 0.5f, y, sh_scale(), col, buf);
}

void sh_text_shadow(f32 x, f32 y, col4_t col, const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    col4_t sh = { 0, 0, 0, col.a };
    f32 s = sh_scale();
    sh_font_draw(x + s, y + s, s, sh, buf);
    sh_font_draw(x, y, s, col, buf);
}

f32 sh_text_width(const char *str)
{
    return sh_font_width(str, sh_scale());
}

f32 sh_text_height(void)
{
    return (f32)10 * sh_scale();
}
