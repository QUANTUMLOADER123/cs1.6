#include "../../include/sh_fakelag.h"
#include "../../include/sh_config.h"
#include "../../include/sh_scanner.h"

static const u8 g_pat_bytes[] = {
    0x83, 0xF8, 0xFF, 0x75, 0x0A, 0xA1, 0x30, 0x9B, 0xD5,
    0x02, 0xA3, 0x00, 0x00, 0x00, 0x00, 0x39, 0x3D, 0x84, 0xDF, 0xD5
};

static const sh_pattern_t g_pat_next_send = {
    "next_send_time", g_pat_bytes, "xxxxxxxxxxx????xxxx", sizeof(g_pat_bytes)
};

f32 g_next_send_time;
static i32 g_choke_counter;
static void *g_next_send_va;
static void *g_last_outgoing_va;

void sh_fakelag_init(void)
{
    if (g_next_send_va)
        return;

    u8 *hit = (u8 *)sh_find_pattern_in("hw.dll", &g_pat_next_send);

    if (!hit)
        return;

    u32 lastout = *(u32 *)(hit + 11);

    g_next_send_va = (void *)(lastout + 4);
    g_last_outgoing_va = (void *)lastout;
}

i32 sh_fakelag_choking(void)
{
    return g_choke_counter;
}

void sh_fakelag_reset(void)
{
    g_choke_counter = 0;
}

qbool sh_fakelag_should_choke(void)
{
    if (!g_cfg.antiaim.fake_lag)
        return 0;

    i32 limit = g_cfg.antiaim.fake_lag_ticks;

    if (limit <= 0)
        return 0;

    if (limit > sh_fakelag_max_choke)
        limit = sh_fakelag_max_choke;

    if (g_choke_counter >= limit)
    {
        g_choke_counter = 0;
        return 0;
    }

    g_choke_counter++;

    return 1;
}

void sh_fakelag_run(sh_usercmd_t *cmd)
{
    if (!cmd || !g_cfg.antiaim.fake_lag)
        return;

    if (g_pmove && g_pmove->movetype == sh_movetype_fly)
        return;

    if (!g_next_send_va)
        sh_fakelag_init();

    if (!g_next_send_va)
        return;

    g_next_send_time = *(volatile f32 *)g_next_send_va;

    f32 now = g_pmove ? g_pmove->time : 0.0f;

    if (g_next_send_time > now + 1.0f)
        return;

    if (!sh_fakelag_should_choke())
    {
        g_choke_counter = 0;
        return;
    }

    f32 cmdrate = 100.0f;

    if (g_cfg.antiaim.fake_lag_ticks > 0)
        cmdrate = 100.0f / (f32)g_cfg.antiaim.fake_lag_ticks;

    if (cmdrate < 5.0f)
        cmdrate = 5.0f;

    f32 interval = 1.0f / cmdrate;

    if (interval > 0.0f && interval < 1.0f)
        *(volatile f32 *)g_next_send_va = now + interval;
}
