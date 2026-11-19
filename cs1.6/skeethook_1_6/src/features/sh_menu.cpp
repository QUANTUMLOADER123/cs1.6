#include "../../include/sh_config.h"
#include "../../include/sh_modelchanger.h"
#include "../../include/sh_movement.h"
#include "../../include/sh_grenade.h"
#include "../../include/sh_skybox.h"
#include "../../include/sh_hitmarker.h"
#include "../../include/sh_render.h"
#include "../../include/sh_types.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_features.h"

#include <windows.h>
#include <stdio.h>

/* =====================  МЕНЮ НА IMGUI  =====================

   Меню переписано на Dear ImGui. Причина простая: самописные
   виджеты приходилось чинить по одному - клики по фронту, курсор,
   отсечение прокрутки, шрифт, перетаскивание окна. Всё это в ImGui
   уже есть и работает.

   Что важно понимать про интеграцию:
   - ImGui рисует только то, что ему передали в DrawData. Ввод
     (позиция мыши, кнопки, клавиши) передаётся руками в
     sh_imgui_begin() - иначе меню рисуется, но не реагирует.
   - Перетаскивание окна делает сам ImGui за заголовок. Наш прежний
     костыль с g_dragging_wnd больше не нужен.
   - Скролл и колесо - тоже ImGui, включая Child-области.
*/

#include "../../ext/imgui/imgui.h"
#include "../../include/sh_imgui.h"

static qbool g_menu_visible;

/* масштаб интерфейса от высоты экрана (база 480p) */
static f32 g_ui_scale = 1.0f;
static i32 g_cur_tab;

static void sh_menu_release_mouse(qbool release);

static HWND sh_game_window(void)
{
    static HWND cached;

    if (cached && IsWindow(cached))
        return cached;

    cached = FindWindowA("Valve001", 0);

    if (!cached)
        cached = GetForegroundWindow();

    return cached;
}

void *sh_game_window_for_imgui(void)
{
    return (void *)sh_game_window();
}

typedef void (*in_mouse_fn)(void);

static in_mouse_fn sh_in_mouse(const char *name)
{
    static HMODULE client;

    if (!client)
        client = GetModuleHandleA("client.dll");

    return client ? (in_mouse_fn)(void *)GetProcAddress(client, name) : 0;
}

static void sh_menu_release_mouse(qbool release)
{
    if (release)
    {
        in_mouse_fn deact = sh_in_mouse("IN_DeactivateMouse");

        if (deact)
            deact();

        ClipCursor(0);
        ReleaseCapture();

        {
            i32 guard = 0;

            while (ShowCursor(TRUE) < 0 && ++guard < 32)
                ;
        }

        SetCursor(LoadCursorA(0, IDC_ARROW));
    }
    else
    {
        in_mouse_fn act = sh_in_mouse("IN_ActivateMouse");

        if (act)
            act();

        ClipCursor(0);
        ShowCursor(FALSE);

        {
            HWND w = sh_game_window();

            if (w)
            {
                RECT rc;

                if (GetClientRect(w, &rc))
                {
                    POINT c;

                    c.x = (rc.right - rc.left) / 2;
                    c.y = (rc.bottom - rc.top) / 2;
                    ClientToScreen(w, &c);
                    SetCursorPos(c.x, c.y);
                }
            }
        }
    }
}


/* золотисто-зелёный акцент как в конфиге */

static const char *g_tab_names[] = {
    "rage", "antiaim", "visuals", "misc", "movement", "settings"
};

static const char *g_mp_items[] = { "off", "center", "corners" };
static const char *g_bt_items[] = { "off", "1 frame", "2 frames", "3 frames", "4 frames", "6 frames" };
static const char *font_items[] = { "default", "small", "tiny" };
static const char *g_espc_items[] = { "visible", "health", "team" };
static const char *g_fog_items[] = { "normal", "thick", "far" };

static const char *g_hitbox_items[] = {"head", "neck", "chest", "stomach", "pelvis", "legs"};

static const char *g_priority_items[] = {"distance", "fov", "damage", "low health"};

static const char *g_pitch_items[] = {"none", "down", "up", "fake down", "fake up", "spin", "static"};

static const char *g_yaw_items[] = {"none", "backwards", "static left", "static right", "left", "right",
    "spin", "static", "random", "jitter", "fake sideways", "fake edge",
    "at target", "at target jitter"};

static void sh_menu_tab_rage(void)
{
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "AIMBOT");
    ImGui::Separator();
    ImGui::Checkbox("enabled", (bool *)&g_cfg.aimbot.enabled);
    ImGui::Checkbox("silent", (bool *)&g_cfg.aimbot.silent);
    ImGui::Checkbox("auto fire", (bool *)&g_cfg.aimbot.auto_fire);
    ImGui::Checkbox("auto wall", (bool *)&g_cfg.aimbot.auto_wall);
    ImGui::Checkbox("penetration", (bool *)&g_cfg.aimbot.penetration);
    ImGui::Checkbox("double tap", (bool *)&g_cfg.aimbot.double_tap);
    ImGui::Checkbox("auto stop", (bool *)&g_cfg.aimbot.auto_stop);
    ImGui::Checkbox("auto stop duck", (bool *)&g_cfg.aimbot.auto_stop_duck);
    ImGui::Checkbox("auto scope", (bool *)&g_cfg.aimbot.auto_scope);
    ImGui::Checkbox("visible check", (bool *)&g_cfg.aimbot.visible_check);
    ImGui::Checkbox("smoke check", (bool *)&g_cfg.aimbot.smoke_check);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "TARGET");
    ImGui::Separator();
    ImGui::Combo("hitbox", (int *)&g_cfg.aimbot.hitbox, g_hitbox_items, 6);
    ImGui::Combo("multipoint", (int *)&g_cfg.aimbot.multipoint, g_mp_items, 3);
    ImGui::Combo("backtrack", (int *)&g_cfg.aimbot.backtrack, g_bt_items, 6);
    ImGui::Combo("priority", (int *)&g_cfg.aimbot.priority, g_priority_items, 4);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "TUNING");
    ImGui::Separator();
    ImGui::SliderFloat("fov", (float *)&g_cfg.aimbot.fov, 1.0f, 180.0f);
    ImGui::SliderFloat("smooth", (float *)&g_cfg.aimbot.smooth, 1.0f, 20.0f);
    ImGui::SliderFloat("min damage", (float *)&g_cfg.aimbot.min_damage, 1.0f, 100.0f);
}

static void sh_menu_tab_antiaim(void)
{
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "ANTIAIM");
    ImGui::Separator();
    ImGui::Checkbox("enabled", (bool *)&g_cfg.antiaim.enabled);
    ImGui::SliderFloat("roll", (float *)&g_cfg.antiaim.roll, -50.0f, 50.0f);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "FAKE LAG");
    ImGui::Separator();
    ImGui::Checkbox("enabled", (bool *)&g_cfg.antiaim.fake_lag);
    {   /* fake_lag_ticks - int, а SliderFloat работает с float:
           держим промежуточное значение статиком, чтобы не терять
           его между кадрами (иначе ползунок дёргается) */
        static float fl_ticks = 0.0f;

        fl_ticks = (float)g_cfg.antiaim.fake_lag_ticks;

        if (ImGui::SliderFloat("ticks", &fl_ticks, 0.0f, 22.0f))
            g_cfg.antiaim.fake_lag_ticks = (i32)(fl_ticks + 0.5f);
    }
    ImGui::SliderFloat("spin speed", (float *)&g_cfg.antiaim.spin_speed, 0.0f, 10.0f);
    ImGui::SliderFloat("yaw base", (float *)&g_cfg.antiaim.yaw_base, -180.0f, 180.0f);
    ImGui::SliderFloat("jitter", (float *)&g_cfg.antiaim.yaw_jitter, 0.0f, 90.0f);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "DESYNC");
    ImGui::Separator();
    ImGui::Checkbox("desync", (bool *)&g_cfg.antiaim.desync);
    ImGui::SliderFloat("amount", (float *)&g_cfg.antiaim.desync_amount, 0.0f, 90.0f);
    ImGui::SliderFloat("ticks", (float *)&g_cfg.antiaim.desync_ticks, 1.0f, 8.0f);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "EXTRAS");
    ImGui::Separator();
    ImGui::Checkbox("fake duck", (bool *)&g_cfg.antiaim.fake_duck);
    ImGui::Checkbox("micro move", (bool *)&g_cfg.antiaim.micro_move);
    ImGui::SliderFloat("micro speed", (float *)&g_cfg.antiaim.micro_move_speed, 0.5f, 5.0f);
}

static void sh_menu_tab_visuals(void)
{
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "ESP");
    ImGui::Separator();
    ImGui::Checkbox("enabled", (bool *)&g_cfg.visuals.enabled);
    ImGui::Checkbox("box", (bool *)&g_cfg.visuals.box);
    ImGui::Checkbox("name", (bool *)&g_cfg.visuals.name);
    ImGui::Checkbox("health", (bool *)&g_cfg.visuals.health_bar);
    ImGui::Checkbox("weapon", (bool *)&g_cfg.visuals.weapon);
    ImGui::Checkbox("weapon icon", (bool *)&g_cfg.visuals.weapon_icon);
    ImGui::Checkbox("armor", (bool *)&g_cfg.visuals.armor);
    ImGui::Checkbox("distance", (bool *)&g_cfg.visuals.distance);
    ImGui::Checkbox("skeleton", (bool *)&g_cfg.visuals.skeleton);
    ImGui::Checkbox("head dot", (bool *)&g_cfg.visuals.head_dot);
    ImGui::Checkbox("snaplines", (bool *)&g_cfg.visuals.snaplines);
    ImGui::Checkbox("show local", (bool *)&g_cfg.esp.show_local);
    ImGui::Checkbox("visible only", (bool *)&g_cfg.visuals.visible_only);
    ImGui::SliderFloat("text scale", (float *)&g_cfg.esp.text_scale, 0.5f, 2.5f);
    ImGui::Combo("font", (int *)&g_cfg.esp.font, font_items, 3);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "MODELS");
    ImGui::Separator();
    ImGui::Checkbox("chams", (bool *)&g_cfg.visuals.chams);
    ImGui::Checkbox("glow", (bool *)&g_cfg.visuals.glow);
    ImGui::Checkbox("team colors", (bool *)&g_cfg.visuals.chams_team_color);
    ImGui::Checkbox("team check", (bool *)&g_cfg.visuals.team_check);
    ImGui::Checkbox("xqz chams", (bool *)&g_cfg.visuals.chams_xqz);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "MODEL CHANGER");
    ImGui::Separator();
    ImGui::Checkbox("sound esp", (bool *)&g_cfg.visuals.sound_esp);
    ImGui::Checkbox("player model", (bool *)&g_cfg.visuals.model_changer);
    ImGui::Checkbox("fake markers", (bool *)&g_cfg.visuals.draw_fakes);
    ImGui::Checkbox("entity esp", (bool *)&g_cfg.visuals.entity_esp);
    ImGui::Checkbox("wallbang info", (bool *)&g_cfg.visuals.penetration_info);
    ImGui::Checkbox("position history", (bool *)&g_cfg.visuals.draw_history);
    ImGui::Checkbox("damage info", (bool *)&g_cfg.visuals.damage_info);
    ImGui::Checkbox("damage hitbox", (bool *)&g_cfg.visuals.damage_hitbox);
    ImGui::Checkbox("hitboxes 3d", (bool *)&g_cfg.visuals.hitboxes);
    ImGui::Combo("esp color", (int *)&g_cfg.visuals.esp_color_mode, g_espc_items, 3);
    ImGui::Checkbox("grenade radius", (bool *)&g_cfg.misc.grenade_radius);
    ImGui::Checkbox("hitbox visibility", (bool *)&g_cfg.visuals.hitbox_vis_check);
    ImGui::Checkbox("hitbox scanner", (bool *)&g_cfg.visuals.hitbox_scan);
}

static void sh_menu_tab_misc(void)
{
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "MOVEMENT");
    ImGui::Separator();
    ImGui::Checkbox("edge jump", (bool *)&g_cfg.misc.edge_jump);
    ImGui::Checkbox("fast ladder", (bool *)&g_cfg.misc.fast_ladder);
    ImGui::Checkbox("fall damage warn", (bool *)&g_cfg.misc.no_fall_damage);
    ImGui::Checkbox("auto pistol", (bool *)&g_cfg.misc.auto_pistol);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "COMBAT");
    ImGui::Separator();
    ImGui::Checkbox("no recoil", (bool *)&g_cfg.misc.no_recoil);
    ImGui::Checkbox("no recoil fx", (bool *)&g_cfg.misc.no_recoil_effect);
    ImGui::Checkbox("no spread", (bool *)&g_cfg.misc.no_spread);
    ImGui::Checkbox("fast reload", (bool *)&g_cfg.misc.fast_reload);
    ImGui::Checkbox("knife bot", (bool *)&g_cfg.misc.knife_bot);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "VISUAL");
    ImGui::Separator();
    ImGui::Checkbox("nightmode", (bool *)&g_cfg.visuals.nightmode);
    ImGui::Checkbox("no flash", (bool *)&g_cfg.misc.no_flash);
    ImGui::Checkbox("world", (bool *)&g_cfg.visuals.world_color);
    ImGui::Checkbox("sky", (bool *)&g_cfg.visuals.sky_color);
    ImGui::Checkbox("lighting", (bool *)&g_cfg.visuals.light_color);
    ImGui::Checkbox("skybox change", (bool *)&g_cfg.visuals.skybox_change);
    ImGui::Checkbox("bullets tracer", (bool *)&g_cfg.visuals.tracer);
    ImGui::Checkbox("impacts", (bool *)&g_cfg.visuals.impact);
    ImGui::Checkbox("thirdperson", (bool *)&g_cfg.visuals.thirdperson);
    ImGui::Checkbox("spectator list", (bool *)&g_cfg.visuals.spectator_list);
    ImGui::Checkbox("grenade path", (bool *)&g_cfg.misc.grenade_predict);
    ImGui::Checkbox("hideshot", (bool *)&g_cfg.misc.hideshot);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "VIEW / CAMERA");
    ImGui::Separator();
    ImGui::Checkbox("fov changer", (bool *)&g_cfg.visuals.fov_enabled);
    ImGui::SliderFloat("fov", (float *)&g_cfg.visuals.fov_value, 40.0f, 160.0f);
    ImGui::Checkbox("aspect (frustum)", (bool *)&g_cfg.visuals.aspect_ratio);
    ImGui::SliderFloat("aspect", (float *)&g_cfg.visuals.aspect_value, 0.5f, 3.0f);
    ImGui::Checkbox("fog", (bool *)&g_cfg.visuals.fog);
    ImGui::SliderFloat("fog density", (float *)&g_cfg.visuals.fog_density, 0.001f, 0.05f);
    ImGui::Combo("fog mode", (int *)&g_cfg.visuals.fog_mode, g_fog_items, 3);
    ImGui::Checkbox("viewmodel offset", (bool *)&g_cfg.visuals.viewmodel_offset);
    ImGui::Checkbox("no bob", (bool *)&g_cfg.visuals.no_bob);
    ImGui::SliderFloat("vm x", (float *)&g_cfg.visuals.viewmodel_x, -20.0f, 20.0f);
    ImGui::SliderFloat("vm y", (float *)&g_cfg.visuals.viewmodel_y, -20.0f, 20.0f);
    ImGui::SliderFloat("vm z", (float *)&g_cfg.visuals.viewmodel_z, -20.0f, 20.0f);
    ImGui::SliderFloat("vm fov", (float *)&g_cfg.visuals.viewmodel_fov_value, 40.0f, 140.0f);
    ImGui::Checkbox("no smoke", (bool *)&g_cfg.misc.no_smoke);
    ImGui::Checkbox("smoke visuals", (bool *)&g_cfg.visuals.smoke_edit);
    ImGui::SliderFloat("smoke alpha", (float *)&g_cfg.visuals.smoke_alpha, 0.05f, 1.0f);
    ImGui::Checkbox("lightmap night", (bool *)&g_cfg.visuals.lightmap_mode);
    ImGui::Checkbox("anim breakers", (bool *)&g_cfg.visuals.anim_breakers);
    ImGui::Checkbox("killsound", (bool *)&g_cfg.visuals.killsound);
    ImGui::Checkbox("custom scope", (bool *)&g_cfg.visuals.custom_scope);
    ImGui::Checkbox("3d tracer", (bool *)&g_cfg.visuals.tracer3d);
    ImGui::Checkbox("chams rim", (bool *)&g_cfg.visuals.chams_rim);
    ImGui::Checkbox("air run", (bool *)&g_cfg.visuals.anim_air);
    ImGui::Checkbox("ground run", (bool *)&g_cfg.visuals.anim_ground);
    ImGui::SliderFloat("lm red", (float *)&g_cfg.visuals.lightmap_r, 0.05f, 2.0f);
    ImGui::SliderFloat("lm green", (float *)&g_cfg.visuals.lightmap_g, 0.05f, 2.0f);
    ImGui::SliderFloat("lm blue", (float *)&g_cfg.visuals.lightmap_b, 0.05f, 2.0f);
    ImGui::Checkbox("hide hud", (bool *)&g_cfg.misc.hide_hud);
    ImGui::Checkbox("hide weapons", (bool *)&g_cfg.misc.hide_hud_weapons);
    ImGui::Checkbox("hide crosshair", (bool *)&g_cfg.misc.hide_hud_crosshair);
    ImGui::Checkbox("hide health", (bool *)&g_cfg.misc.hide_hud_health);
    ImGui::Checkbox("hide timer", (bool *)&g_cfg.misc.hide_hud_timer);
    ImGui::Checkbox("hide money", (bool *)&g_cfg.misc.hide_hud_money);
    ImGui::Checkbox("no scope", (bool *)&g_cfg.misc.no_scope);
    ImGui::Checkbox("no scope borders", (bool *)&g_cfg.misc.no_scope_borders);
    ImGui::Checkbox("crosshair", (bool *)&g_cfg.misc.crosshair);
    ImGui::Checkbox("hitmarker", (bool *)&g_cfg.visuals.hitmarker);
    ImGui::Checkbox("indicators", (bool *)&g_cfg.visuals.indicators);
    ImGui::Checkbox("hitmarker damage", (bool *)&g_cfg.visuals.hitmarker_damage);
    ImGui::Checkbox("hit sound", (bool *)&g_cfg.visuals.hit_sound);
    ImGui::SliderFloat("hit vol", (float *)&g_cfg.visuals.hit_sound_volume, 0.0f, 1.0f);
    ImGui::Checkbox("radar hack", (bool *)&g_cfg.misc.radar_hack);
    ImGui::Checkbox("radar rotate", (bool *)&g_cfg.misc.radar_rotate);
    ImGui::Checkbox("radar height", (bool *)&g_cfg.misc.radar_height);
}

static void sh_menu_tab_movement(void)
{
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "STRAFE");
    ImGui::Separator();
    ImGui::Checkbox("auto strafe", (bool *)&g_cfg.movement.autostrafe);
    ImGui::Checkbox("quick stop", (bool *)&g_cfg.movement.quick_stop);
    ImGui::Checkbox("bunny hop", (bool *)&g_cfg.movement.bhop);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "EXPLOITS");
    ImGui::Separator();
    ImGui::Checkbox("speed hack", (bool *)&g_cfg.movement.speedhack);
    ImGui::SliderFloat("speed mult", (float *)&g_cfg.movement.speedhack_value, 0.5f, 3.0f);
    ImGui::Checkbox("ground strafe", (bool *)&g_cfg.movement.ground_strafe);
    ImGui::Checkbox("jumpbug", (bool *)&g_cfg.movement.jumpbug);
    ImGui::Checkbox("texturebug", (bool *)&g_cfg.movement.texturebug);
    ImGui::SliderFloat("gs max", (float *)&g_cfg.movement.ground_strafe_max, 200.0f, 2000.0f);
    ImGui::Checkbox("strafe hack", (bool *)&g_cfg.movement.strafe_hack);
    ImGui::SliderFloat("max speed", (float *)&g_cfg.movement.strafe_hack_max, 100.0f, 3000.0f);
    ImGui::Checkbox("air stuck", (bool *)&g_cfg.movement.air_stuck);
    ImGui::SliderFloat("stuck force", (float *)&g_cfg.movement.air_stuck_intensity, 0.0f, 3.0f);
    ImGui::Checkbox("tick control", (bool *)&g_cfg.movement.tick_control);
    ImGui::SliderFloat("tick ms", (float *)&g_cfg.movement.tick_ms, 1.0f, 50.0f);
    ImGui::Checkbox("auto jump", (bool *)&g_cfg.movement.auto_jump);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "GRENADE");
    ImGui::Separator();
    ImGui::Checkbox("super toss", (bool *)&g_cfg.movement.super_toss);
    ImGui::SliderFloat("toss power", (float *)&g_cfg.movement.super_toss_power, 0.0f, 100.0f);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "INFO");
    ImGui::Separator();
    /* было отладочным счётчиком (g_movement_shots_dbg), не настройкой:
       показываем как индикатор, менять нечего */
    ImGui::TextDisabled("shots fired: %d", g_shots_fired);
    ImGui::Spacing();
}

static void sh_menu_tab_settings(void)
{
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "ESP");
    ImGui::Separator();
    ImGui::Checkbox("team check", (bool *)&g_cfg.visuals.team_check);
    ImGui::Checkbox("show local", (bool *)&g_cfg.esp.show_local);
    ImGui::SliderFloat("text scale", (float *)&g_cfg.esp.text_scale, 0.5f, 2.0f);
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "STATUS");
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.47f, 0.51f, 0.57f, 1.0f), "KEYS");
    ImGui::Separator();
}

static void sh_menu_toggle(void)
{
    static i32 prev;
    i32 now = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;

    if (now && !prev)
    {
        g_menu_visible = !g_menu_visible;
        g_cfg.menu_open = (qbool)g_menu_visible;

        /* мышь забираем/возвращаем вместе с видимостью */
        sh_menu_release_mouse((qbool)g_menu_visible);

        if (!g_menu_visible)
            sh_config_save();
    }
    prev = now;

    {
        static i32 prev_esc;
        i32 esc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

        if (esc && !prev_esc && g_menu_visible)
        {
            g_menu_visible = 0;
            g_cfg.menu_open = (qbool)0;
            sh_menu_release_mouse(0);
            sh_config_save();
        }
        prev_esc = esc;
    }
}

/* watermark-заглушка: вызывается из HUD_Redraw, пока SDK не готов.
   С ImGui рисуем отдельным проходом, но он ещё не инициализирован,
   поэтому просто ничего не делаем - сам факт вызова важен для
   совместимости. */
void sh_menu_watermark_only(void)
{
}

qbool sh_menu_visible_now(void)
{
    return g_menu_visible;
}

void sh_menu_draw(void)
{
    sh_menu_toggle();

    if (!g_menu_visible)
        return;

    /* РАЗМЕР И МАСШТАБ ОТ РАЗРЕШЕНИЯ.

       На 1920x1080 окно 640x470 - это треть экрана, а шрифт 14px
       при таком размере читается как точки. Отсюда "огромная,
       и нихуя не видно": окно большое, содержимое мелкое.

       Считаем масштаб от высоты экрана, как принято в читах:
       база 480p (в GoldSrc это исходное разрешение интерфейса).
       Множитель ограничиваем 0.85..1.6, чтобы на 4K окно не
       разрослось во весь экран. */
    {
        f32 sh = (f32)sh_screen_height();

        g_ui_scale = sh / 480.0f;

        if (g_ui_scale < 0.85f)
            g_ui_scale = 0.85f;

        if (g_ui_scale > 1.6f)
            g_ui_scale = 1.6f;
    }

    /* РАНЬШЕ ТУТ БЫЛ ВЫЗОВ sh_imgui_scale_font(g_ui_scale) - И ЭТО
       КРАШИЛО ИГРУ.

       Причина: ImGui::NewFrame() уже выполнен в sh_imgui_begin(),
       виджеты уже начали использовать шрифты, а scale_font делает
       Fonts->Clear() + пересборку атласа + удаление GL-текстуры.
       То есть мы уничтожали шрифт ПОСРЕДИ кадра, в котором он
       используется: следующая буква читала освобождённую память.

       Пересборка перенесена в sh_imgui_begin(), ДО NewFrame -
       там её единственное безопасное место. */

    ImGui::SetNextWindowSize(ImVec2(760.0f * g_ui_scale,
                                    520.0f * g_ui_scale),
                             ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(560.0f * g_ui_scale, 360.0f * g_ui_scale),
        ImVec2(1100.0f * g_ui_scale, 900.0f * g_ui_scale));

    ImGui::Begin("skeethook##main", 0,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(7.0f * g_ui_scale, 4.0f * g_ui_scale));

    ImGui::BeginChild("##tabs", ImVec2(132.0f * g_ui_scale, 0.0f), 1);

    for (i32 i = 0; i < 6; ++i)
    {
        if (ImGui::Selectable(g_tab_names[i], g_cur_tab == i, 0,
                              ImVec2(0.0f, 26.0f * g_ui_scale)))
            g_cur_tab = i;
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##content", ImVec2(0.0f, 0.0f), 1);

    /* виджеты чуть просторнее: при мелком шрифте плотная сетка
       сливается в кашу */
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(7.0f * g_ui_scale, 4.0f * g_ui_scale));

    switch (g_cur_tab)
    {
    case 0: sh_menu_tab_rage(); break;
    case 1: sh_menu_tab_antiaim(); break;
    case 2: sh_menu_tab_visuals(); break;
    case 3: sh_menu_tab_misc(); break;
    case 4: sh_menu_tab_movement(); break;
    default: sh_menu_tab_settings(); break;
    }

    ImGui::PopStyleVar();

    ImGui::EndChild();

    ImGui::PopStyleVar();

    ImGui::End();
}
