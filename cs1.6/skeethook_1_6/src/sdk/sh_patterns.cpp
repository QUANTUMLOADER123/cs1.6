#include "../../include/sh_sdk.h"
#include "../../include/sh_config.h"
#include "../../include/sh_scanner.h"

#include <windows.h>

sh_engfuncs_t *g_engfuncs = 0;
sh_playermove_t *g_pmove = 0;
sh_globalvars_t *g_globals = 0;
void **g_entity_list = 0;
sh_cl_entity_t *(*g_get_entity)(i32 index) = 0;
i32 g_local_player = 0;
void *g_module_client = 0;
void *g_module_hw = 0;
void **g_pmove_slot = 0;
void **g_engfuncs_slot = 0;

static const u8 pat_init[] = {
    0x8B, 0x44, 0x24, 0x08, 0x81, 0xEC, 0x08, 0x04, 0x00, 0x00, 0xB9,
    0x87, 0x00, 0x00, 0x00, 0x83, 0xF8, 0x07, 0x53, 0x56, 0x8B, 0xB4};
static const char pat_init_mask[] = "xxxxxxxxxxxxxxxxxxxxxx";

static const u8 pat_player_move[] = {
    0x8B, 0x54, 0x24, 0x08, 0x8B, 0x44, 0x24, 0x04, 0x33, 0xC9, 0xA3,
    0x00, 0x00, 0x00, 0x00, 0x85, 0xD2, 0x0F, 0x95, 0xC1, 0x51, 0xE8};
static const char pat_player_move_mask[] = "xxxxxxxxxxx????xxxxxxx";

static const u8 pat_player_move_init[] = {
    0x8B, 0x44, 0x24, 0x04, 0xA3, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00,
    0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x05, 0x80};
static const char pat_player_move_init_mask[] = "xxxxx????x????x????xxx";

static const u8 pat_create_move[] = {
    0xA0, 0x00, 0x00, 0x00, 0x00, 0x83, 0xEC, 0x0C, 0x53, 0xBB, 0x01,
    0x00, 0x00, 0x00, 0x84, 0xC3, 0x56, 0x75, 0x17, 0x8A, 0xD0, 0x68};
static const char pat_create_move_mask[] = "x????xxxxxxxxxxxxxxxxx";

static const u8 pat_hud_redraw[] = {
    0x8B, 0x44, 0x24, 0x08, 0x8B, 0x4C, 0x24, 0x04, 0x50, 0x51, 0xB9,
    0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x01,
    0x00, 0x00, 0x00, 0xC3};
static const char pat_hud_redraw_mask[] = "xxxxxxxxxxx????x????xxxxxx";

typedef struct sh_named_pat_s
{
    const char *name;
    const sh_pattern_t pat;
} sh_named_pat_t;

static const sh_named_pat_t g_sig_table[] = {
    {"initialize", {"initialize", pat_init, pat_init_mask, (i32)sizeof(pat_init)}},
    {"player_move", {"player_move", pat_player_move, pat_player_move_mask,
                     (i32)sizeof(pat_player_move)}},
    {"player_move_init", {"player_move_init", pat_player_move_init,
                          pat_player_move_init_mask, (i32)sizeof(pat_player_move_init)}},
    {"create_move", {"create_move", pat_create_move, pat_create_move_mask,
                     (i32)sizeof(pat_create_move)}},
    {"hud_redraw", {"hud_redraw", pat_hud_redraw, pat_hud_redraw_mask,
                    (i32)sizeof(pat_hud_redraw)}},
};

static void *sh_va_to_ptr(u32 va)
{
    void *base = sh_module_base("client.dll");
    if (!base)
        return 0;

    u32 off = va - (u32)base;
    if (off >= sh_module_size("client.dll"))
        return 0;
    return (u8 *)base + off;
}

void *sh_find_named_sig(const char *name)
{
    for (u32 i = 0; i < sh_countof(g_sig_table); ++i)
    {
        if (sh_str_eq(g_sig_table[i].name, name))
            return sh_find_pattern_in("client.dll", &g_sig_table[i].pat);
    }
    return 0;
}

static void sh_resolve_initialize(void)
{
    u8 *hit = (u8 *)sh_find_named_sig("initialize");
    if (!hit)
        return;

    u8 *p = hit;
    i32 guard = 0;
    while (guard++ < 128)
    {
        if (*p == 0xBF)
            break;
        ++p;
    }
    if (*p != 0xBF)
        return;

    void **slot = (void **)sh_va_to_ptr(*(u32 *)(p + 1));
    if (!slot)
        return;

    g_engfuncs_slot = slot;
    g_engfuncs = (sh_engfuncs_t *)*slot;
}

static void sh_resolve_player_move(void)
{
    u8 *hit = (u8 *)sh_find_named_sig("player_move");
    if (!hit)
        return;

    void **slot = (void **)sh_va_to_ptr(*(u32 *)(hit + 12));
    if (!slot)
        return;

    g_pmove_slot = slot;
    g_pmove = (sh_playermove_t *)*slot;
}

void sh_sdk_resolve(void)
{
    g_module_client = sh_module_base("client.dll");
    g_module_hw = sh_module_base("hw.dll");

    sh_resolve_initialize();
    sh_resolve_player_move();

    if (g_engfuncs)
    {
        g_get_entity = (sh_cl_entity_t * (*)(i32)) g_engfuncs->pfnGetEntityByIndex;

        if (g_engfuncs->pfnGetLocalPlayer)
        {
            i32 idx = g_engfuncs->pfnGetLocalPlayer();
            if (idx >= 0 && idx < SH_MAX_ENTITIES)
                g_local_player = idx;
        }
    }
}

qbool sh_sdk_ready(void)
{
    return g_engfuncs != 0 && g_pmove != 0;
}

sh_cl_entity_t *sh_get_entity(i32 index)
{
    if (index < 0 || index >= SH_MAX_ENTITIES)
        return 0;
    if (!g_get_entity)
        return 0;
    return g_get_entity(index);
}

void sh_sdk_set_entity_fn(sh_cl_entity_t *(*fn)(i32))
{
    g_get_entity = fn;
}
