#include "../../include/sh_config.h"
#include "../../include/sh_scanner.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_features.h"
#include "../../include/sh_render.h"
#include "../../include/sh_math.h"
#include "../ext/minhook/include/MinHook.h"

#define sh_max_sounds 32
#define sh_sound_life 1.0f

typedef struct sh_sound_s
{
    vec3_t origin;
    f32 born;
    i32 kind;
    i32 active;
} sh_sound_t;

static sh_sound_t g_sounds[sh_max_sounds];
static i32 g_sound_cursor;
static i32 g_sound_ready;

static const char *g_sound_marker = "CL_Parse_Sound: ent = %i, cl.max_edicts %i";

typedef void(__cdecl *sh_pres_sound_fn)(i32 ent, i32 channel,
                                        const char *name, f32 *origin,
                                        f32 volume, f32 attenuation,
                                        i32 timeofs, i32 pitch);

static sh_pres_sound_fn g_pres_orig;

static void __cdecl sh_soundesp_hook(i32 ent, i32 channel,
                                     const char *name, f32 *origin,
                                     f32 volume, f32 attenuation,
                                     i32 timeofs, i32 pitch);

static void *sh_find_string_in(const char *module, const char *text)
{
    void *base = sh_module_base(module);

    if (!base)
        return 0;

    u32 size = sh_module_size(module);

    if (!size)
        return 0;

    u8 *start = (u8 *)base;
    u8 *end = start + size;

    i32 len = 0;

    while (text[len])
        ++len;

    for (u8 *p = start; p + len + 1 < end; ++p)
    {
        i32 k = 0;

        while (k < len && p[k] == (u8)text[k])
            ++k;

        if (k == len && p[k] == 0)
            return p;
    }

    return 0;
}

static void *sh_find_push_to(void *string_va, const char *module)
{
    void *base = sh_module_base(module);

    if (!base || !string_va)
        return 0;

    u32 size = sh_module_size(module);

    if (!size)
        return 0;

    u8 *start = (u8 *)base;
    u8 *end = start + size;

    u32 target = (u32)(unsigned int)(size_t)string_va;

    for (u8 *p = start; p + 5 < end; ++p)
    {
        if (p[0] != 0x68)
            continue;

        u32 imm = 0;

        imm |= (u32)p[1];
        imm |= (u32)p[2] << 8;
        imm |= (u32)p[3] << 16;
        imm |= (u32)p[4] << 24;

        if (imm == target)
            return p;
    }

    return 0;
}

static void *sh_absolute_jump(u8 *at)
{
    if (!at)
        return 0;

    i32 rel = 0;

    rel |= (i32)at[1];
    rel |= (i32)at[2] << 8;
    rel |= (i32)at[3] << 16;
    rel |= (i32)at[4] << 24;

    return (void *)(at + rel + 5);
}

qbool sh_soundesp_init(void)
{
    if (g_sound_ready)
        return g_sound_ready == 2;

    g_sound_ready = 1;

    void *str = sh_find_string_in("hw.dll", g_sound_marker);

    if (!str)
        return 0;

    u8 *push = (u8 *)sh_find_push_to(str, "hw.dll");

    if (!push)
        return 0;

    if (push < (u8 *)sh_module_base("hw.dll") + 0x20)
        return 0;

    u8 *call = push - 0x10;

    if (call[0] != 0xE8 && call[0] != 0xE9)
        call = push - 0x11;

    if (call[0] != 0xE8 && call[0] != 0xE9)
        return 0;

    void *target = sh_absolute_jump(call);

    if (!target)
        return 0;

    sh_pres_sound_fn fn = (sh_pres_sound_fn)target;

    if (fn == g_pres_orig)
        return 0;

    if (MH_CreateHook((void *)fn, (void *)sh_soundesp_hook,
                      (void **)&g_pres_orig) != MH_OK)
        return 0;

    if (MH_EnableHook((void *)fn) != MH_OK)
        return 0;

    g_sound_ready = 2;
    return 1;
}

static i32 sh_sound_kind(const char *name)
{
    if (!name)
        return 0;

    const char *p = name;

    while (*p)
    {
        if (p[0] == 'w' && p[1] == 'e' && p[2] == 'a')
            return 1;

        if (p[0] == 'p' && p[1] == 'l' && p[2] == '_')
            return 2;

        ++p;
    }

    return 0;
}

static qbool sh_sound_skip(const char *name)
{
    if (!name)
        return 1;

    const char *skip[] = { "pl_shell", "ric", "die", "glass", "debris" };

    for (i32 i = 0; i < 5; ++i)
    {
        const char *a = name;
        const char *b = skip[i];

        while (*a && *b)
        {
            if (*a != *b)
                break;
            ++a;
            ++b;
        }

        if (!*b && *a)
            return 1;
    }

    return 0;
}

void sh_soundesp_add(const vec3_t *origin, const char *name)
{
    if (!origin || !g_pmove || !g_cfg.visuals.sound_esp)
        return;

    if (sh_sound_skip(name))
        return;

    i32 kind = sh_sound_kind(name);

    if (!kind)
        return;

    sh_sound_t *s = &g_sounds[g_sound_cursor];

    s->origin = *origin;
    s->born = g_pmove->time;
    s->kind = kind;
    s->active = 1;

    g_sound_cursor = (g_sound_cursor + 1) % sh_max_sounds;
}

void sh_soundesp_tick(void)
{
    f32 now = g_pmove ? g_pmove->time : 0.0f;

    for (i32 i = 0; i < sh_max_sounds; ++i)
    {
        if (!g_sounds[i].active)
            continue;

        if (now - g_sounds[i].born > sh_sound_life)
            g_sounds[i].active = 0;
    }
}

void sh_soundesp_draw(void)
{
    if (!g_cfg.visuals.sound_esp)
        return;

    if (!g_pmove)
    {
        for (i32 i = 0; i < sh_max_sounds; ++i)
            g_sounds[i].active = 0;

        return;
    }

    sh_soundesp_tick();

    f32 now = g_pmove->time;

    for (i32 i = 0; i < sh_max_sounds; ++i)
    {
        sh_sound_t *s = &g_sounds[i];

        if (!s->active)
            continue;

        vec2_t sp;

        if (!sh_world_to_screen(&s->origin, &sp))
            continue;

        f32 age = (now - s->born) / sh_sound_life;

        if (age < 0.0f)
            age = 0.0f;

        f32 size = 10.0f * (1.0f - age);

        if (size < 1.0f)
            size = 1.0f;

        col4_t c;

        if (s->kind == 1)
            c = (col4_t)SH_COL_RGBA(255, 60, 60, 220);
        else
            c = (col4_t)SH_COL_RGBA(100, 200, 160, 220);

        sh_draw_line(sp.x - size, sp.y - size, sp.x + size, sp.y - size, 1.0f, c);
        sh_draw_line(sp.x + size, sp.y - size, sp.x + size, sp.y + size, 1.0f, c);
        sh_draw_line(sp.x + size, sp.y + size, sp.x - size, sp.y + size, 1.0f, c);
        sh_draw_line(sp.x - size, sp.y + size, sp.x - size, sp.y - size, 1.0f, c);
    }
}

static void __cdecl sh_soundesp_hook(i32 ent, i32 channel,
                                     const char *name, f32 *origin,
                                     f32 volume, f32 attenuation,
                                     i32 timeofs, i32 pitch)
{
    (void)channel;
    (void)volume;
    (void)attenuation;
    (void)timeofs;
    (void)pitch;

    if (g_cfg.visuals.sound_esp && origin && name)
    {
        vec3_t pos;
        pos.x = origin[0];
        pos.y = origin[1];
        pos.z = origin[2];

        sh_soundesp_add(&pos, name);
    }

    if (g_pres_orig)
        g_pres_orig(ent, channel, name, origin, volume, attenuation,
                    timeofs, pitch);
}
