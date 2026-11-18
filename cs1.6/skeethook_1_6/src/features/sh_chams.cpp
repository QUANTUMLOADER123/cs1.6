#include <windows.h>
#include "../ext/minhook/include/MinHook.h"

#include "../../include/sh_config.h"
#include "../../include/sh_scanner.h"
#include "../../include/sh_types.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_math.h"

typedef void(__stdcall *gl_void_i_fn)(u32);
typedef u8(__stdcall *gl_is_enabled_fn)(u32);
typedef void(__stdcall *gl_void_f4_fn)(f32, f32, f32, f32);
typedef void(__stdcall *gl_void_ii_fn)(u32, u32);
typedef void(__stdcall *gl_get_iv_fn)(u32, i32 *);

static const u8 g_pat_model[] = {
    0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x48, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x8B,
    0x0D, 0x00, 0x00, 0x00, 0x00, 0x53, 0x56, 0x8B, 0x70, 0x54, 0x8B, 0x40,
    0x60, 0x03, 0xC1, 0x57, 0x89, 0x45, 0xEC,
};

static const sh_pattern_t g_sig_model = {
    "chams_model", g_pat_model,
    "xxxxxxx????xx????xxxxxxxxxxxxxx",
    sizeof(g_pat_model)
};

static const u8 g_pat_alpha[] = {
    0x55, 0x8B, 0xEC, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x4D, 0x08, 0x50,
    0x89, 0x88, 0xFC, 0x02, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x89,
    0x45, 0x08, 0x83, 0xC4, 0x04, 0xDB, 0x45, 0x08, 0xD9, 0x1D, 0x00, 0x00,
    0x00, 0x00, 0xD9, 0x05, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x0D, 0x00, 0x00,
    0x00, 0x00, 0xD9, 0x1D, 0x00, 0x00, 0x00, 0x00,
};

static const sh_pattern_t g_sig_alpha = {
    "chams_alpha", g_pat_alpha,
    "xxxx????xxxxxxxxxxx????xxxxxxxxxxx????xx????xx????xx????",
    sizeof(g_pat_alpha)
};

static const u8 g_pat_light[] = {
    0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x24, 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00,
    0x56, 0x57, 0x33, 0xFF, 0x3B, 0xCF,
};

static const sh_pattern_t g_sig_light = {
    "chams_light", g_pat_light,
    "xxxxxxxx????xxxxxx",
    sizeof(g_pat_light)
};

typedef i32(__cdecl *draw_model_fn)(void);
typedef i32(__cdecl *light_vertex_fn)(i32 light, f32 *normal, f32 *base, f32 *out);

static draw_model_fn g_model_orig;
static light_vertex_fn g_light_orig;
static gl_void_i_fn g_gl_disable;
static gl_void_i_fn g_gl_enable;
static gl_is_enabled_fn g_gl_is_enabled;
static gl_void_f4_fn g_gl_color4f;
static gl_void_ii_fn g_gl_blend_func;
static gl_get_iv_fn g_gl_get_iv;
static gl_void_i_fn g_gl_depth_func;
static gl_void_i_fn g_gl_depth_mask;
static f32 *g_material_alpha;
static qbool g_ready;
static qbool g_override;
static f32 g_ov_col[4];

#define sh_gl_texture_2d    0x0DE1
#define sh_gl_depth_test    0x0B71
#define sh_gl_blend         0x0BE2
#define sh_gl_src_alpha     0x0302
#define sh_gl_one_minus     0x0303
#define sh_gl_lequal        0x0203
#define sh_gl_blend_src_rgb 0x80E1
#define sh_gl_blend_dst_rgb 0x80E0
#define sh_gl_greater      0x0204

static i32 sh_chams_block(draw_model_fn orig, const f32 *col, u32 depth_func,
                          qbool write_depth, qbool untextured)
{
    qbool textured = g_gl_is_enabled(sh_gl_texture_2d) != 0;
    qbool dt = g_gl_is_enabled(sh_gl_depth_test) != 0;
    qbool bl = g_gl_is_enabled(sh_gl_blend) != 0;

    g_gl_depth_mask(write_depth ? 1 : 0);
    g_gl_depth_func(depth_func);

    if (!dt)
        g_gl_enable(sh_gl_depth_test);
    if (!bl)
        g_gl_enable(sh_gl_blend);

    if (untextured && textured)
        g_gl_disable(sh_gl_texture_2d);
    else if (!untextured && !textured)
        g_gl_enable(sh_gl_texture_2d);

    i32 prev_src = sh_gl_src_alpha;
    i32 prev_dst = sh_gl_one_minus;

    if (g_gl_get_iv)
    {
        g_gl_get_iv(sh_gl_blend_src_rgb, &prev_src);
        g_gl_get_iv(sh_gl_blend_dst_rgb, &prev_dst);
    }

    g_gl_blend_func(sh_gl_src_alpha, sh_gl_one_minus);

    f32 prev = 1.0f;

    if (g_material_alpha)
    {
        prev = *g_material_alpha;
        *g_material_alpha = col[3];
    }

    g_gl_color4f(col[0], col[1], col[2], col[3]);
    g_override = (qbool)1;
    g_ov_col[0] = col[0];
    g_ov_col[1] = col[1];
    g_ov_col[2] = col[2];
    g_ov_col[3] = col[3];

    i32 result = orig();

    g_override = (qbool)0;

    if (g_material_alpha)
        *g_material_alpha = prev;

    g_gl_blend_func((u32)prev_src, (u32)prev_dst);
    g_gl_depth_func(sh_gl_lequal);
    g_gl_depth_mask(1);

    if (untextured && textured)
        g_gl_enable(sh_gl_texture_2d);
    else if (!untextured && !textured)
        g_gl_disable(sh_gl_texture_2d);

    if (!bl)
        g_gl_disable(sh_gl_blend);
    if (!dt)
        g_gl_disable(sh_gl_depth_test);

    return result;
}

static i32 __cdecl sh_draw_model_hook(void)
{
    if (!g_model_orig)
        return 0;

    if (!g_ready || !g_cfg.visuals.chams || !g_cfg.visuals.chams_xqz)
        return g_model_orig();

    col4_t xq = g_cfg.visuals.col_chams_xqz;
    col4_t vi = g_cfg.visuals.col_chams_vis;

    f32 occ[4] = { (f32)xq.r / 255.0f, (f32)xq.g / 255.0f,
                   (f32)xq.b / 255.0f, (f32)xq.a / 255.0f };
    f32 vis[4] = { (f32)vi.r / 255.0f, (f32)vi.g / 255.0f,
                   (f32)vi.b / 255.0f, (f32)vi.a / 255.0f };

    sh_chams_block(g_model_orig, occ, sh_gl_greater, (qbool)0, (qbool)1);

    return sh_chams_block(g_model_orig, vis, sh_gl_lequal, (qbool)1, (qbool)1);
}

static i32 __cdecl sh_light_vertex_hook(i32 light, f32 *normal, f32 *base, f32 *out)
{
    if (!g_light_orig)
        return 0;

    i32 r = g_light_orig(light, normal, base, out);

    if (!g_override || !out)
        return r;

    if (!g_cfg.visuals.chams_rim)
    {
        out[0] = g_ov_col[0];
        out[1] = g_ov_col[1];
        out[2] = g_ov_col[2];
        return r;
    }

    f32 lv = (out[0] + out[1] + out[2]) * (1.0f / 3.0f);

    if (lv < 0.0f)
        lv = 0.0f;
    if (lv > 1.0f)
        lv = 1.0f;

    f32 rim = 0.2f;

    if (normal)
    {
        f32 nl = sh_sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);

        if (nl > 0.0001f)
        {
            f32 nx = normal[0] / nl;
            f32 ny = normal[1] / nl;
            f32 nz = normal[2] / nl;

            f32 facing = 0.0f;

            if (g_globals)
            {
                facing = nx * g_globals->v_forward.x + ny * g_globals->v_forward.y +
                         nz * g_globals->v_forward.z;
            }

            if (facing < 0.0f)
                facing = -facing;
            if (facing > 1.0f)
                facing = 1.0f;

            f32 edge = 1.0f - facing;
            rim = edge * edge * edge;
        }
    }

    f32 shade = 0.06f + lv * 0.28f + rim * 0.82f;

    if (shade > 1.0f)
        shade = 1.0f;

    out[0] = g_ov_col[0] * shade;
    out[1] = g_ov_col[1] * shade;
    out[2] = g_ov_col[2] * shade;

    return r;
}

qbool sh_chams_gl_init(void)
{
    if (g_ready)
        return true;

    HMODULE gl = GetModuleHandleA("opengl32.dll");

    if (!gl)
        return false;

    g_gl_disable = (gl_void_i_fn)(void *)GetProcAddress(gl, "glDisable");
    g_gl_enable = (gl_void_i_fn)(void *)GetProcAddress(gl, "glEnable");
    g_gl_is_enabled = (gl_is_enabled_fn)(void *)GetProcAddress(gl, "glIsEnabled");
    g_gl_color4f = (gl_void_f4_fn)(void *)GetProcAddress(gl, "glColor4f");
    g_gl_blend_func = (gl_void_ii_fn)(void *)GetProcAddress(gl, "glBlendFunc");
    g_gl_get_iv = (gl_get_iv_fn)(void *)GetProcAddress(gl, "glGetIntegerv");
    g_gl_depth_func = (gl_void_i_fn)(void *)GetProcAddress(gl, "glDepthFunc");
    g_gl_depth_mask = (gl_void_i_fn)(void *)GetProcAddress(gl, "glDepthMask");

    if (!g_gl_disable || !g_gl_enable || !g_gl_is_enabled || !g_gl_color4f ||
        !g_gl_blend_func || !g_gl_depth_func || !g_gl_depth_mask)
        return false;

    u8 *model = (u8 *)sh_find_pattern_in("hw.dll", &g_sig_model);
    u8 *alpha = (u8 *)sh_find_pattern_in("hw.dll", &g_sig_alpha);

    if (!model || !alpha)
        return false;

    g_material_alpha = *(f32 **)(alpha + 34);

    if (MH_CreateHook((void *)model, (void *)sh_draw_model_hook,
                      (void **)&g_model_orig) != MH_OK)
        return false;

    MH_EnableHook((void *)model);

    u8 *light = (u8 *)sh_find_pattern_in("hw.dll", &g_sig_light);

    if (light)
    {
        if (MH_CreateHook((void *)light, (void *)sh_light_vertex_hook,
                          (void **)&g_light_orig) == MH_OK)
            MH_EnableHook((void *)light);
    }

    g_ready = (qbool)1;

    return true;
}

qbool sh_chams_gl_ready(void)
{
    return g_ready;
}

qbool sh_chams_override_active(void)
{
    return g_override;
}
