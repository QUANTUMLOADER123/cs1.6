#include "../../include/sh_features.h"
#include "../../include/sh_hitmarker.h"

sh_msg_state_t g_msgs;

static const char *g_weapon_icons[] = {
    "?", "", "p228", "glock", "scout", "hegrenade", "xm1014", "c4",
    "mac10", "aug", "smokegrenade", "elite", "fiveseven", "ump45",
    "sg550", "galil", "famas", "usp", "glock18", "awp", "mp5navy",
    "m249", "m3", "m4a1", "tmp", "g3sg1", "flashbang", "deagle",
    "sg552", "ak47", "knife", "p90"
};

void sh_msgs_reset(void)
{
    for (u32 i = 0; i < sizeof(g_msgs); ++i)
        ((u8 *)&g_msgs)[i] = 0;
}

void sh_msg_weapon_list(const char *name, i32 id, i32 slot, i32 clip,
                        i32 clip_max, i32 ammo_type)
{
    if (id < 0 || id >= 64)
        return;

    sh_weapon_list_t *w = &g_msgs.weapons[id];

    i32 n = 0;

    if (name)
    {
        while (name[n] && n < 31)
        {
            w->name[n] = name[n];
            ++n;
        }
    }

    w->name[n] = 0;
    w->id = id;
    w->slot = slot;
    w->clip = clip;
    w->clip_max = clip_max;
    w->ammo_type = ammo_type;
    w->valid = 1;

    if (id >= g_msgs.weapon_count)
        g_msgs.weapon_count = id + 1;
}

void sh_msg_cur_weapon(i32 state, i32 id, i32 clip)
{
    if (state)
    {
        g_msgs.cur_weapon_id = id;
        g_msgs.cur_weapon_clip = clip;
    }
    else
    {
        g_msgs.cur_weapon_clip = clip;
    }
}

const sh_weapon_list_t *sh_weapon_get(i32 id)
{
    if (id < 0 || id >= 64)
        return 0;

    if (!g_msgs.weapons[id].valid)
        return 0;

    return &g_msgs.weapons[id];
}

const char *sh_weapon_icon(i32 id)
{
    if (id < 0 || id >= (i32)sh_countof(g_weapon_icons))
        return "?";

    return g_weapon_icons[id];
}

void sh_msg_score_attrib(i32 index, u8 attrib)
{
    if (index < 0 || index >= SH_MAX_ENTITIES)
        return;

    g_msgs.attrib[index] = attrib;
}

void sh_msg_round_time(i32 seconds)
{
    g_msgs.round_seconds = seconds;
}

i32 sh_player_is_dead(i32 index)
{
    if (index < 0 || index >= SH_MAX_ENTITIES)
        return 0;

    return (g_msgs.attrib[index] & 1) != 0;
}

void sh_msg_enemy_fire(i32 index)
{
    (void)index;
}

static u32 sh_str_len(const char *s, u32 max)
{
    u32 n = 0;

    while (n < max && s[n])
        ++n;

    return n;
}

static i32 __cdecl sh_cb_weapon_list(const char *buf, i32 size, i32 read)
{
    (void)size;

    if (!buf || read < 10)
        return 1;

    u32 n = sh_str_len(buf, 63);
    const u8 *b = (const u8 *)buf + n + 1;

    i32 primary_id = b[0];
    i32 primary_max = b[1];
    i32 secondary_id = b[2];
    i32 secondary_max = b[3];
    i32 slot = b[4];
    i32 number = b[5];
    i32 weapon_id = b[6];

    (void)primary_max;
    (void)secondary_id;
    (void)secondary_max;
    (void)number;

    sh_msg_weapon_list(buf, weapon_id, slot, primary_id, primary_max, 0);

    return 1;
}

static i32 __cdecl sh_cb_cur_weapon(const char *name, i32 size, i32 read)
{
    (void)size;

    if (read < 3 || !name)
        return 1;

    const u8 *p = (const u8 *)name;

    sh_msg_cur_weapon(p[0], p[1], p[2]);

    return 1;
}

static i32 g_last_health = -1;
static i32 g_last_armor = -1;
static f32 g_damage_done_time = -1.0f;

static i32 __cdecl sh_cb_health(const char *name, i32 size, i32 read)
{
    (void)size;

    if (read < 1 || !name)
        return 1;

    i32 hp = (u8)name[0];

    if (g_last_health >= 0 && hp < g_last_health && hp > 0)
    {
        i32 dmg = g_last_health - hp;

        if (g_last_health > 0 && dmg > 0)
        {
            sh_hitmarker_on_hit(0, dmg);
            g_damage_done_time = g_pmove ? g_pmove->time : 0.0f;
        }
    }

    g_last_health = hp;

    return 1;
}

static i32 __cdecl sh_cb_armor(const char *name, i32 size, i32 read)
{
    (void)size;

    if (read < 1 || !name)
        return 1;

    g_last_armor = (u8)name[0];

    return 1;
}

static i32 __cdecl sh_cb_damage(const char *name, i32 size, i32 read)
{
    (void)size;

    if (read < 4 || !name)
        return 1;

    const u8 *p = (const u8 *)name;

    f32 t = g_pmove ? g_pmove->time : 0.0f;

    if (t - g_damage_done_time < 0.05f)
        return 1;

    f32 dmg = (f32)p[2] + (f32)p[3] * 256.0f;

    if (dmg > 0.0f && dmg < 200.0f)
    {
        sh_hitmarker_on_hit(0, (i32)dmg);
        g_damage_done_time = t;
    }

    return 1;
}

static i32 __cdecl sh_cb_score_attrib(const char *name, i32 size, i32 read)
{
    (void)size;

    if (read < 2 || !name)
        return 1;

    const u8 *p = (const u8 *)name;

    sh_msg_score_attrib(p[0], p[1]);

    return 1;
}

static i32 __cdecl sh_cb_round_time(const char *name, i32 size, i32 read)
{
    (void)size;

    if (read < 1 || !name)
        return 1;

    sh_msg_round_time(((const u8 *)name)[0]);

    return 1;
}

void sh_msgs_register(void)
{
    if (!g_engfuncs || !g_engfuncs->pfnHookUserMsg)
        return;

    g_engfuncs->pfnHookUserMsg("WeaponList", sh_cb_weapon_list);
    g_engfuncs->pfnHookUserMsg("CurWeapon", sh_cb_cur_weapon);
    g_engfuncs->pfnHookUserMsg("Health", sh_cb_health);
    g_engfuncs->pfnHookUserMsg("Armor", sh_cb_armor);
    g_engfuncs->pfnHookUserMsg("Damage", sh_cb_damage);
    g_engfuncs->pfnHookUserMsg("ScoreAttrib", sh_cb_score_attrib);
    g_engfuncs->pfnHookUserMsg("RoundTime", sh_cb_round_time);
}
