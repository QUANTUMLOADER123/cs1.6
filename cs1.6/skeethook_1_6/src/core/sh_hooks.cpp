#include "../../include/sh_hooks.h"
#include "../../include/sh_config.h"
#include "../../include/sh_features.h"
#include "../../include/sh_imgui.h"
#include "../../include/sh_icons.h"
#include "../../include/sh_render.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_math.h"
#include "../../include/sh_scanner.h"
#include "../../include/sh_crash.h"
#include "../../include/sh_font.h"
#include "../../include/sh_nospread.h"
#include "../../include/sh_modelchanger.h"

qbool sh_soundesp_init(void);
void sh_soundesp_draw(void);
void sh_soundesp_tick(void);
void sh_wallbang_draw(void);
void sh_hitbox_scan_draw(void);
void sh_viewmodel_tick(void);
void sh_fov_apply(void);
void sh_aspect_init(void);
void sh_nightmode_init(void);
void sh_killsound_init(void);
void sh_scope_init(void);
void sh_tracer3d_init(void);
qbool sh_chams_gl_init(void);
qbool sh_chams_gl_ready(void);
qbool sh_skybox_fast_init(void);
void sh_misc_screen_init(void);
void sh_tracer3d_add(const vec3_t *start, const vec3_t *end);
qbool sh_tracer3d_ready(void);
void sh_scope_draw(void);
void sh_nightmode_poll(void);
qbool sh_nightmode_ready(void);
void sh_aspect_poll(void);
void sh_aspect_shutdown(void);
qbool sh_aspect_ready(void);
void sh_remove_bob(void);
void sh_fog_apply(void);
void sh_entityesp_add(sh_cl_entity_t *ent, const char *model_name);
void sh_entityesp_draw(void);
qbool sh_trace_fraction(const vec3_t *start, const vec3_t *end, f32 *frac);
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
#include "../../include/sh_bones.h"

void sh_players_track_anim(void);

void sh_indicators_draw(void);
#include "../../include/sh_visuals.h"
#include "../../include/sh_features.h"


#include <windows.h>
#include "../ext/minhook/include/MinHook.h"

typedef i32(__cdecl *create_move_fn)(f32 frametime, sh_usercmd_t *cmd);
typedef i32(__cdecl *hud_redraw_fn)(f32 time, i32 intermission);

typedef struct sh_stdioevent_s
{
    i32 frame;
    i32 event;
    i32 type;
    char options[64];
} sh_stdioevent_t;

typedef struct sh_local_state_s
{
    sh_entity_state_t playerstate;
    sh_client_data_t client;
    sh_weapon_data_t weapondata[32];
} sh_local_state_t;

typedef i32(__cdecl *sh_slot_fn_i32)(void);
typedef void(__cdecl *hud_studio_event_fn)(const sh_stdioevent_t *ev,
                                           sh_cl_entity_t *ent);
typedef void(__cdecl *hud_post_run_cmd_fn)(sh_local_state_t *from,
                                           sh_local_state_t *to,
                                           sh_usercmd_t *cmd, i32 runfuncs,
                                           f64 time, u32 random_seed);
typedef i32(__cdecl *player_move_fn)(sh_playermove_t *pmove, i32 server);
typedef i32(__cdecl *hook_user_msg_fn)(const char *name, i32 size);
typedef i32(__cdecl *add_entity_fn)(i32 type, sh_cl_entity_t *ent,
                                    const char *model_name);

typedef void(__cdecl *txfer_override_fn)(sh_entity_state_t *state,
                                         const sh_client_data_t *client);

static txfer_override_fn g_txfer_override_orig;

f32 g_local_fov = 90.0f;
i32 g_local_weapon_id;

void __cdecl sh_txfer_override_hook(sh_entity_state_t *state,
                                    const sh_client_data_t *client)
{
    if (g_txfer_override_orig)
        g_txfer_override_orig(state, client);

    if (client)
    {
        g_local_fov = client->fov;
        g_local_weapon_id = client->weapon_id;
    }

    (void)state;
}

typedef void(__cdecl *camera_offset_fn)(f32 *ofs);
typedef i32(__cdecl *is_thirdperson_fn)(void);

static camera_offset_fn g_camera_offset_orig;
static is_thirdperson_fn g_is_thirdperson_orig;

void __cdecl sh_camera_offset_hook(f32 *ofs)
{
    if (g_camera_offset_orig)
        g_camera_offset_orig(ofs);

    (void)ofs;
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

    if (pcd && g_cfg.visuals.fov_enabled)
    {
        sh_client_data_t *cd = (sh_client_data_t *)pcd;

        f32 f = g_cfg.visuals.fov_value;

        if (f < 40.0f)
            f = 40.0f;
        if (f > 160.0f)
            f = 160.0f;

        if (cd->fov < 90.0f && cd->fov > 1.0f)
        {
            f32 zoom_ratio = cd->fov / 90.0f;

            cd->fov = f * zoom_ratio;
        }
        else
        {
            cd->fov = f;
        }

    }
}

static create_move_fn g_create_move_orig;
static hud_redraw_fn g_hud_redraw_orig;

/* --- ХУК SwapBuffers ---

   HUD_Redraw вызывается только в игре. В главном меню игры и на
   паузе его нет вообще - поэтому там меню не работало.

   SwapBuffers зовётся всегда, когда кадр уходит на экран. Именно
   так сделано в референсе (gold-sdk): вся отрисовка ImGui висит
   на on_swap_buffers, а не на HUD_Redraw. */
typedef BOOL(WINAPI *swap_buffers_fn)(HDC);

/* проверка клавиш меню (INSERT/ESC) - зовётся каждый кадр */
void sh_menu_poll_keys(void);

static swap_buffers_fn g_swap_buffers_orig;

/* Поставлен ли хук SwapBuffers. Если да - меню рисуется ТОЛЬКО
   там, и в HUD_Redraw его рисовать нельзя (два NewFrame за кадр
   ломают ввод). Если хук не встал - рисуем из HUD_Redraw. */
static qbool g_swap_hook_ok;
static hud_studio_event_fn g_hud_studio_event_orig;
static hud_post_run_cmd_fn g_hud_post_run_cmd_orig;
static u32 g_server_random_seed;
static player_move_fn g_player_move_orig;
static hook_user_msg_fn g_user_msg_orig;
static add_entity_fn g_add_entity_orig;

static qbool g_init_done;
static qbool g_render_ready;
static f32 g_frame_time;

static void __cdecl sh_hud_studio_event_hook(const sh_stdioevent_t *ev,
                                            sh_cl_entity_t *ent)
{
    if (ev && ent)
    {
        i32 is_muzzle = (ev->event == 5001 || ev->event == 5011 ||
                         ev->event == 5021 || ev->event == 5031);

        if (is_muzzle)
            sh_events_on_fire(ent->index, &ent->origin, ev->event);
    }

    if (g_hud_studio_event_orig)
        g_hud_studio_event_orig(ev, ent);
}

static void __cdecl sh_hud_post_run_cmd_hook(sh_local_state_t *from,
                                             sh_local_state_t *to,
                                             sh_usercmd_t *cmd, i32 runfuncs,
                                             f64 time, u32 random_seed)
{
    g_server_random_seed = random_seed;
    sh_spread_feed_seed(random_seed);

    if (cmd)
        sh_misc_on_post_run(cmd, runfuncs, time);

    if (g_hud_post_run_cmd_orig)
        g_hud_post_run_cmd_orig(from, to, cmd, runfuncs, time, random_seed);
}

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

static i32 __cdecl sh_trace_wrapper(vec3_t start, vec3_t end, i32 flags,
                                   i32 ignore_pe)
{
    if (!g_pmove || !g_pmove->pm_player_trace)
        return 0;

    sh_pmtrace_t tr = g_pmove->pm_player_trace((f32 *)&start, (f32 *)&end,
                                               flags, ignore_pe);

    if (tr.fraction >= 1.0f && !tr.startsolid && !tr.allsolid)
        return 0;

    if (tr.ent > 0)
        return tr.hitgroup > 0 ? tr.hitgroup : 1;

    return 0;
}

static void sh_engine_trace_setup(void)
{
    if (!g_pmove)
        return;

    if (!g_pmove->pm_player_trace)
        return;

    if (!g_trace_fn)
        sh_set_trace_fn(sh_trace_wrapper);
}

static qbool sh_is_ready(void)
{
    /* именно hookable, не ready: хуки можно ставить без карты,
       g_pmove появится при первом CreateMove */
    if (!sh_sdk_hookable())
    {
        sh_sdk_resolve();
        if (!sh_sdk_hookable())
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
    sh_icons_init();
    sh_soundesp_init();
    sh_events_reset();
    sh_spread_reset();
    sh_nospread_reset();
}

i32 __cdecl sh_create_move_hook(f32 frametime, sh_usercmd_t *cmd)
{
    sh_crash_stage("CreateMove");
    if (!cmd)
        return g_create_move_orig ? g_create_move_orig(frametime, cmd) : 0;

    i32 result = 1;

    if (g_create_move_orig)
        result = g_create_move_orig(frametime, cmd);

    /* здесь нужен ПОЛНЫЙ sdk (включая player_move): фичи читают
       g_pmove. sh_is_ready() теперь проверяет только enginefuncs,
       поэтому дотягиваем player_move отдельно - он появляется,
       когда игрок уже на карте, то есть ровно в CreateMove. */
    if (!sh_sdk_ready())
    {
        sh_sdk_resolve();

        if (!sh_sdk_ready())
            return result;
    }

    if (!g_cfg.ready)
        sh_config_load();

    /* Меню открыто - не даём читу дёргать игрока и не даём крутить
       камеру мышью. Иначе при открытом меню персонаж продолжал
       бежать/прыгать, а прицел вертелся от движений мыши, которыми
       ты водишь по пунктам меню. */
    if (g_cfg.menu_open)
    {
        /* ВАЖНО: это ПОСЛЕ g_create_move_orig, то есть движение
           текущего кадра уже обработано движком. Обнуление cmd тут
           влияет на следующий кадр, поэтому оно работает как
           "залипание" - персонаж останавливается, пока открыто меню.
           Именно то, что нужно: мышью водишь по меню, а не крутишь
           камеру. */
        cmd->buttons = 0;
        cmd->forwardmove = 0.0f;
        cmd->sidemove = 0.0f;
        cmd->upmove = 0.0f;
        return result;
    }

    sh_players_collect();
    sh_players_track_anim();

    sh_rage_run(cmd);
    sh_rage_apply_recoil(cmd);

    vec3_t pre_angles = cmd->viewangles;

    if (g_cfg.antiaim.enabled)
        sh_antiaim_run(cmd);

    f32 yaw_delta = cmd->viewangles.y - pre_angles.y;

    while (yaw_delta > 180.0f)
        yaw_delta -= 360.0f;
    while (yaw_delta < -180.0f)
        yaw_delta += 360.0f;

    if (yaw_delta != 0.0f)
        sh_movement_fix(cmd, yaw_delta);

    sh_movement_run(cmd);


    sh_fakelag_run(cmd);

    sh_misc_run(cmd);

    return result;
}

/* Один кадр меню. Вынесено из HUD_Redraw, потому что теперь
   вызывается из двух мест: HUD_Redraw (в игре) и SwapBuffers
   (главное меню, пауза). */
static void sh_imgui_draw_frame(i32 w, i32 h)
{
    static i32 imgui_tried;

    /* клавиши меню - до всего остального */
    sh_menu_poll_keys();

    if (!sh_imgui_ready() && !imgui_tried)
    {
        imgui_tried = 1;

        if (!sh_imgui_init((void *)sh_game_window_for_imgui()))
            sh_crash_stage("imgui: инициализация не удалась");
    }

    if (!sh_imgui_ready())
        return;

    sh_imgui_begin(w, h, sh_menu_visible_now());
    sh_menu_draw();
    sh_imgui_end();
}

/* ХУК SwapBuffers.

   Рисуем меню только если его ещё не нарисовал HUD_Redraw в этом
   кадре (иначе два NewFrame подряд ломают ввод).

   В главном меню и на паузе HUD_Redraw молчит - и меню рисуется
   отсюда. Это и есть лечение "в мейн меню не ворк". */
static BOOL WINAPI sh_swap_buffers_hook(HDC dc)
{
    /* защита от повторного входа: opengl32 и gdi32 могут иметь
       РАЗНЫЕ адреса SwapBuffers, и мы хукаем оба под этот же
       обработчик */
    static i32 busy;

    if (!g_swap_buffers_orig)
        return TRUE;

    if (busy)
        return g_swap_buffers_orig(dc);

    busy = 1;

    /* КЛАВИШИ МЕНЮ ПРОВЕРЯЕМ ЗДЕСЬ, КАЖДЫЙ КАДР.

       Раньше это делалось внутри sh_menu_draw, а он вызывается
       только когда меню уже открыто - замкнутый круг. Плюс в
       главном меню игры sh_menu_draw не зовётся вовсе, поэтому
       меню там не открывалось вообще. */
    sh_menu_poll_keys();

    {
        HWND w = (HWND)sh_game_window_for_imgui();

        if (w && sh_menu_visible_now())
        {
            RECT rc;

            if (GetClientRect(w, &rc))
            {
                if (rc.right > 0 && rc.bottom > 0)
                    sh_imgui_draw_frame(rc.right, rc.bottom);
            }
        }
    }

    busy = 0;

    return g_swap_buffers_orig(dc);
}

i32 __cdecl sh_hud_redraw_hook(f32 time, i32 intermission)
{
    i32 result = 0;

    if (g_hud_redraw_orig)
        result = g_hud_redraw_orig(time, intermission);

    /* Пока sdk не готов, меню и watermark всё равно надо показать:
       иначе неоткрывающееся меню неотличимо от "dll не работает".
       Рисуем в отдельном проходе (render_begin/end) и выходим. */
    if (!sh_is_ready())
    {
        /* Пока sdk не готов, всё равно показываем watermark - иначе
           "меню не открывается" неотличимо от "dll не работает".

           Рисовать можно только когда GL-процедуры уже получены;
           sh_render_init() грузит opengl32.dll и кэширует указатели,
           поэтому делаем её здесь (это дешёвый GetProcAddress,
           повторный вызов безопасен) и проверяем sh_render_ready(). */
        static i32 init_tried;

        if (!init_tried)
        {
            sh_render_init();
            init_tried = 1;
        }

        if (sh_render_ready())
        {
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
            sh_menu_watermark_only();
            sh_render_end();
        }

        return result;
    }

    sh_crash_stage("HUD_Redraw: первый init");

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

    sh_crash_stage("HUD_Redraw: render_begin");

    sh_render_begin(w, h);

    sh_crash_stage("HUD_Redraw: тики фич");

    sh_visuals_effects_tick(time);
    sh_skybox_tick();
    sh_model_changer_tick();
    sh_viewmodel_tick();
    sh_fov_apply();
    sh_aspect_poll();
    sh_nightmode_poll();
    sh_remove_bob();
    sh_visuals_apply_nightmode();
    sh_worldfx_tick();

    sh_crash_stage("HUD_Redraw: ESP");

    if (g_cfg.visuals.enabled)
        sh_esp_run();

    sh_crash_stage("HUD_Redraw: отрисовка");

    sh_visuals_effects_draw();
    sh_soundesp_draw();
    sh_wallbang_draw();
    sh_hitbox_scan_draw();
    sh_entityesp_draw();
    sh_grenade_path_draw();
    sh_hitmarker_draw();
    sh_indicators_draw();
    sh_scope_draw();

    sh_crash_stage("HUD_Redraw: меню");

    /* МЕНЮ.

       Рисуем общей функцией: в игре её зовёт этот хук, а в
       главном меню и на паузе - хук SwapBuffers, потому что
       HUD_Redraw там не вызывается вообще.

       sh_render_end() идёт ПОСЛЕ меню: он возвращает движку
       GL_TEXTURE_2D, которое sh_render_begin() выключает для ESP.
       Без этого мир рисовался без текстур. Перед меню его звать
       нельзя - он выключает блендинг, и ImGui теряет прозрачность. */
    if (!g_swap_hook_ok)
        sh_imgui_draw_frame(w, h);

    sh_render_end();

    return result;
}

i32 __cdecl sh_player_move_hook(sh_playermove_t *pmove, i32 server)
{
    sh_crash_stage("PlayerMove");
    g_pmove = pmove;

    sh_engine_trace_setup();

    if (g_player_move_orig)
        g_player_move_orig(pmove, server);

    if (pmove && g_cfg.misc.no_fall_damage)
        sh_fall_predict(pmove);

    if (pmove && g_cfg.misc.no_recoil_effect)
    {
        pmove->punchangle.x = 0.0f;
        pmove->punchangle.y = 0.0f;
        pmove->punchangle.z = 0.0f;
    }


    return 0;
}

i32 __cdecl sh_add_entity_hook(i32 type, sh_cl_entity_t *ent,
                               const char *model_name)
{
    i32 result = 0;

    if (g_cfg.misc.no_smoke && model_name && type == 2)
    {
        const char *base = model_name;
        const char *p = model_name;

        while (*p)
        {
            if (*p == '/' || *p == '\\')
                base = p + 1;

            ++p;
        }

        if (g_cfg.misc.no_scope_borders)
        {
            const char *sc = (const char *)base;
            i32 is_scope = 0;

            {
                const char *pat = "scope_arc";
                i32 ok = 1;

                for (i32 c = 0; pat[c]; ++c)
                {
                    char a = sc[c];

                    if (a >= 'A' && a <= 'Z')
                        a = (char)(a + 32);
                    if (a != pat[c])
                    {
                        ok = 0;
                        break;
                    }
                }

                if (ok)
                    is_scope = 1;
            }

            if (!is_scope)
            {
                const char *pat2 = "crosshairs/scope";
                i32 ok = 1;

                for (i32 c = 0; pat2[c]; ++c)
                {
                    char a = sc[c];

                    if (a >= 'A' && a <= 'Z')
                        a = (char)(a + 32);
                    if (a != pat2[c])
                    {
                        ok = 0;
                        break;
                    }
                }

                if (ok)
                    is_scope = 1;
            }

            if (is_scope)
                return 0;
        }

        const char *sm_names[] = {
            "black_smoke", "black_smoke1", "black_smoke2",
            "black_smoke3", "black_smoke4", "smokepuff", "gas_puff"
        };

        for (u32 k = 0; k < sh_countof(sm_names); ++k)
        {
            const char *n = sm_names[k];
            i32 match = 1;

            for (i32 c = 0; n[c]; ++c)
            {
                char a = base[c];
                char b = n[c];

                if (a >= 'A' && a <= 'Z')
                    a = (char)(a + 32);

                if (a != b)
                {
                    match = 0;
                    break;
                }
            }

            if (match && base[0])
            {
                if (g_cfg.misc.no_smoke)
                    return 0;

                if (ent && g_cfg.visuals.smoke_edit)
                {
                    f32 a = g_cfg.visuals.smoke_alpha;

                    if (a < 0.05f)
                        a = 0.05f;
                    if (a > 1.0f)
                        a = 1.0f;

                    ent->curstate.rendermode = sh_rendermode_transalpha;
                    ent->curstate.renderamt = (i32)(a * 255.0f);
                    ent->curstate.rendercolor = (sh_color24_t){
                        (u8)g_cfg.visuals.smoke_r,
                        (u8)g_cfg.visuals.smoke_g,
                        (u8)g_cfg.visuals.smoke_b
                    };
                }

                break;
            }
        }
    }

    if (g_cfg.visuals.entity_esp && ent && model_name)
        sh_entityesp_add(ent, model_name);

    if (g_add_entity_orig)
        result = g_add_entity_orig(type, ent, model_name);

    if (!ent)
        return result;


    if (type != 0)
        return result;

    if (g_cfg.misc.hideshot && ent->index == g_local_player)
    {
        ent->curstate.effects &= ~sh_ef_muzzleflash;
        ent->curstate.weaponanim = 0;
    }

    if (g_cfg.visuals.glow)
    {
        f32 amt = g_cfg.visuals.glow_amount;

        if (amt < 20.0f)
            amt = 20.0f;
        if (amt > 255.0f)
            amt = 255.0f;

        ent->curstate.renderfx = sh_renderfx_glowshell;
        ent->curstate.renderamt = (i32)amt;
        ent->curstate.rendermode = sh_rendermode_normal;

        if (g_cfg.visuals.chams_team_color)
        {
            i32 idx = ent->player;

            if (idx > 0 && idx <= 32)
            {
                sh_player_t *pl = sh_player_get(idx);

                if (pl && pl->team == 1)
                    ent->curstate.rendercolor = (sh_color24_t){ 255, 90, 90 };
                else if (pl && pl->team == 2)
                    ent->curstate.rendercolor = (sh_color24_t){ 90, 140, 255 };
            }
        }
    }
    else if (g_cfg.visuals.chams && !sh_chams_gl_ready())
    {
        sh_player_t *pl = 0;

        if (ent->player > 0 && ent->player <= 32)
            pl = sh_player_get(ent->player);

        qbool visible = pl ? (pl->visible != 0) : 1;

        col4_t col = g_cfg.visuals.col_chams_vis;

        if (!visible)
        {
            if (!g_cfg.visuals.chams_xqz)
            {
                ent->curstate.rendermode = sh_rendermode_normal;
                ent->curstate.renderamt = 255;
                ent->curstate.renderfx = sh_renderfx_none;
                return result;
            }

            col = g_cfg.visuals.col_chams_xqz;
        }

        if (g_cfg.visuals.chams_team_color && pl)
        {
            if (pl->team == 1)
                col = (col4_t){ 255, 90, 90, col.a };
            else if (pl->team == 2)
                col = (col4_t){ 90, 140, 255, col.a };
        }

        ent->curstate.rendermode = sh_rendermode_transalpha;
        ent->curstate.renderamt = col.a;
        ent->curstate.renderfx = sh_renderfx_none;
        ent->curstate.rendercolor = (sh_color24_t){ col.r, col.g, col.b };
    }

    return result;
}

i32 __cdecl sh_user_msg_hook(const char *name, i32 size)
{
    if (g_user_msg_orig)
        return g_user_msg_orig(name, size);
    return 0;
}

typedef void(__cdecl *calc_refdef_fn)(sh_ref_params_t *pparams);

static calc_refdef_fn g_calc_refdef_orig;

void __cdecl sh_calc_refdef_hook(sh_ref_params_t *pparams)
{
    if (g_calc_refdef_orig)
        g_calc_refdef_orig(pparams);

    if (!pparams)
        return;

    if (g_cfg.misc.no_recoil_effect)
    {
        pparams->punchangle.x = 0.0f;
        pparams->punchangle.y = 0.0f;
        pparams->punchangle.z = 0.0f;

        if (g_cfg.misc.no_recoil)
            pparams->viewangles = pparams->cl_viewangles;
    }

    if (g_cfg.visuals.thirdperson && g_cfg.visuals.thirdperson_dist > 0.0f)
    {
        f32 dist = g_cfg.visuals.thirdperson_dist;

        if (dist < 32.0f)
            dist = 32.0f;
        if (dist > 400.0f)
            dist = 400.0f;

        vec3_t eye = pparams->vieworg;
        vec3_t want;

        want.x = eye.x - pparams->forward.x * dist;
        want.y = eye.y - pparams->forward.y * dist;
        want.z = eye.z - pparams->forward.z * dist;

        f32 frac = 1.0f;

        if (sh_trace_fraction(&eye, &want, &frac) && frac < 1.0f)
        {
            if (frac < 0.05f)
                frac = 0.05f;

            want.x = eye.x + (want.x - eye.x) * frac;
            want.y = eye.y + (want.y - eye.y) * frac;
            want.z = eye.z + (want.z - eye.z) * frac;
        }

        pparams->vieworg = want;
    }
}

typedef void(__cdecl *draw_tris_fn)(void);

static draw_tris_fn g_draw_normal_orig;

void __cdecl sh_draw_normal_tris_hook(void)
{
    if (g_draw_normal_orig)
        g_draw_normal_orig();

    sh_fog_apply();
}

qbool sh_hooks_install(void)
{
    if (!sh_is_ready())
        return false;

    MH_STATUS init_status = MH_Initialize();

    if (init_status != MH_OK && init_status != MH_ERROR_ALREADY_INITIALIZED)
        return false;

    void *studio_event = sh_find_export_body("client.dll", "HUD_StudioEvent");
    void *post_run_cmd = sh_find_export_body("client.dll", "HUD_PostRunCmd");

    if (studio_event)
    {
        if (MH_CreateHook(studio_event, (void *)sh_hud_studio_event_hook,
                          (void **)&g_hud_studio_event_orig) == MH_OK)
            MH_EnableHook(studio_event);
    }

    if (post_run_cmd)
    {
        if (MH_CreateHook(post_run_cmd, (void *)sh_hud_post_run_cmd_hook,
                          (void **)&g_hud_post_run_cmd_orig) == MH_OK)
            MH_EnableHook(post_run_cmd);
    }

    void *draw_normal = sh_find_export_body("client.dll", "HUD_DrawNormalTriangles");

    if (draw_normal)
    {
        if (MH_CreateHook(draw_normal, (void *)sh_draw_normal_tris_hook,
                          (void **)&g_draw_normal_orig) == MH_OK)
            MH_EnableHook(draw_normal);
    }

    void *calc_refdef = sh_find_export_body("client.dll", "V_CalcRefdef");

    if (calc_refdef)
    {
        if (MH_CreateHook(calc_refdef, (void *)sh_calc_refdef_hook,
                          (void **)&g_calc_refdef_orig) == MH_OK)
            MH_EnableHook(calc_refdef);
    }

    void *create_move = sh_find_export_body("client.dll", "CL_CreateMove");
    void *hud_redraw = sh_find_export_body("client.dll", "HUD_Redraw");
    void *player_move = sh_find_export_body("client.dll", "HUD_PlayerMove");

    sh_aspect_init();
    sh_nightmode_init();
    sh_killsound_init();
    sh_scope_init();
    sh_tracer3d_init();
    sh_chams_gl_init();
    sh_matrix_init();
    sh_misc_screen_init();
    sh_skybox_fast_init();

    i32 essential = 0;

    if (create_move)
    {
        if (MH_CreateHook(create_move, (void *)sh_create_move_hook,
                          (void **)&g_create_move_orig) == MH_OK)
        {
            MH_EnableHook(create_move);
            ++essential;
        }
    }

    if (hud_redraw)
    {
        if (MH_CreateHook(hud_redraw, (void *)sh_hud_redraw_hook,
                          (void **)&g_hud_redraw_orig) == MH_OK)
        {
            MH_EnableHook(hud_redraw);
            ++essential;
        }
    }

    /* ХУК SwapBuffers - чтобы меню работало в главном меню игры и
       на паузе, где HUD_Redraw не вызывается.

       Адрес берём в gdi32.dll: игра его импортирует, значит
       библиотека уже загружена. */
    {
        HMODULE gdi = GetModuleHandleA("gdi32.dll");

        if (!gdi)
            gdi = LoadLibraryA("gdi32.dll");

        /* ВАЖНО: SwapBuffers есть и в gdi32.dll, и в opengl32.dll
           (там он реэкспортирован). Обычно это ОДИН адрес, но
           если библиотеки разные - игра может звать через
           opengl32, и хук по gdi32 не поймает.

           Поэтому берём адрес из gdi32; если opengl32 отдаёт
           ДРУГОЙ адрес - хукаем его тоже (второй вызов под тот
           же обработчик). Оригинал сохраняем только один раз:
           оба ведут в одну и ту же функцию Windows. */
        if (gdi)
        {
            void *swap = (void *)GetProcAddress(gdi, "SwapBuffers");

            if (swap)
            {
                if (MH_CreateHook(swap, (void *)sh_swap_buffers_hook,
                                  (void **)&g_swap_buffers_orig) == MH_OK)
                {
                    MH_EnableHook(swap);
                    g_swap_hook_ok = 1;
                    ++essential;
                }

                /* opengl32.dll может отдавать СВОЙ адрес */
                {
                    HMODULE ogl = GetModuleHandleA("opengl32.dll");

                    if (ogl)
                    {
                        void *swap2 = (void *)GetProcAddress(ogl, "SwapBuffers");

                        if (swap2 && swap2 != swap)
                        {
                            if (MH_CreateHook(swap2,
                                              (void *)sh_swap_buffers_hook,
                                              0) == MH_OK)
                            {
                                MH_EnableHook(swap2);
                                g_swap_hook_ok = 1;
                            }
                        }
                    }
                }
            }
        }
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
        {
            MH_EnableHook(player_move);
            ++essential;
        }
    }

    return essential > 0;
}

void sh_hooks_uninstall(void)
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    sh_font_shutdown();
}
