#include "../../include/sh_hooks.h"
#include "../../include/sh_config.h"
#include "../../include/sh_features.h"
#include "../../include/sh_render.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_scanner.h"
#include "../../include/sh_font.h"
#include "../../include/sh_movement.h"
#include "../../include/sh_ballistics.h"
#include "../../include/sh_visuals.h"
#include "../../include/sh_fakelag.h"
#include "../../include/sh_rage.h"
#include "../../include/sh_skybox.h"
#include "../../include/sh_hitmarker.h"
#include "../../include/sh_worldfx.h"
#include "../../include/sh_movement.h"

void sh_indicators_draw(void);
#include "../../include/sh_visuals.h"
#include "../../include/sh_features.h"


#include <windows.h>
#include "../ext/minhook/include/MinHook.h"

typedef i32(__cdecl *create_move_fn)(f32 frametime, sh_usercmd_t *cmd);
typedef i32(__cdecl *hud_redraw_fn)(f32 time, i32 intermission);
typedef i32(__cdecl *player_move_fn)(sh_playermove_t *pmove, i32 server);
typedef i32(__cdecl *hook_user_msg_fn)(const char *name, i32 size);
typedef i32(__cdecl *add_entity_fn)(i32 type, sh_cl_entity_t *ent);

static create_move_fn g_create_move_orig;
static hud_redraw_fn g_hud_redraw_orig;
static player_move_fn g_player_move_orig;
static hook_user_msg_fn g_user_msg_orig;
static add_entity_fn g_add_entity_orig;

static qbool g_init_done;
static qbool g_render_ready;
static f32 g_frame_time;

static void *sh_find_export_body(const char *module, const char *name)
{
    HMODULE mod = (HMODULE)sh_module_base(module);
    if (!mod)
        return 0;

    FARPROC fn = GetProcAddress(mod, name);
    if (!fn)
        return 0;

    u8 *p = (u8 *)fn;

    for (i32 guard = 0; guard < 16; ++guard)
    {
        if (p[0] != 0xE9 && p[0] != 0xEB)
            break;
        if (p[0] == 0xE9)
        {
            i32 rel = *(i32 *)(p + 1);
            p = p + 5 + rel;
        }
        else
        {
            i8 rel = *(i8 *)(p + 1);
            p = p + 2 + rel;
        }
    }
    return p;
}

static void sh_engine_trace_setup(void)
{
    if (!g_pmove || g_pmove->numphysent <= 0)
        return;
}

static qbool sh_is_ready(void)
{
    if (!sh_sdk_ready())
    {
        sh_sdk_resolve();
        if (!sh_sdk_ready())
            return false;
    }

    if (!g_cfg.ready)
        sh_config_load();

    return true;
}

static void sh_first_init(void)
{
    if (g_init_done)
        return;

    g_init_done = true;

    sh_render_init();
    g_render_ready = sh_font_init();
}

i32 __cdecl sh_create_move_hook(f32 frametime, sh_usercmd_t *cmd)
{
    if (!cmd)
        return g_create_move_orig ? g_create_move_orig(frametime, cmd) : 0;

    i32 result = 1;

    if (g_create_move_orig)
        result = g_create_move_orig(frametime, cmd);

    if (!sh_is_ready())
        return result;

    sh_players_collect();

    sh_rage_run(cmd);

    if (g_cfg.antiaim.enabled)
        sh_antiaim_run(cmd);

    sh_movement_run(cmd);

    sh_visuals_thirdperson(cmd);

    sh_fakelag_run(cmd);

    sh_misc_run(cmd);

    return result;
}

i32 __cdecl sh_hud_redraw_hook(f32 time, i32 intermission)
{
    i32 result = 0;

    if (g_hud_redraw_orig)
        result = g_hud_redraw_orig(time, intermission);

    if (!sh_is_ready())
        return result;

    g_frame_time = time;

    sh_first_init();

    sh_msgs_register();

    i32 w = 640;
    i32 h = 480;

    if (g_engfuncs && g_engfuncs->pfnGetScreenInfo)
    {
        sh_screeninfo_t info;
        for (u32 i = 0; i < sizeof(info); ++i)
            ((u8 *)&info)[i] = 0;
        if (g_engfuncs->pfnGetScreenInfo(&info))
        {
            w = info.width;
            h = info.height;
        }
    }

    sh_render_begin(w, h);

    sh_visuals_effects_tick(time);
    sh_skybox_tick();
    sh_visuals_apply_nightmode();
    sh_worldfx_tick();

    if (g_cfg.visuals.enabled)
        sh_esp_run();

    sh_visuals_effects_draw();
    sh_hitmarker_draw();
    sh_indicators_draw();

    sh_menu_draw();

    sh_render_end();

    return result;
}

i32 __cdecl sh_player_move_hook(sh_playermove_t *pmove, i32 server)
{
    g_pmove = pmove;

    if (g_player_move_orig)
        g_player_move_orig(pmove, server);

    if (pmove && g_cfg.misc.no_fall_damage)
        sh_fall_predict(pmove);

    return 0;
}

i32 __cdecl sh_add_entity_hook(i32 type, sh_cl_entity_t *ent)
{
    i32 result = 0;

    if (g_add_entity_orig)
        result = g_add_entity_orig(type, ent);

    if (!ent)
        return result;

    if (type != 0)
    {
        if (g_cfg.misc.no_smoke && ent->curstate.renderfx == 20)
            return 0;

        if (g_cfg.misc.no_flash && ent->curstate.renderfx == 19)
            return 0;

        return result;
    }

    if (g_cfg.visuals.chams || g_cfg.visuals.glow)
    {
        ent->curstate.rendermode = 0;
        ent->curstate.renderamt = g_cfg.visuals.glow ? 60 : 255;
        if (g_cfg.visuals.glow)
            ent->curstate.renderfx = 20;
    }

    return result;
}

i32 __cdecl sh_user_msg_hook(const char *name, i32 size)
{
    if (g_user_msg_orig)
        return g_user_msg_orig(name, size);
    return 0;
}

qbool sh_hooks_install(void)
{
    if (!sh_is_ready())
        return false;

    void *create_move = sh_find_export_body("client.dll", "CL_CreateMove");
    void *hud_redraw = sh_find_export_body("client.dll", "HUD_Redraw");
    void *player_move = sh_find_export_body("client.dll", "HUD_PlayerMove");

    if (MH_Initialize() != MH_OK)
        return false;

    if (create_move)
    {
        if (MH_CreateHook(create_move, (void *)sh_create_move_hook,
                          (void **)&g_create_move_orig) == MH_OK)
            MH_EnableHook(create_move);
    }

    if (hud_redraw)
    {
        if (MH_CreateHook(hud_redraw, (void *)sh_hud_redraw_hook,
                          (void **)&g_hud_redraw_orig) == MH_OK)
            MH_EnableHook(hud_redraw);
    }

    void *add_entity = sh_find_export_body("client.dll", "HUD_AddEntity");
    if (add_entity)
    {
        if (MH_CreateHook(add_entity, (void *)sh_add_entity_hook,
                          (void **)&g_add_entity_orig) == MH_OK)
            MH_EnableHook(add_entity);
    }

    if (player_move)
    {
        if (MH_CreateHook(player_move, (void *)sh_player_move_hook,
                          (void **)&g_player_move_orig) == MH_OK)
            MH_EnableHook(player_move);
    }

    return true;
}

void sh_hooks_uninstall(void)
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    sh_font_shutdown();
}
