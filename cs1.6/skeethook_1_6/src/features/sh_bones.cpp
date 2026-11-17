#include "../../include/sh_bones.h"
#include "../../include/sh_features.h"
#include "../../include/sh_render.h"
#include "../../include/sh_math.h"

#include <windows.h>
#include "../ext/minhook/include/MinHook.h"

sh_studio_api_t *g_studio_api;
static sh_bones_draw_orig_fn g_draw_orig;
static sh_bone_snapshot_t g_bones[sh_max_bone_players];

static const char *g_bone_names[sh_bone_count] = {
    "Bip01 Head",
    "Bip01 Neck",
    "Bip01 Spine3",
    "Bip01 Spine2",
    "Bip01 Spine1",
    "Bip01 Pelvis",
    "Bip01 L Clavicle",
    "Bip01 L UpperArm",
    "Bip01 L Forearm",
    "Bip01 L Hand",
    "Bip01 R Clavicle",
    "Bip01 R UpperArm",
    "Bip01 R Forearm",
    "Bip01 R Hand",
    "Bip01 L Thigh",
    "Bip01 L Calf",
    "Bip01 L Foot",
    "Bip01 R Thigh",
    "Bip01 R Calf",
    "Bip01 R Foot"
};

static qbool sh_name_eq(const char *a, const char *b)
{
    if (!a || !b)
        return 0;

    for (i32 i = 0; i < 32; ++i)
    {
        char ca = a[i];
        char cb = b[i];

        if (ca >= 'a' && ca <= 'z')
            ca = (char)(ca - 32);
        if (cb >= 'a' && cb <= 'z')
            cb = (char)(cb - 32);

        if (ca != cb)
            return 0;

        if (!ca)
            return 1;
    }

    return 1;
}

void sh_bones_studio_set(sh_studio_api_t *api)
{
    g_studio_api = api;
}

void sh_bones_clear(void)
{
    for (i32 i = 0; i < sh_max_bone_players; ++i)
    {
        g_bones[i].valid = 0;
        g_bones[i].count = 0;
    }
}

i32 sh_minhook_create(void *target, void *detour, void **orig)
{
    if (!target || !detour || !orig)
        return 0;

    if (MH_CreateHook(target, detour, orig) != MH_OK)
        return 0;

    if (MH_EnableHook(target) != MH_OK)
        return 0;

    return 1;
}

static void sh_bones_fallback_map(sh_bone_snapshot_t *snap, i32 n)
{
    static const i32 def[sh_bone_count] = {
        9, 8, 7, 6, 5, 4, 14, 15, 16, 17,
        38, 39, 40, 41, 50, 51, 52, 53, 54, 55
    };

    for (i32 b = 0; b < sh_bone_count; ++b)
    {
        i32 v = def[b];

        snap->map[b] = (v < n) ? v : -1;
    }
}

static void sh_bones_build_map(sh_bone_snapshot_t *snap,
                               const sh_studio_header_t *hdr)
{
    const sh_studio_bone_t *bones =
        (const sh_studio_bone_t *)((const u8 *)hdr + hdr->boneindex);

    i32 n = hdr->numbones;

    if (n > sh_max_bones)
        n = sh_max_bones;

    snap->count = n;

    for (i32 b = 0; b < sh_bone_count; ++b)
        snap->map[b] = -1;

    for (i32 i = 0; i < n; ++i)
    {
        for (i32 b = 0; b < sh_bone_count; ++b)
        {
            if (snap->map[b] >= 0)
                continue;

            if (sh_name_eq(bones[i].name, g_bone_names[b]))
                snap->map[b] = i;
        }
    }
}

const sh_studio_header_t *sh_bones_current_header(void)
{
    if (!g_studio_api || !g_studio_api->get_current_entity || !g_studio_api->mod_extradata)
        return 0;

    sh_cl_entity_t *ent = g_studio_api->get_current_entity();

    if (!ent)
        return 0;

    void *model = (void *)ent->model;

    if (!model)
        return 0;

    return (const sh_studio_header_t *)g_studio_api->mod_extradata(model);
}

static i32 sh_bones_fill(sh_bone_snapshot_t *snap, void ***raw, i32 n)
{
    i32 got = 0;

    for (i32 b = 0; b < sh_bone_count; ++b)
    {
        i32 bi = snap->map[b];

        if (bi < 0 || bi >= n)
            continue;

        f32 **rows = (f32 **)raw[bi];

        if (!rows || !rows[0] || !rows[1] || !rows[2])
            continue;

        snap->pos[b].x = rows[0][3];
        snap->pos[b].y = rows[1][3];
        snap->pos[b].z = rows[2][3];

        ++got;
    }

    return got;
}

void sh_bones_capture(i32 index, const sh_studio_header_t *hdr)
{
    if (!g_studio_api || !g_studio_api->studio_get_bone_transform)
        return;

    if (index <= 0 || index >= sh_max_bone_players || !hdr)
        return;

    if (hdr->numbones <= 0 || hdr->boneindex <= 0)
        return;

    void ***raw = (void ***)g_studio_api->studio_get_bone_transform();

    if (!raw)
        return;

    sh_bone_snapshot_t *snap = &g_bones[index];

    if (!snap->valid || snap->model != (const void *)hdr)
    {
        sh_bones_build_map(snap, hdr);
        snap->model = (const void *)hdr;
    }

    i32 n = snap->count;

    if (n <= 0)
        return;

    i32 got = sh_bones_fill(snap, raw, n);

    if (got < 8)
    {
        sh_bones_fallback_map(snap, n);
        got = sh_bones_fill(snap, raw, n);
    }

    if (got < 6)
        return;

    snap->index = index;
    snap->born = g_pmove ? g_pmove->time : 0.0f;
    snap->valid = 1;
}

const sh_bone_snapshot_t *sh_bones_get(i32 index)
{
    if (index <= 0 || index >= sh_max_bone_players)
        return 0;

    if (!g_bones[index].valid)
        return 0;

    if (g_pmove)
    {
        f32 now = g_pmove->time;

        if (now > g_bones[index].born && now - g_bones[index].born > 0.25f)
            return 0;
    }

    return &g_bones[index];
}

qbool sh_bones_world_to_screen(const sh_bone_snapshot_t *snap, i32 bone, vec2_t *out)
{
    if (!snap || !out || bone < 0 || bone >= sh_bone_count)
        return 0;

    if (snap->map[bone] < 0)
        return 0;

    return sh_world_to_screen(&snap->pos[bone], out);
}

static i32 sh_bones_draw_player(i32 flags, sh_entity_state_t *pplayer)
{
    i32 r = 0;

    if (g_draw_orig)
        r = g_draw_orig(flags, pplayer);

    if (pplayer && pplayer->number > 0)
    {
        const sh_studio_header_t *hdr = sh_bones_current_header();

        sh_bones_capture(pplayer->number, hdr);
    }

    return r;
}

void sh_bones_hook_iface(sh_studio_iface_t *iface)
{
    if (!iface || !iface->studio_draw_player)
        return;

    if (g_draw_orig)
        return;

    if (!sh_minhook_create((void *)iface->studio_draw_player,
                           (void *)sh_bones_draw_player,
                           (void **)&g_draw_orig))
        return;

    iface->studio_draw_player = sh_bones_draw_player;
}
