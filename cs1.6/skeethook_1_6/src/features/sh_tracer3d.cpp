#include "../../include/sh_config.h"
#include "../../include/sh_math.h"
#include "../../include/sh_scanner.h"
#include "../../include/sh_types.h"

typedef void(__cdecl *sh_user_tracer_fn)(const vec3_t *start, const vec3_t *dir,
                                         f32 life, i32 color, f32 speed,
                                         i8 scale, i32 alpha);

static const u8 g_pat_tracer[] = {
    0x55, 0x8B, 0xEC, 0x8B, 0x4D, 0x00, 0x85, 0xC9, 0x7D, 0x00, 0x68, 0x00,
    0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x83, 0xC4, 0x04, 0x5D,
    0xC3, 0x83, 0xF9, 0x0C, 0x76, 0x00, 0x6A, 0x0C, 0x68, 0x00, 0x00, 0x00,
    0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x83, 0xC4, 0x08, 0x5D, 0xC3, 0xA1,
    0x00, 0x00, 0x00, 0x00, 0x85, 0xC0, 0x74, 0x00, 0x8B, 0x50, 0x10, 0x53,
    0xD9, 0x45, 0x00, 0xDC, 0x05, 0x00, 0x00, 0x00, 0x00, 0x89, 0x15, 0x00,
    0x00, 0x00, 0x00, 0x8B, 0x15, 0x00, 0x00, 0x00, 0x00, 0x89, 0x50, 0x10,
    0x8B, 0x55, 0x00, 0x66, 0x89, 0x48, 0x0C, 0x8B, 0x4D, 0x00, 0xD9, 0x58,
    0x24,
};

static const sh_pattern_t g_sig_tracer = {
    "user_tracer", g_pat_tracer,
    "xxxxx?xxx?x????x????xxxxxxxxx?xxx????x????xxxxxx????xxx?"
    "xxxxxx?xx????xx????xx????xxxxx?xxxxxx?xxx",
    sizeof(g_pat_tracer)
};

static sh_user_tracer_fn g_user_tracer;

void sh_tracer3d_init(void)
{
    if (g_user_tracer)
        return;

    g_user_tracer = (sh_user_tracer_fn)sh_find_pattern_in("hw.dll", &g_sig_tracer);
}

void sh_tracer3d_add(const vec3_t *start, const vec3_t *end)
{
    if (!g_user_tracer || !g_cfg.visuals.tracer3d)
        return;

    if (!start || !end)
        return;

    vec3_t dir;
    dir.x = end->x - start->x;
    dir.y = end->y - start->y;
    dir.z = end->z - start->z;

    f32 len = sh_sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

    if (len < 1.0f)
        return;

    dir.x /= len;
    dir.y /= len;
    dir.z /= len;

    i32 col = g_cfg.visuals.tracer3d_color;

    if (col < 0)
        col = 0;
    if (col > 12)
        col = 12;

    f32 life = g_cfg.visuals.tracer3d_life;

    if (life < 0.05f)
        life = 0.05f;
    if (life > 5.0f)
        life = 5.0f;

    g_user_tracer(start, &dir, life, col, len * 10.0f, 0, 0);
}

qbool sh_tracer3d_ready(void)
{
    return g_user_tracer != 0;
}
