#include "../../include/sh_hitmarker.h"
#include "../../include/sh_config.h"
#include "../../include/sh_render.h"

#define sh_hit_life 0.55f

typedef struct sh_hit_s
{
    f32 born;
    qbool headshot;
    i32 damage;
    qbool active;
} sh_hit_t;

static sh_hit_t g_hits[16];
static i32 g_cursor;
static i32 g_total;
static f32 g_now;

void sh_hitmarker_on_hit(qbool headshot, i32 damage)
{
    sh_hit_t *h = &g_hits[g_cursor];

    h->born = g_now;
    h->headshot = headshot;
    h->damage = damage;
    h->active = 1;

    g_cursor = (g_cursor + 1) % 16;
    g_total++;
}

void sh_hitmarker_tick(f32 time)
{
    g_now = time;

    for (i32 i = 0; i < 16; ++i)
    {
        if (g_hits[i].active && (g_now - g_hits[i].born) > sh_hit_life)
            g_hits[i].active = 0;
    }
}

i32 sh_hitmarker_count(void)
{
    return g_total;
}

void sh_hitmarker_draw(void)
{
    f32 cx = (f32)sh_screen_width() * 0.5f;
    f32 cy = (f32)sh_screen_height() * 0.5f;

    for (i32 i = 0; i < 16; ++i)
    {
        sh_hit_t *h = &g_hits[i];

        if (!h->active)
            continue;

        f32 age = (g_now - h->born) / sh_hit_life;
        f32 fade = (age < 0.0f) ? 1.0f : (1.0f - age);

        if (fade <= 0.0f)
            continue;

        col4_t c = h->headshot
                 ? (col4_t){ 255, 80, 80, (u8)(255.0f * fade) }
                 : (col4_t){ 255, 255, 255, (u8)(255.0f * fade) };

        f32 len = 9.0f;
        f32 gap = 4.0f;

        sh_draw_line(cx - gap - len, cy - gap - len, cx - gap, cy - gap, 1.5f, c);
        sh_draw_line(cx + gap, cy - gap, cx + gap + len, cy - gap - len, 1.5f, c);
        sh_draw_line(cx - gap - len, cy + gap + len, cx - gap, cy + gap, 1.5f, c);
        sh_draw_line(cx + gap, cy + gap, cx + gap + len, cy + gap + len, 1.5f, c);
    }
}
