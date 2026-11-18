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

static i32 g_tab;
static i32 g_visible;
static i32 g_selected;

static const char *g_tabs[] = {
    "rage", "antiaim", "visuals", "misc", "movement", "settings"
};

static void sh_menu_release_mouse(qbool release);
static HWND sh_game_window(void);

static void sh_menu_toggle(void)
{
    static i32 prev;
    i32 now = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;

    if (now && !prev)
    {
        g_visible = !g_visible;
        g_cfg.menu_open = (qbool)g_visible;

        /* забираем/возвращаем мышь вместе с видимостью меню */
        sh_menu_release_mouse((qbool)g_visible);

        if (!g_visible)
            sh_config_save();
    }
    prev = now;

    /* ESC закрывает ТОЛЬКО наше меню, если оно открыто.
       Раньше ESC не обрабатывался вообще, и игра закрывала своё
       меню - а наше оставалось висеть с "потерянным" курсором.
       Теперь ESC = быстрый выход, но не в том же кадре, что INSERT. */
    static i32 prev_esc;
    i32 esc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

    if (esc && !prev_esc && g_visible)
    {
        g_visible = 0;
        g_cfg.menu_open = (qbool)0;
        sh_menu_release_mouse(0);
        sh_config_save();
    }
    prev_esc = esc;
}

static POINT g_mouse;
static qbool g_click;
static qbool g_click_edge;   /* фронт ЛКМ, считается раз за кадр */

/* сколько раз мы дёрнули ShowCursor(TRUE) — чтобы скомпенсировать
   на закрытии: он ведёт счётчик, а не флаг */
static i32 g_cursor_balance;

/* видимая область содержимого - нужна и в хит-тестах, и в отрисовке */
static f32 g_scroll_rect_x;
static f32 g_scroll_rect_y;
static f32 g_scroll_rect_w;
static f32 g_scroll_rect_h;
static f32 g_scroll_content_h;

/* ОКНО ИГРЫ.

   WindowFromPoint() тут не годится: он возвращает САМОЕ ГЛУБОКОЕ окно
   под курсором - им вполне может оказаться дочернее окно или чужое
   окно, оказавшееся сверху. Тогда ScreenToClient считает координаты
   относительно него, и попадания по пунктам меню уезжают.

   Поэтому окно игры находим один раз по классу: GoldSrc создаёт окно
   класса "Valve001". _windowed_mouse=0 (инжект в полноэкранном режиме)
   тоже ломает координаты, но его мы исправляем ниже. */
static HWND sh_game_window(void)
{
    static HWND cached;

    if (cached && IsWindow(cached))
        return cached;

    cached = FindWindowA("Valve001", 0);

    if (!cached)
        cached = GetActiveWindow();

    if (!cached)
        cached = GetForegroundWindow();

    return cached;
}

/* ОСВОБОЖДЕНИЕ МЫШИ.

   В GoldSrc мышь захвачена движком (SetCapture + ClipCursor +
   скрытый курсор), поэтому при открытом меню курсора не видно и
   двигать нечем - ровно то, на что жаловались.

   IN_DeactivateMouse() определена в client.dll и снимает захват;
   IN_ActivateMouse() возвращает обратно. Вызываем их при
   переключении видимости меню, а не каждый кадр - это дешёво
   и не конфликтует с движком. */
typedef void (*in_mouse_fn)(void);

static in_mouse_fn sh_in_mouse(const char *name)
{
    static HMODULE client;
    static in_mouse_fn on;
    static in_mouse_fn off;

    if (!client)
        client = GetModuleHandleA("client.dll");

    if (!client)
        return 0;

    if (name[3] == 'D')   /* IN_DeactivateMouse */
    {
        if (!off)
            off = (in_mouse_fn)(void *)GetProcAddress(client, name);
        return off;
    }

    if (!on)
        on = (in_mouse_fn)(void *)GetProcAddress(client, name);

    return on;
}

static void sh_menu_release_mouse(qbool release)
{
    if (release)
    {
        /* Экспорты IN_* из client.dll трогают внутреннее состояние
           движка (в частности mouse_x/mouse_y), поэтому вызываем их
           только как дополнение к Win32-снятию захвата, и только если
           они реально нашлись. Если нет - обходимся Win32: ClipCursor(0)
           и ReleaseCapture достаточно, чтобы курсор освободился. */
        in_mouse_fn deact = sh_in_mouse("IN_DeactivateMouse");

        if (deact)
            deact();

        ClipCursor(0);
        ReleaseCapture();

        /* ShowCursor — счётчик. Доводим его до 0 (видимо), но
           запоминаем, сколько раз дёрнули, чтобы на закрытии
           скомпенсировать ровно столько же. Иначе за сессию
           счётчик уползёт и курсор "залипнет" видимым. */
        {
            i32 guard = 0;

            while (ShowCursor(TRUE) < 0 && ++guard < 32)
                ;

            g_cursor_balance += guard;
        }

        {
            /* курсор в центр, иначе он прилипает к краю экрана */
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
    else
    {
        in_mouse_fn act = sh_in_mouse("IN_ActivateMouse");

        if (act)
            act();

        ClipCursor(0);

        /* возвращаем счётчик туда, где взяли */
        while (g_cursor_balance > 0)
        {
            ShowCursor(FALSE);
            --g_cursor_balance;
        }

        ShowCursor(FALSE);

        {
            /* курсор обратно в центр окна, чтобы камера не дёрнулась:
               GoldSrc считает дельту мыши от предыдущей позиции, и
               если курсор остался в углу меню, камеру резко дёрнет. */
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

/* Покадровая коррекция: если меню открыто, мышь обязана быть свободна.
   Движок (IN_ActivateMouse из V_CalcRefdef / при возврате фокуса) может
   повторно захватить её в любой момент, поэтому недостаточно освободить
   один раз при открытии - проверяем каждый кадр. */
static void sh_menu_cursor_tick(void)
{
    if (!g_visible)
        return;

    /* снимаем клип, если движок вернул его обратно */
    if (GetCapture() == sh_game_window())
        ReleaseCapture();

    /* ВАЖНО: ShowCursor здесь НЕ зовём.

       ShowCursor ведёт СЧЁТЧИК, а не флаг. Вызов в каждом кадре
       увеличивал бы его на единицу за кадр — за секунду набегает
       ~60, и курсор потом не спрятать никаким одним
       ShowCursor(FALSE). Счётчик правим РОВНО ДВА РАЗА: в
       sh_menu_release_mouse() при открытии и закрытии.

       Покадрово нужно только SetCursor: GoldSrc перерисовывает
       курсор сам (WM_SETCURSOR), и без этого он снова исчезает. */
    SetCursor(LoadCursorA(0, IDC_ARROW));
}

/* Приведение координат курсора к системе координат рендера.
   Меню рисуется в координатах движка (pfnGetScreenInfo), а
   ScreenToClient даёт координаты клиентской области окна. В
   полноэкранном режиме они обычно совпадают, но при windowed /
   другом разрешении - нет, и клики уезжают. */
static void sh_menu_to_render_space(POINT *p)
{
    HWND wnd = sh_game_window();
    RECT rc;

    if (!wnd || !GetClientRect(wnd, &rc))
        return;

    i32 cw = rc.right - rc.left;
    i32 ch = rc.bottom - rc.top;
    i32 rw = sh_screen_width();
    i32 rh = sh_screen_height();

    if (cw <= 0 || ch <= 0 || rw <= 0 || rh <= 0)
        return;

    p->x = (LONG)((f32)p->x * (f32)rw / (f32)cw);
    p->y = (LONG)((f32)p->y * (f32)rh / (f32)ch);
}

static void sh_menu_input_begin(void)
{
    GetCursorPos(&g_mouse);

    HWND wnd = sh_game_window();

    if (wnd)
        ScreenToClient(wnd, &g_mouse);

    sh_menu_to_render_space(&g_mouse);

    {
        static i32 prev_click;

        i32 now = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0;

        g_click = (qbool)now;
        g_click_edge = (qbool)(now && !prev_click);

        prev_click = now;
    }
}

/* Отсечение по области прокрутки.

   Виджеты внутри вкладки рисуются в сдвинутых координатах и могут
   оказаться ЗА пределами видимой области (выше или ниже окна).
   Без этой проверки они продолжают ловить клики, даже будучи
   невидимыми: например, пункт, уехавший под нижнюю границу,
   срабатывает при клике по тому месту, где его уже нет.

   g_scroll_rect_* заполняются в sh_menu_draw() до отрисовки
   содержимого, поэтому к моменту хит-тестов они валидны. */
static qbool sh_mouse_in(f32 x, f32 y, f32 w, f32 h)
{
    f32 mx = (f32)g_mouse.x;
    f32 my = (f32)g_mouse.y;

    if (mx < x || mx > x + w || my < y || my > y + h)
        return 0;

    /* горизонталь ограничиваем всегда, вертикаль - только когда
       область прокрутки реально задана */
    if (g_scroll_rect_w > 0.0f && g_scroll_rect_h > 0.0f)
    {
        if (mx < g_scroll_rect_x ||
            mx > g_scroll_rect_x + g_scroll_rect_w)
            return 0;

        if (my < g_scroll_rect_y ||
            my > g_scroll_rect_y + g_scroll_rect_h)
            return 0;
    }

    return 1;
}

/* ВАЖНО: это ФРОНТ нажатия, а не уровень.

   Раньше возвращался g_click, то есть "кнопка сейчас зажата". Зажал
   ЛКМ на чекбоксе - значение перещёлкивалось КАЖДЫЙ кадр, пока
   кнопка не отпущена. Выглядело как "ничего нельзя выставить":
   галочка ставится и тут же снимается следующим кадром (и так 60 раз
   в секунду), слайдер уезжает в край.

   Теперь возвращаем true ровно один кадр - в момент перехода
   "было отпущено -> стало зажато". */
/* флаг фронта считается ОДИН раз за кадр в sh_menu_input_begin(),
   потому что sh_mouse_clicked() зовётся из десятков виджетов:
   если считать фронт внутри неё, второй вызов в том же кадре
   вернул бы false и клик "съедался" бы первым же виджетом */
static qbool sh_mouse_clicked(void)
{
    return g_click_edge;
}

static void sh_menu_checkbox(f32 x, f32 y, const char *label, qbool *value)
{
    f32 s = sh_scale();
    f32 box = 11.0f * s;
    f32 tw = sh_text_width(label);
    f32 hitw = box + 8.0f * s + tw;
    qbool hover = sh_mouse_in(x, y - 3.0f * s, hitw, box + 5.0f * s);

    if (hover)
        sh_draw_rect(x - 3.0f * s, y - 3.0f * s, hitw + 6.0f * s,
                     box + 6.0f * s, (col4_t)SH_COL_RGBA(26, 27, 31, 150));

    sh_draw_rect(x, y, box, box, (col4_t)SH_COL_RGBA(16, 17, 20, 255));
    sh_draw_rect_outline(x, y, box, box, 1.0f,
                         (col4_t)SH_COL_RGBA(*value ? 190 : 62,
                                             *value ? 235 : 62,
                                             *value ? 60 : 70, 255));

    if (*value)
    {
        /* галочка двумя штрихами - дешевле буквы и читается лучше */
        sh_draw_rect(x + 2.5f * s, y + 5.0f * s, 3.0f * s, 2.0f * s,
                     (col4_t)SH_COL_RGBA(190, 235, 60, 255));
        sh_draw_rect(x + 4.5f * s, y + 3.5f * s, 2.0f * s, 2.0f * s,
                     (col4_t)SH_COL_RGBA(190, 235, 60, 255));
        sh_draw_rect(x + 5.5f * s, y + 2.5f * s, 3.0f * s, 5.0f * s,
                     (col4_t)SH_COL_RGBA(190, 235, 60, 255));
    }

    sh_text(x + box + 8.0f * s, y - 1.0f * s,
            (col4_t)SH_COL_RGBA(hover ? 235 : 205, hover ? 235 : 205,
                                hover ? 240 : 212, 255), "%s", label);

    if (hover && sh_mouse_clicked())
        *value = !*value;
}

static void sh_menu_slider(f32 x, f32 y, f32 w, const char *label, f32 *value,
                           f32 lo, f32 hi, const char *suffix)
{
    f32 s = sh_scale();
    f32 tracky = y + 13.0f * s;

    sh_text(x, y, (col4_t)SH_COL_RGBA(215, 215, 215, 255), "%s", label);

    char val[32];
    i32 iv = (i32)*value;
    snprintf(val, sizeof(val), "%d%s", iv, suffix ? suffix : "");
    f32 vw = sh_text_width(val);
    sh_text(x + w - vw, y, (col4_t)SH_COL_RGBA(190, 235, 60, 255), "%s", val);

    sh_draw_rect(x, tracky, w, 3.0f * s, (col4_t)SH_COL_RGBA(38, 38, 38, 255));

    f32 frac = (*value - lo) / (hi - lo);
    sh_draw_rect(x, tracky, w * frac, 3.0f * s, (col4_t)SH_COL_RGBA(190, 235, 60, 255));

    /* Слайдер: тянется, пока ЛКМ зажата, а не только по клику.

       Для перетаскивания нужен УРОВЕНЬ кнопки (g_click), а не фронт:
       фронт сработал бы один раз и значение встало бы намертво.
       Начало перетаскивания ловим по фронту. */
    {
        static const void *drag_owner;
        const void *self = (const void *)value;

        qbool over = sh_mouse_in(x, tracky - 4.0f * s, w, 11.0f * s);

        if (over && sh_mouse_clicked())
            drag_owner = self;

        if (drag_owner == self && g_click)
        {
            f32 t = ((f32)g_mouse.x - x) / (w > 1.0f ? w : 1.0f);

            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            *value = lo + t * (hi - lo);
        }

        if (!g_click)
            drag_owner = 0;
    }
}

static void sh_menu_combo(f32 x, f32 y, f32 w, const char *label, i32 *value,
                         const char **items, i32 count)
{
    f32 s = sh_scale();
    f32 h = 16.0f * s;

    static i32 open_combo = -1;
    static const void *owner;

    if (count <= 0)
        return;

    if (*value < 0)
        *value = 0;
    if (*value >= count)
        *value = count - 1;

    col4_t bg = { 26, 26, 26, 255 };
    col4_t bd = { 44, 44, 44, 255 };
    col4_t tx = { 200, 200, 200, 255 };

    sh_draw_rect(x, y, w, h, bg);
    sh_draw_rect_outline(x, y, w, h, 1.0f, bd);

    sh_text(x + 5.0f * s, y + 3.0f * s, tx, "%s", label);
    sh_text(x + w - 10.0f * s, y + 3.0f * s, tx, "v");

    qbool hovered = sh_mouse_in(x, y, w, h);

    if (hovered && sh_mouse_clicked())
    {
        if (open_combo == *value && owner == (const void *)value)
        {
            open_combo = -1;
            owner = 0;
        }
        else
        {
            open_combo = *value;
            owner = (const void *)value;
        }
    }

    if (owner != (const void *)value)
        return;

    for (i32 i = 0; i < count; ++i)
    {
        f32 iy = y + h + (f32)i * h;

        sh_draw_rect(x, iy, w, h, bg);
        sh_draw_rect_outline(x, iy, w, h, 1.0f, bd);

        col4_t it = (i == *value) ? (col4_t){ 120, 180, 255, 255 } : tx;
        sh_text(x + 5.0f * s, iy + 3.0f * s, it, "%s", items[i]);

        if (sh_mouse_in(x, iy, w, h) && sh_mouse_clicked())
        {
            *value = i;
            open_combo = -1;
            owner = 0;
        }
    }
}

static void sh_menu_section(const char *title, f32 x, f32 y)
{
    f32 s = sh_scale();

    sh_text(x, y, (col4_t)SH_COL_RGBA(120, 130, 145, 255), "%s", title);
    sh_draw_rect(x, y + 13.0f * s, 240.0f * s, 1.0f,
                 (col4_t)SH_COL_RGBA(36, 38, 44, 255));
}

static const char *g_hitbox_items[] = {
    "head", "neck", "chest", "stomach", "pelvis", "legs"
};

static const char *g_priority_items[] = {
    "distance", "fov", "damage", "low health"
};

static void sh_menu_draw_rage(f32 x, f32 y)
{
    f32 s = sh_scale();
    f32 row = 20.0f * s;
    f32 colw = 210.0f * s;

    sh_menu_section("aimbot", x, y);

    f32 cy = y + 20.0f * s;
    sh_menu_checkbox(x, cy, "enabled", &g_cfg.aimbot.enabled);
    sh_menu_checkbox(x + colw, cy, "silent", &g_cfg.aimbot.silent);
    cy += row;

    sh_menu_checkbox(x, cy, "auto fire", &g_cfg.aimbot.auto_fire);
    sh_menu_checkbox(x + colw, cy, "auto wall", &g_cfg.aimbot.auto_wall);
    cy += row;

    sh_menu_checkbox(x, cy, "penetration", &g_cfg.aimbot.penetration);
    sh_menu_checkbox(x + colw, cy, "double tap", &g_cfg.aimbot.double_tap);
    cy += row;

    sh_menu_checkbox(x, cy, "auto stop", &g_cfg.aimbot.auto_stop);
    sh_menu_checkbox(x + colw, cy, "auto stop duck", &g_cfg.aimbot.auto_stop_duck);
    cy += row;

    sh_menu_checkbox(x, cy, "auto scope", &g_cfg.aimbot.auto_scope);
    sh_menu_checkbox(x + colw, cy, "visible check", &g_cfg.aimbot.visible_check);
    cy += row;

    sh_menu_checkbox(x, cy, "smoke check", &g_cfg.aimbot.smoke_check);
    cy += row + 4.0f * s;

    sh_menu_section("target", x, cy);
    cy += 20.0f * s;

    static const char *g_mp_items[] = { "off", "center", "corners" };

    sh_menu_combo(x, cy, colw, "hitbox", &g_cfg.aimbot.hitbox, g_hitbox_items, 6);
    sh_menu_combo(x + colw, cy, colw, "multipoint", &g_cfg.aimbot.multipoint, g_mp_items, 3);
    cy += row;

    static const char *g_bt_items[] = { "off", "1 frame", "2 frames", "3 frames", "4 frames", "6 frames" };

    sh_menu_combo(x, cy, colw, "backtrack", &g_cfg.aimbot.backtrack, g_bt_items, 6);
    cy += row;

    sh_menu_combo(x, cy, colw, "priority", &g_cfg.aimbot.priority, g_priority_items, 4);
    cy += row + 4.0f * s;

    sh_menu_section("tuning", x, cy);
    cy += 20.0f * s;

    sh_menu_slider(x, cy, colw, "fov", &g_cfg.aimbot.fov, 1.0f, 180.0f, "");
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "smooth", &g_cfg.aimbot.smooth, 1.0f, 20.0f, "");
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "min damage", &g_cfg.aimbot.min_damage, 1.0f, 100.0f, "");
}

static const char *g_pitch_items[] = {
    "none", "down", "up", "fake down", "fake up", "spin", "static"
};

static const char *g_yaw_items[] = {
    "none", "backwards", "static left", "static right", "left", "right",
    "spin", "static", "random", "jitter", "fake sideways", "fake edge",
    "at target", "at target jitter"
};

static void sh_menu_draw_antiaim(f32 x, f32 y)
{
    f32 s = sh_scale();
    f32 row = 20.0f * s;
    f32 colw = 210.0f * s;

    sh_menu_section("antiaim", x, y);

    f32 cy = y + 20.0f * s;
    sh_menu_checkbox(x, cy, "enabled", &g_cfg.antiaim.enabled);
    cy += row;

    sh_menu_combo(x, cy, colw, "pitch", &g_cfg.antiaim.pitch_mode,
                  g_pitch_items, 7);
    cy += row;

    sh_menu_combo(x, cy, colw, "yaw", &g_cfg.antiaim.yaw_mode,
                  g_yaw_items, 14);
    cy += row;

    sh_menu_slider(x, cy, colw, "roll", &g_cfg.antiaim.roll, -50.0f, 50.0f, "%.0f");
    cy += row + 8.0f * s;

    sh_menu_section("fake lag", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "enabled", &g_cfg.antiaim.fake_lag);
    cy += row;

    static f32 g_fl_dbg;
    g_fl_dbg = (f32)g_cfg.antiaim.fake_lag_ticks;

    sh_menu_slider(x, cy, colw, "ticks", &g_fl_dbg, 0.0f, 22.0f, "%.0f");
    cy += row;

    if (g_fl_dbg >= 0.0f)
        g_cfg.antiaim.fake_lag_ticks = (i32)(g_fl_dbg + 0.5f);

    static f32 g_spin_dbg;
    g_spin_dbg = g_cfg.antiaim.spin_speed;
    sh_menu_slider(x, cy, colw, "spin speed", &g_spin_dbg, 0.0f, 10.0f, "%.1f");
    g_cfg.antiaim.spin_speed = g_spin_dbg;
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "yaw base", &g_cfg.antiaim.yaw_base, -180.0f, 180.0f, "%.0f");
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "jitter", &g_cfg.antiaim.yaw_jitter, 0.0f, 90.0f, "%.0f");
    cy += row + 8.0f * s;

    sh_menu_section("desync", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "desync", &g_cfg.antiaim.desync);
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "amount", &g_cfg.antiaim.desync_amount, 0.0f, 90.0f, "%.0f");

    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "ticks", &g_cfg.antiaim.desync_ticks, 1.0f, 8.0f, "%.0f");
    cy += row + 8.0f * s;

    sh_menu_section("extras", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "fake duck", &g_cfg.antiaim.fake_duck);
    sh_menu_checkbox(x + colw, cy, "micro move", &g_cfg.antiaim.micro_move);
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "micro speed", &g_cfg.antiaim.micro_move_speed, 0.5f, 5.0f, "%.1f");
}

static void sh_menu_draw_visuals(f32 x, f32 y)
{
    f32 s = sh_scale();
    f32 row = 20.0f * s;
    f32 colw = 210.0f * s;

    sh_menu_section("esp", x, y);

    f32 cy = y + 20.0f * s;
    sh_menu_checkbox(x, cy, "enabled", &g_cfg.visuals.enabled);
    sh_menu_checkbox(x + colw, cy, "box", &g_cfg.visuals.box);
    cy += row;

    sh_menu_checkbox(x, cy, "name", &g_cfg.visuals.name);
    sh_menu_checkbox(x + colw, cy, "health", &g_cfg.visuals.health_bar);
    cy += row;

    sh_menu_checkbox(x, cy, "weapon", &g_cfg.visuals.weapon);
    sh_menu_checkbox(x + colw, cy, "weapon icon", &g_cfg.visuals.weapon_icon);
    cy += row;

    sh_menu_checkbox(x, cy, "armor", &g_cfg.visuals.armor);
    sh_menu_checkbox(x + colw, cy, "distance", &g_cfg.visuals.distance);
    cy += row;

    sh_menu_checkbox(x, cy, "skeleton", &g_cfg.visuals.skeleton);
    sh_menu_checkbox(x + colw, cy, "head dot", &g_cfg.visuals.head_dot);
    cy += row;

    sh_menu_checkbox(x, cy, "snaplines", &g_cfg.visuals.snaplines);
    sh_menu_checkbox(x + colw, cy, "show local", &g_cfg.esp.show_local);
    sh_menu_checkbox(x + colw, cy, "visible only", &g_cfg.visuals.visible_only);
    cy += row;
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "max distance", &g_cfg.visuals.max_distance,
                   200.0f, 8192.0f, "");
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "text scale", &g_cfg.esp.text_scale, 0.5f, 2.5f, "x");
    cy += row + 6.0f * s;

    static const char *font_items[] = { "default", "small", "tiny" };

    sh_menu_combo(x, cy, colw, "font", &g_cfg.esp.font, font_items, 3);
    cy += row + 6.0f * s;

    sh_menu_section("models", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "chams", &g_cfg.visuals.chams);
    sh_menu_checkbox(x + colw, cy, "glow", &g_cfg.visuals.glow);
    cy += row;

    sh_menu_checkbox(x, cy, "team colors", &g_cfg.visuals.chams_team_color);
    sh_menu_checkbox(x + colw, cy, "team check", &g_cfg.visuals.team_check);
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "glow amount", &g_cfg.visuals.glow_amount,
                   20.0f, 255.0f, "");

    sh_menu_checkbox(x, cy, "xqz chams", &g_cfg.visuals.chams_xqz);

    cy += row + 6.0f * s;

    sh_menu_section("model changer", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "sound esp", &g_cfg.visuals.sound_esp);
    sh_menu_checkbox(x + colw, cy, "player model", &g_cfg.visuals.model_changer);

    cy += row;

    sh_menu_checkbox(x, cy, "fake markers", &g_cfg.visuals.draw_fakes);
    sh_menu_checkbox(x + colw, cy, "entity esp", &g_cfg.visuals.entity_esp);
    sh_menu_checkbox(x + colw, cy, "wallbang info", &g_cfg.visuals.penetration_info);

    cy += row;

    sh_menu_checkbox(x, cy, "position history", &g_cfg.visuals.draw_history);
    sh_menu_checkbox(x + colw, cy, "damage info", &g_cfg.visuals.damage_info);
    cy += row;

    sh_menu_checkbox(x, cy, "damage hitbox", &g_cfg.visuals.damage_hitbox);
    sh_menu_checkbox(x + colw, cy, "hitboxes 3d", &g_cfg.visuals.hitboxes);
    cy += row;

    static const char *g_espc_items[] = { "visible", "health", "team" };

    sh_menu_combo(x, cy, colw, "esp color", &g_cfg.visuals.esp_color_mode, g_espc_items, 3);
    cy += row;

    sh_menu_checkbox(x, cy, "grenade radius", &g_cfg.misc.grenade_radius);
    cy += row;

    sh_menu_checkbox(x, cy, "hitbox visibility", &g_cfg.visuals.hitbox_vis_check);
    sh_menu_checkbox(x + colw, cy, "hitbox scanner", &g_cfg.visuals.hitbox_scan);
    cy += row;

    {
        static const char *items[16];
        static char names[16][32];

        i32 n = sh_model_changer_count();

        if (n > 16)
            n = 16;

        for (i32 i = 0; i < n; ++i)
        {
            const char *src = sh_model_changer_name(i);
            i32 k = 0;

            while (src[k] && k < 31)
            {
                names[i][k] = src[k];
                ++k;
            }

            names[i][k] = 0;
            items[i] = names[i];
        }

        sh_menu_combo(x, cy, colw, "model", &g_cfg.visuals.model_index,
                      items, n);
    }
}

static void sh_menu_draw_misc(f32 x, f32 y)
{
    f32 s = sh_scale();
    f32 row = 20.0f * s;
    f32 colw = 210.0f * s;

    sh_menu_section("movement", x, y);

    f32 cy = y + 20.0f * s;
    sh_menu_checkbox(x, cy, "edge jump", &g_cfg.misc.edge_jump);
    sh_menu_checkbox(x + colw, cy, "fast ladder", &g_cfg.misc.fast_ladder);
    cy += row;

    sh_menu_checkbox(x, cy, "fall damage warn", &g_cfg.misc.no_fall_damage);
    sh_menu_checkbox(x + colw, cy, "auto pistol", &g_cfg.misc.auto_pistol);
    cy += row + 8.0f * s;

    sh_menu_section("combat", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "no recoil", &g_cfg.misc.no_recoil);
    sh_menu_checkbox(x + colw, cy, "no recoil fx", &g_cfg.misc.no_recoil_effect);
    cy += row;
    sh_menu_checkbox(x + colw, cy, "no spread", &g_cfg.misc.no_spread);
    cy += row;

    sh_menu_checkbox(x, cy, "fast reload", &g_cfg.misc.fast_reload);
    sh_menu_checkbox(x + colw, cy, "knife bot", &g_cfg.misc.knife_bot);
    cy += row + 8.0f * s;

    sh_menu_section("visual", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "nightmode", &g_cfg.visuals.nightmode);
    sh_menu_checkbox(x + colw, cy, "no flash", &g_cfg.misc.no_flash);
    cy += row;

    if (g_cfg.visuals.nightmode)
    {
        sh_menu_checkbox(x, cy, "world", &g_cfg.visuals.world_color);
        sh_menu_checkbox(x + 100.0f * s, cy, "sky", &g_cfg.visuals.sky_color);
        sh_menu_checkbox(x + 180.0f * s, cy, "lighting", &g_cfg.visuals.light_color);
        cy += row;
    }

    sh_menu_checkbox(x, cy, "skybox change", &g_cfg.visuals.skybox_change);
    cy += row;

    static const char *g_sky_items[sh_skybox_max];
    static qbool sky_init;

    if (!sky_init)
    {
        sh_skybox_scan();

        i32 cnt = sh_skybox_custom_count();

        if (cnt <= 0)
            cnt = sh_skybox_count;

        if (cnt > sh_skybox_max)
            cnt = sh_skybox_max;

        for (i32 i = 0; i < cnt; ++i)
            g_sky_items[i] = sh_skybox_name(i);

        sky_init = 1;
    }

    i32 sky_cnt = sh_skybox_custom_count();

    if (sky_cnt <= 0)
        sky_cnt = sh_skybox_count;

    if (sky_cnt > sh_skybox_max)
        sky_cnt = sh_skybox_max;

    static i32 sky_dbg;
    sky_dbg = g_cfg.visuals.skybox_index;

    if (sky_dbg >= sky_cnt)
        sky_dbg = 0;

    sh_menu_combo(x, cy, colw, "skybox", &sky_dbg, g_sky_items, sky_cnt);
    g_cfg.visuals.skybox_index = sky_dbg;
    cy += row + 6.0f * s;

    sh_menu_checkbox(x, cy, "bullets tracer", &g_cfg.visuals.tracer);
    sh_menu_checkbox(x + colw, cy, "impacts", &g_cfg.visuals.impact);
    cy += row;

    sh_menu_checkbox(x, cy, "thirdperson", &g_cfg.visuals.thirdperson);
    sh_menu_checkbox(x + colw, cy, "spectator list", &g_cfg.visuals.spectator_list);
    cy += row;

    sh_menu_checkbox(x, cy, "grenade path", &g_cfg.misc.grenade_predict);
    sh_menu_checkbox(x + colw, cy, "hideshot", &g_cfg.misc.hideshot);
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "thirdperson dist", &g_cfg.visuals.thirdperson_dist,
                   32.0f, 400.0f, "%.0f");
    cy += row;

    sh_menu_section("view / camera", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "fov changer", &g_cfg.visuals.fov_enabled);
    sh_menu_slider(x + colw, cy, colw, "fov", &g_cfg.visuals.fov_value, 40.0f, 160.0f, "%.0f");
    cy += row;

    sh_menu_checkbox(x, cy, "aspect (frustum)", &g_cfg.visuals.aspect_ratio);
    sh_menu_slider(x + colw, cy, colw, "aspect", &g_cfg.visuals.aspect_value, 0.5f, 3.0f, "%.2f");
    cy += row;

    sh_menu_checkbox(x, cy, "fog", &g_cfg.visuals.fog);
    cy += row;

    sh_menu_slider(x, cy, colw, "fog density", &g_cfg.visuals.fog_density, 0.001f, 0.05f, "%.3f");

    static const char *g_fog_items[] = { "normal", "thick", "far" };

    if (g_cfg.visuals.fog_mode < 0 || g_cfg.visuals.fog_mode > 2)
        g_cfg.visuals.fog_mode = 0;

    sh_menu_combo(x + colw, cy, colw, "fog mode", &g_cfg.visuals.fog_mode, g_fog_items, 3);
    cy += row;
    sh_menu_checkbox(x + colw, cy, "viewmodel offset", &g_cfg.visuals.viewmodel_offset);
    sh_menu_checkbox(x + colw, cy, "no bob", &g_cfg.visuals.no_bob);
    cy += row;

    sh_menu_slider(x, cy, colw, "vm x", &g_cfg.visuals.viewmodel_x, -20.0f, 20.0f, "%.0f");
    sh_menu_slider(x + colw, cy, colw, "vm y", &g_cfg.visuals.viewmodel_y, -20.0f, 20.0f, "%.0f");
    cy += row;

    sh_menu_slider(x, cy, colw, "vm z", &g_cfg.visuals.viewmodel_z, -20.0f, 20.0f, "%.0f");
    sh_menu_slider(x + colw, cy, colw, "vm fov", &g_cfg.visuals.viewmodel_fov_value, 40.0f, 140.0f, "%.0f");
    cy += row;

    sh_menu_checkbox(x, cy, "no smoke", &g_cfg.misc.no_smoke);
    sh_menu_checkbox(x + colw, cy, "smoke visuals", &g_cfg.visuals.smoke_edit);
    cy += row;

    sh_menu_slider(x, cy, colw, "smoke alpha", &g_cfg.visuals.smoke_alpha, 0.05f, 1.0f, "%.2f");
    cy += row;

    sh_menu_checkbox(x, cy, "lightmap night", &g_cfg.visuals.lightmap_mode);
    sh_menu_checkbox(x + colw, cy, "anim breakers", &g_cfg.visuals.anim_breakers);
    sh_menu_checkbox(x, cy + row, "killsound", &g_cfg.visuals.killsound);
    sh_menu_checkbox(x + colw, cy + row, "custom scope", &g_cfg.visuals.custom_scope);
    sh_menu_checkbox(x, cy + row * 2.0f, "3d tracer", &g_cfg.visuals.tracer3d);
    sh_menu_checkbox(x + colw, cy + row * 2.0f, "chams rim", &g_cfg.visuals.chams_rim);
    cy += row;

    if (g_cfg.visuals.anim_breakers)
    {
        sh_menu_checkbox(x, cy, "air run", &g_cfg.visuals.anim_air);
        sh_menu_checkbox(x + colw, cy, "ground run", &g_cfg.visuals.anim_ground);
        cy += row;
    }

    sh_menu_slider(x, cy, colw, "lm red", &g_cfg.visuals.lightmap_r, 0.05f, 2.0f, "%.2f");
    sh_menu_slider(x + colw, cy, colw, "lm green", &g_cfg.visuals.lightmap_g, 0.05f, 2.0f, "%.2f");
    cy += row;

    sh_menu_slider(x, cy, colw, "lm blue", &g_cfg.visuals.lightmap_b, 0.05f, 2.0f, "%.2f");
    cy += row;
    cy += row;

    sh_menu_checkbox(x, cy, "hide hud", &g_cfg.misc.hide_hud);
    sh_menu_checkbox(x + colw, cy, "hide weapons", &g_cfg.misc.hide_hud_weapons);
    cy += row;

    sh_menu_checkbox(x, cy, "hide crosshair", &g_cfg.misc.hide_hud_crosshair);
    sh_menu_checkbox(x + colw, cy, "hide health", &g_cfg.misc.hide_hud_health);
    cy += row;

    sh_menu_checkbox(x, cy, "hide timer", &g_cfg.misc.hide_hud_timer);
    sh_menu_checkbox(x + colw, cy, "hide money", &g_cfg.misc.hide_hud_money);
    cy += row;

    sh_menu_checkbox(x, cy, "no scope", &g_cfg.misc.no_scope);
    sh_menu_checkbox(x + colw, cy, "no scope borders", &g_cfg.misc.no_scope_borders);
    cy += row;

    sh_menu_checkbox(x, cy, "crosshair", &g_cfg.misc.crosshair);
    cy += row;

    sh_menu_checkbox(x, cy, "hitmarker", &g_cfg.visuals.hitmarker);
    sh_menu_checkbox(x + colw, cy, "indicators", &g_cfg.visuals.indicators);
    cy += row;

    sh_menu_checkbox(x, cy, "hitmarker damage", &g_cfg.visuals.hitmarker_damage);
    sh_menu_checkbox(x + colw, cy, "hit sound", &g_cfg.visuals.hit_sound);
    cy += row;

    sh_menu_slider(x, cy, colw, "hit vol", &g_cfg.visuals.hit_sound_volume, 0.0f, 1.0f, "%");
    cy += row;

    sh_menu_checkbox(x, cy, "radar hack", &g_cfg.misc.radar_hack);
    sh_menu_checkbox(x + colw, cy, "radar rotate", &g_cfg.misc.radar_rotate);
    cy += row;

    sh_menu_checkbox(x, cy, "radar height", &g_cfg.misc.radar_height);
}

static f32 g_movement_shots_dbg;

static void sh_menu_draw_movement(f32 x, f32 y)
{
    f32 s = sh_scale();
    f32 row = 20.0f * s;
    f32 colw = 210.0f * s;

    sh_menu_section("strafe", x, y);

    f32 cy = y + 20.0f * s;
    sh_menu_checkbox(x, cy, "auto strafe", &g_cfg.movement.autostrafe);
    sh_menu_checkbox(x + colw, cy, "quick stop", &g_cfg.movement.quick_stop);
    cy += row;

    sh_menu_checkbox(x, cy, "bunny hop", &g_cfg.movement.bhop);
    cy += row;

    sh_menu_section("exploits", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "speed hack", &g_cfg.movement.speedhack);
    sh_menu_slider(x + colw, cy, colw, "speed mult", &g_cfg.movement.speedhack_value, 0.5f, 3.0f, "x%.2f");
    cy += row;

    sh_menu_checkbox(x, cy, "ground strafe", &g_cfg.movement.ground_strafe);
    sh_menu_checkbox(x + colw, cy, "jumpbug", &g_cfg.movement.jumpbug);
    cy += row;

    sh_menu_checkbox(x, cy, "texturebug", &g_cfg.movement.texturebug);
    sh_menu_slider(x + colw, cy, colw, "gs max", &g_cfg.movement.ground_strafe_max, 200.0f, 2000.0f, "%.0f");
    cy += row;

    sh_menu_checkbox(x, cy, "strafe hack", &g_cfg.movement.strafe_hack);
    sh_menu_slider(x + colw, cy, colw, "max speed", &g_cfg.movement.strafe_hack_max, 100.0f, 3000.0f, "%.0f");
    cy += row;

    sh_menu_checkbox(x, cy, "air stuck", &g_cfg.movement.air_stuck);
    sh_menu_slider(x + colw, cy, colw, "stuck force", &g_cfg.movement.air_stuck_intensity, 0.0f, 3.0f, "%.1f");
    cy += row;

    sh_menu_checkbox(x, cy, "tick control", &g_cfg.movement.tick_control);
    cy += row;

    sh_menu_slider(x, cy, colw, "tick ms", &g_cfg.movement.tick_ms, 1.0f, 50.0f, "%.0f");
    cy += row;
    sh_menu_checkbox(x + colw, cy, "auto jump", &g_cfg.movement.auto_jump);
    cy += row + 8.0f * s;

    sh_menu_section("grenade", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "super toss", &g_cfg.movement.super_toss);
    cy += row;

    sh_menu_slider(x, cy, colw, "toss power", &g_cfg.movement.super_toss_power, 0.0f, 100.0f, "%.0f");
    cy += row + 8.0f * s;

    sh_menu_section("info", x, cy);
    cy += 20.0f * s;

    g_movement_shots_dbg = (f32)sh_shots_count();

    sh_menu_slider(x, cy, colw, "shots", &g_movement_shots_dbg, 0.0f, 40.0f, "%.0f");
    cy += row + 8.0f * s;

    sh_menu_section("grenade predict", x, cy);
    cy += 20.0f * s;

    static f32 g_gren_angle_dbg;
    static f32 g_gren_range_dbg;

    g_gren_angle_dbg = sh_grenade_optimal_angle();
    g_gren_range_dbg = sh_grenade_range(g_gren_angle_dbg, 64.0f);

    sh_menu_slider(x, cy, colw, "best angle", &g_gren_angle_dbg, 0.0f, 90.0f, "%.0f");
    cy += row;

    sh_menu_slider(x, cy, colw, "range", &g_gren_range_dbg, 0.0f, 1200.0f, "%.0f");
}

static const char *g_font_items[] = { "default", "small", "bold" };

static void sh_menu_draw_settings(f32 x, f32 y)
{
    f32 s = sh_scale();
    f32 row = 20.0f * s;
    f32 colw = 210.0f * s;

    sh_menu_section("esp", x, y);

    f32 cy = y + 20.0f * s;
    sh_menu_checkbox(x, cy, "team check", &g_cfg.visuals.team_check);
    sh_menu_checkbox(x + colw, cy, "show local", &g_cfg.esp.show_local);
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "text scale", &g_cfg.esp.text_scale, 0.5f, 2.0f, "x");
    cy += row + 6.0f * s;

    sh_menu_section("status", x, cy);
    cy += 20.0f * s;

    qbool ready = sh_sdk_ready();

    sh_text(x, cy, ready ? (col4_t)SH_COL_RGBA(120, 255, 120, 255)
                         : (col4_t)SH_COL_RGBA(255, 90, 90, 255),
            "sdk: %s", ready ? "attached" : "waiting");
    cy += 14.0f * s;

    sh_text(x, cy, (col4_t)SH_COL_RGBA(170, 170, 170, 255),
            "players: %d", sh_players_count());
    cy += 14.0f * s;

    sh_text(x, cy, (col4_t)SH_COL_RGBA(170, 170, 170, 255),
            "hits: %d   shots: %d", sh_hitmarker_count(), sh_shots_count());
    cy += row + 6.0f * s;

    sh_menu_section("keys", x, cy);
    cy += 20.0f * s;

    sh_text(x, cy, (col4_t)SH_COL_RGBA(200, 200, 200, 255), "insert - menu");
    cy += 14.0f * s;
    sh_text(x, cy, (col4_t)SH_COL_RGBA(200, 200, 200, 255), "config autosaves on close");
    cy += 14.0f * s;
    sh_text(x, cy, (col4_t)SH_COL_RGBA(200, 200, 200, 255), "delete - save config");
}

static void sh_draw_tab_button(f32 x, f32 y, f32 w, const char *label, i32 index)
{
    f32 s = sh_scale();
    qbool active = (g_tab == index);
    qbool hover = sh_mouse_in(x, y, w, 19.0f * s);

    if (active)
    {
        sh_draw_rect(x, y, w, 19.0f * s,
                     (col4_t)SH_COL_RGBA(28, 30, 36, 255));
        sh_draw_rect(x, y, 2.0f * s, 19.0f * s,
                     (col4_t)SH_COL_RGBA(190, 235, 60, 255));

        sh_text(x + 9.0f * s, y + 4.0f * s,
                (col4_t)SH_COL_RGBA(240, 240, 244, 255), "%s", label);
    }
    else if (hover)
    {
        sh_draw_rect(x, y, w, 19.0f * s,
                     (col4_t)SH_COL_RGBA(22, 23, 27, 255));

        sh_text(x + 9.0f * s, y + 4.0f * s,
                (col4_t)SH_COL_RGBA(190, 190, 196, 255), "%s", label);
    }
    else
    {
        sh_text(x + 9.0f * s, y + 4.0f * s,
                (col4_t)SH_COL_RGBA(120, 120, 128, 255), "%s", label);
    }
}

/* Диагностический watermark: рисуется ВСЕГДА, когда DLL работает.
   Нужен, чтобы за секунду отвечать на вопрос "а dll вообще жива?" -
   раньше при неоткрывающемся меню это было неотличимо от "инжект
   не прошёл". Показываем состояние sdk и подсказку по клавише. */
static void sh_menu_watermark(void)
{
    if (!g_cfg.watermark)
        return;

    f32 x = 8.0f * sh_scale();
    f32 y = 8.0f * sh_scale();

    const char *sdk = sh_sdk_ready() ? "sdk ok" : "sdk waiting";
    col4_t col = sh_sdk_ready() ? (col4_t)SH_COL_RGBA(120, 255, 140, 230)
                                : (col4_t)SH_COL_RGBA(255, 190, 80, 230);

    sh_text(x, y, col, "%s v%s  [%s]", SH_NAME, SH_VERSION, sdk);
    sh_text(x, y + 12.0f * sh_scale(), (col4_t)SH_COL_RGBA(190, 190, 190, 210),
            "INSERT - menu");
}

/* Публичная обёртка: нужна из HUD_Redraw, пока sdk не готов,
   чтобы показать "dll жива, ждём sdk" даже без полного init. */
void sh_menu_watermark_only(void)
{
    sh_menu_watermark();
}

/* ================= КАРКАС МЕНЮ ================= */

/* Позиция окна: перетаскивается за заголовок и запоминается.
   0xFF = "ещё не инициализировано", тогда центрируем. */
static f32 g_win_x = -1.0f;
static f32 g_win_y = -1.0f;

/* Прокрутка содержимого вкладки (в единицах 480p, до масштаба) */
static f32 g_scroll[6];


/* Зона заголовка — за неё тащим окно */
static f32 g_title_x;
static f32 g_title_y;
static f32 g_title_w;
static f32 g_title_h;

static qbool g_dragging_wnd;
static f32 g_drag_dx;
static f32 g_drag_dy;

#define sh_menu_w 560.0f
#define sh_menu_h_max 620.0f
#define sh_menu_title_h 26.0f
#define sh_menu_sidebar_w 104.0f
#define sh_menu_margin 16.0f
#define sh_menu_scrollbar_w 7.0f

/* Высота содержимого каждой вкладки в единицах 480p.
   Значения берутся из реальной раскладки sh_menu_draw_*() —
   последний элемент стоит примерно на этой высоте. Если добавляешь
   пункты, поправь здесь, иначе скролл не дотянет до низа. */
/* Высота считана СИМУЛЯЦИЕЙ по коду sh_menu_draw_*(): берём
   стартовый сдвиг cy, прибавляем все "cy += row" и "cy += N*s".
   Значения ниже - результат этой прокрутки, а не оценка на глаз.

   range: rage 300, antiaim 360, visuals 480, misc 820,
          movement 400, settings 196
   Запас +10 на то, что последний виджет имеет высоту ~16. */
static const f32 g_tab_content_h[6] = {
    310.0f, 370.0f, 490.0f, 830.0f, 410.0f, 206.0f
};

static void sh_menu_scroll_clamp(void)
{
    f32 view = g_scroll_rect_h;
    f32 maxs = g_scroll_content_h - view;

    if (maxs < 0.0f)
        maxs = 0.0f;

    if (g_scroll[g_tab] > maxs)
        g_scroll[g_tab] = maxs;

    if (g_scroll[g_tab] < 0.0f)
        g_scroll[g_tab] = 0.0f;
}

/* Сдвиг содержимого вверх на величину прокрутки.
   Все sh_menu_draw_*() рисуют в абсолютных координатах, поэтому
   подставляем cy уже сдвинутым, а хит-тесты внутри виджетов
   работают по тем же сдвинутым координатам — ничего менять не надо. */
/* ПОДКЛАСС ОКНА ИГРЫ — нужен ради колеса мыши.

   WM_MOUSEWHEEL приходит в оконную процедуру, а не в поток, поэтому
   без перехвата его не получить (GetAsyncKeyState колесо не отдаёт).
   Ставим свой WndProc через SetWindowLongPtr(GWLP_WNDPROC) и зовём
   оригинал для всего остального.

   Побочный плюс: пока меню открыто, гасим клики и клавиатуру, чтобы
   они не уходили в игру (иначе ESC открывал штатное меню, а ЛКМ
   стреляла). */
static WNDPROC g_orig_wndproc;
static HWND g_subclassed;
static i32 g_wheel_delta;

static LRESULT CALLBACK sh_menu_wndproc(HWND hwnd, UINT msg, WPARAM wp,
                                        LPARAM lp)
{
    if (g_visible)
    {
        if (msg == WM_MOUSEWHEEL)
        {
            g_wheel_delta += (i16)HIWORD(wp);

            /* не отдаём колесо игре: иначе переключается оружие */
            return 0;
        }

        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP ||
            msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP)
        {
            /* клики нужны меню, игре - нет */
            return 0;
        }
    }

    return CallWindowProcA(g_orig_wndproc, hwnd, msg, wp, lp);
}

static void sh_menu_subclass(void)
{
    HWND w = sh_game_window();

    if (!w || w == g_subclassed)
        return;

    /* если окно пересоздали, старый подкласс умер вместе с ним */
    g_orig_wndproc = (WNDPROC)SetWindowLongPtrA(w, GWLP_WNDPROC,
                                                (LONG_PTR)sh_menu_wndproc);

    if (g_orig_wndproc)
        g_subclassed = w;
}

static void sh_menu_scroll_input(void)
{
    f32 s = sh_scale();

    if (!g_visible)
        return;

    sh_menu_subclass();

    /* колесо: WHEEL_DELTA = 120 на щелчок */
    if (g_wheel_delta != 0)
    {
        g_scroll[g_tab] -= (f32)g_wheel_delta / 120.0f * 48.0f * s;
        g_wheel_delta = 0;
    }

    /* клавиши-дублёры, если колеса нет */
    if (GetAsyncKeyState(VK_PRIOR) & 0x8000)
        g_scroll[g_tab] -= 40.0f * s;

    if (GetAsyncKeyState(VK_NEXT) & 0x8000)
        g_scroll[g_tab] += 40.0f * s;
}

/* Ползунок: тянется мышью, как обычный скроллбар. */
static void sh_menu_scrollbar_draw(void)
{
    f32 s = sh_scale();
    f32 x = g_scroll_rect_x + g_scroll_rect_w - sh_menu_scrollbar_w * s;
    f32 y = g_scroll_rect_y;
    f32 w = sh_menu_scrollbar_w * s;
    f32 h = g_scroll_rect_h;

    f32 maxs = g_scroll_content_h - g_scroll_rect_h;

    if (maxs <= 1.0f)
        return;

    sh_draw_rect(x, y, w, h, (col4_t)SH_COL_RGBA(22, 22, 22, 200));

    {
        f32 thumb_h = h * (g_scroll_rect_h / g_scroll_content_h);

        if (thumb_h < 18.0f * s)
            thumb_h = 18.0f * s;

        {
            f32 travel = h - thumb_h;
            f32 t = g_scroll[g_tab] / maxs;

            f32 ty = y + travel * t;

            static qbool dragging;
            static f32 grab;

            qbool over = sh_mouse_in(x, ty, w, thumb_h);

            if (over && sh_mouse_clicked())
            {
                dragging = 1;
                grab = (f32)g_mouse.y - ty;
            }

            if (dragging && g_click)
            {
                f32 ny = (f32)g_mouse.y - grab;

                if (travel > 1.0f)
                {
                    f32 nt = (ny - y) / travel;

                    if (nt < 0.0f) nt = 0.0f;
                    if (nt > 1.0f) nt = 1.0f;

                    g_scroll[g_tab] = nt * maxs;
                }
            }

            if (!g_click)
                dragging = 0;

            sh_draw_rect(x, ty, w, thumb_h,
                         (col4_t)SH_COL_RGBA(dragging ? 150 : 100,
                                             dragging ? 190 : 140,
                                             dragging ? 255 : 200, 235));
        }
    }
}

void sh_menu_draw(void)
{
    f32 s;
    f32 w, h, ox, oy;
    f32 sidebar;
    f32 cx, cy;
    f32 content_h;

    sh_menu_toggle();

    sh_menu_watermark();

    if (!g_visible)
        return;

    s = sh_scale();

    w = sh_menu_w * s;

    content_h = g_tab_content_h[g_tab] + sh_menu_margin * 2.0f;

    h = content_h * s + sh_menu_title_h * s;

    if (h > sh_menu_h_max * s)
        h = sh_menu_h_max * s;

    /* начальная позиция — по центру, дальше запоминается */
    if (g_win_x < 0.0f || g_win_y < 0.0f)
    {
        g_win_x = ((f32)sh_screen_width() - w) * 0.5f;
        g_win_y = ((f32)sh_screen_height() - h) * 0.5f;
    }

    /* не даём утащить окно за пределы экрана */
    if (g_win_x > (f32)sh_screen_width() - 60.0f * s)
        g_win_x = (f32)sh_screen_width() - 60.0f * s;

    if (g_win_y > (f32)sh_screen_height() - sh_menu_title_h * s)
        g_win_y = (f32)sh_screen_height() - sh_menu_title_h * s;

    if (g_win_x < -(w - 60.0f * s))
        g_win_x = -(w - 60.0f * s);

    if (g_win_y < 0.0f)
        g_win_y = 0.0f;

    ox = g_win_x;
    oy = g_win_y;

    sh_menu_cursor_tick();
    sh_menu_input_begin();

    /* ---- зона заголовка / перетаскивание окна ---- */
    g_title_x = ox;
    g_title_y = oy;
    g_title_w = w;
    g_title_h = sh_menu_title_h * s;

    {
        qbool over_title = sh_mouse_in(g_title_x, g_title_y, g_title_w, g_title_h);

        if (over_title && sh_mouse_clicked())
        {
            g_dragging_wnd = 1;
            g_drag_dx = (f32)g_mouse.x - ox;
            g_drag_dy = (f32)g_mouse.y - oy;
        }

        if (g_dragging_wnd && g_click)
        {
            g_win_x = (f32)g_mouse.x - g_drag_dx;
            g_win_y = (f32)g_mouse.y - g_drag_dy;

            ox = g_win_x;
            oy = g_win_y;
        }

        if (!g_click)
            g_dragging_wnd = 0;
    }

    /* ---- окно ---- */
    sh_draw_rect(ox, oy, w, h, (col4_t)SH_COL_RGBA(12, 12, 12, 246));
    sh_draw_rect_outline(ox, oy, w, h, 1.0f, (col4_t)SH_COL_RGBA(52, 52, 58, 255));

    /* заголовок */
    sh_draw_rect(ox, oy, w, sh_menu_title_h * s,
                 (col4_t)SH_COL_RGBA(20, 20, 24, 255));
    sh_draw_rect(ox, oy + sh_menu_title_h * s - 1.0f, w, 1.0f,
                 (col4_t)SH_COL_RGBA(52, 52, 58, 255));

    sh_text(ox + 12.0f * s, oy + 6.0f * s,
            (col4_t)SH_COL_RGBA(235, 235, 240, 255),
            "%s", SH_NAME);
    sh_text(ox + 12.0f * s + sh_text_width(SH_NAME) + 6.0f * s,
            oy + 7.0f * s, (col4_t)SH_COL_RGBA(110, 110, 120, 255),
            "v%s", SH_VERSION);

    /* подсказка справа в заголовке */
    {
        const char *hint = "INSERT / ESC";
        f32 tw = sh_text_width(hint);

        sh_text(ox + w - tw - 12.0f * s, oy + 7.0f * s,
                (col4_t)SH_COL_RGBA(85, 85, 95, 255), "%s", hint);
    }

    /* ---- сайдбар ---- */
    sidebar = sh_menu_sidebar_w * s;

    sh_draw_rect(ox, oy + sh_menu_title_h * s, sidebar, h - sh_menu_title_h * s,
                 (col4_t)SH_COL_RGBA(16, 16, 20, 255));
    sh_draw_rect(ox + sidebar - 1.0f, oy + sh_menu_title_h * s, 1.0f,
                 h - sh_menu_title_h * s, (col4_t)SH_COL_RGBA(40, 40, 46, 255));

    for (i32 i = 0; i < (i32)sh_countof(g_tabs); ++i)
    {
        f32 ty = oy + sh_menu_title_h * s + 10.0f * s + (f32)i * 24.0f * s;

        sh_draw_tab_button(ox + 8.0f * s, ty, sidebar - 16.0f * s,
                           g_tabs[i], i);

        if (sh_mouse_clicked() &&
            sh_mouse_in(ox + 8.0f * s, ty, sidebar - 16.0f * s, 19.0f * s))
        {
            g_tab = i;
            g_dragging_wnd = 0;
        }
    }

    /* ---- область содержимого ---- */
    g_scroll_rect_x = ox + sidebar + sh_menu_margin * s;
    g_scroll_rect_y = oy + sh_menu_title_h * s + 8.0f * s;
    g_scroll_rect_w = w - sidebar - sh_menu_margin * 2.0f * s;
    g_scroll_rect_h = h - sh_menu_title_h * s - 16.0f * s;

    g_scroll_content_h = g_tab_content_h[g_tab] * s;

    sh_menu_scroll_clamp();
    sh_menu_scroll_input();
    sh_menu_scroll_clamp();

    cx = g_scroll_rect_x;
    cy = g_scroll_rect_y - g_scroll[g_tab];

    /* без отсечения элементы, уехавшие за границу области, рисуются
       поверх заголовка и сайдбара */
    sh_render_scissor_begin(g_scroll_rect_x, g_scroll_rect_y,
                            g_scroll_rect_w, g_scroll_rect_h);

    switch (g_tab)
    {
    case 0: sh_menu_draw_rage(cx, cy); break;
    case 1: sh_menu_draw_antiaim(cx, cy); break;
    case 2: sh_menu_draw_visuals(cx, cy); break;
    case 3: sh_menu_draw_misc(cx, cy); break;
    case 4: sh_menu_draw_movement(cx, cy); break;
    default: sh_menu_draw_settings(cx, cy); break;
    }

    sh_render_scissor_end();

    sh_menu_scrollbar_draw();

    /* индикатор прокрутки, если содержимое не влезло */
    if (g_scroll_content_h > g_scroll_rect_h + 1.0f)
    {
        f32 pct = g_scroll[g_tab] / (g_scroll_content_h - g_scroll_rect_h);

        sh_text(g_scroll_rect_x + g_scroll_rect_w - 60.0f * s,
                oy + h - 12.0f * s,
                (col4_t)SH_COL_RGBA(70, 70, 78, 255), "%d%%", (i32)(pct * 100.0f));
    }
}
