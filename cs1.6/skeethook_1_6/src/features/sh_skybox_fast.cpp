#include "../../include/sh_scanner.h"
#include "../../include/sh_types.h"
#include "../../include/sh_skybox.h"

static const u8 g_pat_load_skys[] = {
    0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x6C, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x56,
    0x57, 0x33, 0xFF, 0x3B, 0xC7, 0x89, 0x7D, 0xF4, 0x75, 0x25,
};

static const sh_pattern_t g_sig_load_skys = {
    "load_skys", g_pat_load_skys,
    "xxxxxxx????xxxxxxxxxxx",
    sizeof(g_pat_load_skys)
};

static const u8 g_pat_load_skys_name[] = {
    0x50, 0x68, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x6A,
    0x40, 0x51, 0x33, 0xFF,
};

static const sh_pattern_t g_sig_load_skys_name = {
    "load_skys_name", g_pat_load_skys_name,
    "xx????x????xxxxx",
    sizeof(g_pat_load_skys_name)
};

typedef void(__cdecl *load_skys_fn)(void);

static load_skys_fn g_load_skys;
static char *g_sky_name_buf;
static i32 *g_sky_reload;

qbool sh_skybox_fast_init(void)
{
    if (g_load_skys)
        return true;

    u8 *ld = (u8 *)sh_find_pattern_in("hw.dll", &g_sig_load_skys);
    u8 *nm = (u8 *)sh_find_pattern_in("hw.dll", &g_sig_load_skys_name);

    if (!ld || !nm)
        return false;

    g_sky_reload = *(i32 **)(ld + 7);
    g_sky_name_buf = *(char **)(nm + 2);
    g_load_skys = (load_skys_fn)ld;

    if (!g_sky_reload || !g_sky_name_buf)
    {
        g_load_skys = 0;
        return false;
    }

    return true;
}

qbool sh_skybox_fast_ready(void)
{
    return g_load_skys != 0;
}

qbool sh_skybox_fast_apply(const char *name)
{
    if (!g_load_skys || !g_sky_name_buf || !g_sky_reload || !name)
        return false;

    i32 i = 0;

    while (name[i] && i < 31)
    {
        g_sky_name_buf[i] = name[i];
        ++i;
    }

    g_sky_name_buf[i] = 0;

    *g_sky_reload = 1;

    g_load_skys();

    return true;
}
