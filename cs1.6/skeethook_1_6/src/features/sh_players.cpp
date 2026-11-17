#include "../../include/sh_features.h"
#include "../../include/sh_math.h"

static sh_player_t g_players[SH_MAX_ENTITIES];
static i32 g_player_count;
static i32 g_local_index = -1;

static const char *g_weapon_names[] = {
    "none", "p228", "glock", "scout", "hegrenade", "xm1014", "c4",
    "mac10", "aug", "smokegrenade", "elite", "fiveseven", "ump45",
    "sg550", "galil", "famas", "usp", "glock18", "awp", "mp5",
    "m249", "m3", "m4a1", "tmp", "g3sg1", "flashbang", "deagle",
    "sg552", "ak47", "knife"};

static const char *sh_weapon_name_from_id(i32 id)
{
    if (id < 0 || id >= (i32)sh_countof(g_weapon_names))
        return "knife";
    return g_weapon_names[id];
}

static f32 sh_eye_height(i32 flags)
{
    return (flags & sh_fl_ducking) ? 12.0f : 28.0f;
}

static void sh_str_copy(char *dst, const char *src, i32 cap)
{
    if (!dst || cap <= 0)
        return;
    i32 i = 0;
    if (src)
    {
        while (src[i] && i < cap - 1)
        {
            dst[i] = src[i];
            ++i;
        }
    }
    dst[i] = 0;
}

static void sh_player_fill(sh_player_t *p, i32 index)
{
    sh_cl_entity_t *ent = sh_get_entity(index);
    if (!ent)
        return;

    for (u32 i = 0; i < sizeof(sh_player_t); ++i)
        ((u8 *)p)[i] = 0;

    p->ent = ent;
    p->index = index;
    p->origin = ent->origin;
    p->mins = ent->curstate.mins;
    p->maxs = ent->curstate.maxs;
    p->health = ent->curstate.health;
    p->armor = 0;
    p->weapon_id = ent->curstate.weaponmodel;

    if (g_engfuncs && g_engfuncs->pfnGetPlayerInfo)
        g_engfuncs->pfnGetPlayerInfo(index, &p->info);

    sh_str_copy(p->name, p->info.name, 32);
    if (!p->name[0])
        sh_str_copy(p->name, "player", 32);

    p->team = p->info.teamnumber;
    sh_str_copy(p->weapon_name, sh_weapon_name_from_id(p->weapon_id), 32);

    vec3_t delta;
    if (g_pmove)
        sh_sub(&delta, &p->origin, &g_pmove->origin);
    else
        delta.x = delta.y = delta.z = 0.0f;
    p->distance = sh_len3d(&delta);

    p->head = p->origin;
    p->head.z += sh_eye_height(ent->curstate.eflags);
    p->visible = false;
}

void sh_players_collect(void)
{
    g_player_count = 0;

    if (g_engfuncs && g_engfuncs->pfnGetLocalPlayer)
        g_local_index = g_engfuncs->pfnGetLocalPlayer();
    else
        g_local_index = -1;

    if (!g_get_entity)
        return;

    for (i32 i = 0; i < SH_MAX_ENTITIES; ++i)
    {
        if (g_player_count >= SH_MAX_ENTITIES)
            break;

        sh_cl_entity_t *ent = sh_get_entity(i);
        if (!ent || !ent->player)
            continue;
        if (i == g_local_index)
            continue;

        sh_player_t *p = &g_players[g_player_count];
        sh_player_fill(p, i);
        if (p->ent)
            ++g_player_count;
    }

    for (i32 i = 0; i < g_player_count; ++i)
        g_players[i].visible = sh_player_visible(&g_players[i]);
}

i32 sh_players_count(void)
{
    return g_player_count;
}

sh_player_t *sh_player_get(i32 index)
{
    if (index < 0 || index >= g_player_count)
        return 0;
    return &g_players[index];
}

sh_player_t *sh_player_local(void)
{
    return 0;
}


void sh_set_trace_fn(i32 (*fn)(vec3_t, vec3_t, i32, i32))
{
    g_trace_fn = fn;
}

qbool sh_trace_line(const vec3_t *start, const vec3_t *end, vec3_t *hitsh)
{
    if (!g_trace_fn)
        return false;

    i32 hit = g_trace_fn(*start, *end, 0, -1);
    if (!hit)
        return true;
    if (hitsh)
        *hitsh = *end;
    return false;
}

void sh_msg_enemy_fire(i32 index);

static i32 g_last_anim[33];

void sh_players_track_anim(void)
{
    for (i32 i = 1; i <= 32; ++i)
    {
        sh_player_t *p = sh_player_get(i);

        if (!p || !p->ent)
            continue;

        i32 anim = p->ent->curstate.weaponanim;
        i32 prev = g_last_anim[i];

        g_last_anim[i] = anim;

        if (prev == 0 || anim == prev)
            continue;

        if (anim == 1 || anim == 2 || anim == 3)
        {
            if (p->index != g_local_player)
                sh_msg_enemy_fire(i);
        }

        if (p->weapon_id == 9 && anim == 6)
            sh_smoke_add(&p->origin);
    }
}

qbool sh_player_visible(const sh_player_t *p)
{
    if (!p || !g_pmove)
        return false;

    vec3_t start = g_pmove->origin;
    start.z += sh_eye_height(g_pmove->flags);

    if (p->ent && p->ent->curstate.health <= 0)
        return false;

    if (!g_trace_fn)
        return true;

    i32 hit = g_trace_fn(start, p->head, 0, -1);
    return hit != 0;
}

#define sh_max_smokes 8

typedef struct sh_smoke_s
{
    vec3_t origin;
    f32 born;
    qbool active;
} sh_smoke_t;

static sh_smoke_t g_smokes[sh_max_smokes];
static i32 g_smoke_cursor;

void sh_smoke_add(const vec3_t *origin)
{
    if (!origin)
        return;

    sh_smoke_t *s = &g_smokes[g_smoke_cursor];

    s->origin = *origin;
    s->born = g_pmove ? g_pmove->time : 0.0f;
    s->active = 1;

    g_smoke_cursor = (g_smoke_cursor + 1) % sh_max_smokes;
}

void sh_smoke_tick(f32 now)
{
    for (i32 i = 0; i < sh_max_smokes; ++i)
    {
        if (!g_smokes[i].active)
            continue;

        if (now - g_smokes[i].born > 18.0f)
            g_smokes[i].active = 0;
    }
}

static qbool sh_segment_sphere(const vec3_t *a, const vec3_t *b,
                               const vec3_t *c, f32 radius)
{
    vec3_t ab = *b;
    ab.x -= a->x;
    ab.y -= a->y;
    ab.z -= a->z;

    f32 len2 = sh_dot(&ab, &ab);

    if (len2 < 0.001f)
        return 0;

    vec3_t ac = *c;
    ac.x -= a->x;
    ac.y -= a->y;
    ac.z -= a->z;

    f32 t = sh_dot(&ac, &ab) / len2;

    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;

    vec3_t closest = *a;
    closest.x += ab.x * t;
    closest.y += ab.y * t;
    closest.z += ab.z * t;

    vec3_t d = *c;
    d.x -= closest.x;
    d.y -= closest.y;
    d.z -= closest.z;

    return sh_len3d(&d) <= radius;
}

qbool sh_player_behind_smoke(const sh_player_t *p)
{
    if (!p || !g_pmove)
        return 0;

    vec3_t eye = g_pmove->origin;
    eye.z += sh_eye_height(g_pmove->flags);

    for (i32 i = 0; i < sh_max_smokes; ++i)
    {
        if (!g_smokes[i].active)
            continue;

        if (sh_segment_sphere(&eye, &p->head, &g_smokes[i].origin, 200.0f))
            return 1;
    }

    return 0;
}
