#include "../../include/sh_hooks.h"
#include "../../include/sh_config.h"
#include "../../include/sh_features.h"
#include "../../include/sh_render.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_scanner.h"
#include "../../include/sh_font.h"
#include "../../include/sh_movement.h"
#include "../../include/sh_bones.h"

void sh_players_track_anim(void);
#include "../../include/sh_ballistics.h"
#include "../../include/sh_visuals.h"
#include "../../include/sh_fakelag.h"
#include "../../include/sh_rage.h"
#include "../../include/sh_skybox.h"
#include "../../include/sh_hitmarker.h"
#include "../../include/sh_worldfx.h"
#include "../../include/sh_movement.h"
#include "../../include/sh_bones.h"

void sh_players_track_anim(void);

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

typedef void(__cdecl *txfer_override_fn)(sh_entity_state_t *state,
                                         const sh_client_data_t *client);

static txfer_override_fn g_txfer_override_orig;

void __cdecl sh_txfer_override_hook(sh_entity_state_t *state,
                                    const sh_client_data_t *client)
{
    if (g_txfer_override_orig)
        g_txfer_override_orig(state, client);

    if (!state || !client)
        return;

    if (g_cfg.antiaim.fake_duck)
        state->iuser3 = 0;

    if (g_cfg.misc.hideshot)
        state->iuser4 = 0;
}

typedef void(__cdecl *camera_offset_fn)(f32 *ofs);
typedef i32(__cdecl *is_thirdperson_fn)(void);

static camera_offset_fn g_camera_offset_orig;
static is_thirdperson_fn g_is_thirdperson_orig;

void __cdecl sh_camera_offset_hook(f32 *ofs)
{
    if (g_camera_offset_orig)
        g_camera_offset_orig(ofs);

    if (!ofs || !g_cfg.visuals.thirdperson)
        return;

    f32 dist = g_cfg.visuals.thirdperson_dist;

    if (dist < 32.0f)
        dist = 32.0f;
    if (dist > 200.0f)
        dist = 200.0f;

    ofs[2] = dist;
}

i32 __cdecl sh_is_thirdperson_hook(void)
{
    i32 r = 0;

    if (g_is_thirdperson_orig)
        r = g_is_thirdperson_orig();

    if (g_cfg.visuals.thirdperson)
        return 1;

    return r;
}

typedef i32(__cdecl *studio_iface_fn)(i32 version, void **ppinterface,
                                     sh_studio_api_t *pstudio);

static studio_iface_fn g_studio_iface_orig;

i32 __cdecl sh_studio_iface_hook(i32 version, void **ppinterface,
                                 sh_studio_api_t *pstudio)
{
    i32 r = 0;

    if (g_studio_iface_orig)
        r = g_studio_iface_orig(version, ppinterface, pstudio);

    if (pstudio)
        sh_bones_studio_set(pstudio);

    if (ppinterface && *ppinterface)
        sh_bones_hook_iface((sh_studio_iface_t *)*ppinterface);

    return r;
}

sh_weapon_data_t g_weapon_data[sh_max_weapons];

typedef void(__cdecl *txfer_pred_fn)(void *ps, const void *pps, void *pcd,
                                     const void *ppcd, sh_weapon_data_t *wd,
                                     const sh_weapon_data_t *pwd);

static txfer_pred_fn g_txfer_pred_orig;

void __cdecl sh_txfer_pred_hook(void *ps, const void *pps, void *pcd,
                                const void *ppcd, sh_weapon_data_t *wd,
                                const sh_weapon_data_t *pwd)
{
    if (g_txfer_pred_orig)
        g_txfer_pred_orig(ps, pps, pcd, ppcd, wd, pwd);

    if (wd)
    {
        for (i32 i = 0; i < sh_max_weapons; ++i)
            g_weapon_data[i] = wd[i];
    }
}

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
    sh_players_track_anim();

    sh_rage_run(cmd);
    sh_rage_apply_recoil(cmd);

    if (g_cfg.antiaim.enabled)
        sh_antiaim_run(cmd);

    sh_movement_run(cmd);


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
    sh_grenade_path_draw();
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

    if (g_cfg.misc.hideshot && ent->index == g_local_player)
    {
        ent->curstate.effects &= ~2;
        ent->curstate.weaponanim = 0;
    }

    if (g_cfg.visuals.chams || g_cfg.visuals.glow)
    {
        if (g_cfg.visuals.glow)
        {
            f32 amt = g_cfg.visuals.glow_amount;
            if (amt < 20.0f)
                amt = 20.0f;
            if (amt > 255.0f)
                amt = 255.0f;
            ent->curstate.renderamt = (i32)amt;
            ent->curstate.renderfx = 20;
            ent->curstate.rendermode = 0;
        }
        else
        {
            ent->curstate.rendermode = 0;
            ent->curstate.renderamt = 255;
            ent->curstate.renderfx = 0;
        }

        if (g_cfg.visuals.chams_team_color)
        {
            i32 idx = ent->player;

            if (idx > 0 && idx <= 32)
            {
                sh_player_t *pl = sh_player_get(idx);

                if (pl)
                {
                    if (pl->team == 1)
                        ent->curstate.rendercolor = (sh_color24_t){ 255, 90, 90 };
                    else if (pl->team == 2)
                        ent->curstate.rendercolor = (sh_color24_t){ 90, 140, 255 };
                }
            }
        }
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

    void *ovr = sh_find_export_body("client.dll", "HUD_TxferLocalOverrides");
    if (ovr)
    {
        if (MH_CreateHook(ovr, (void *)sh_txfer_override_hook,
                          (void **)&g_txfer_override_orig) == MH_OK)
            MH_EnableHook(ovr);
    }

    void *cam = sh_find_export_body("client.dll", "CL_CameraOffset");
    if (cam)
    {
        if (MH_CreateHook(cam, (void *)sh_camera_offset_hook,
                          (void **)&g_camera_offset_orig) == MH_OK)
            MH_EnableHook(cam);
    }

    void *tpp = sh_find_export_body("client.dll", "CL_IsThirdPerson");
    if (tpp)
    {
        if (MH_CreateHook(tpp, (void *)sh_is_thirdperson_hook,
                          (void **)&g_is_thirdperson_orig) == MH_OK)
            MH_EnableHook(tpp);
    }

    void *studio = sh_find_export_body("client.dll", "HUD_GetStudioModelInterface");
    if (studio)
    {
        if (MH_CreateHook(studio, (void *)sh_studio_iface_hook,
                          (void **)&g_studio_iface_orig) == MH_OK)
            MH_EnableHook(studio);
    }

    void *txfer = sh_find_export_body("client.dll", "HUD_TxferPredictionData");
    if (txfer)
    {
        if (MH_CreateHook(txfer, (void *)sh_txfer_pred_hook,
                          (void **)&g_txfer_pred_orig) == MH_OK)
            MH_EnableHook(txfer);
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
