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

        for (i32 r = 0; r < 3; ++r)
            for (i32 c = 0; c < 4; ++c)
                snap->mat[b][r][c] = rows[r][c];

        ++got;
    }

    return got;
}

static void sh_bones_read_bbox(sh_bone_snapshot_t *snap,
                               const sh_studio_header_t *hdr)
{
    snap->bbox_count = 0;

    if (!hdr || hdr->numhitboxes <= 0 || hdr->hitboxindex <= 0)
        return;

    const u8 *base = (const u8 *)hdr + hdr->hitboxindex;
    i32 n = hdr->numhitboxes;

    if (n > sh_max_bbox)
        n = sh_max_bbox;

    for (i32 i = 0; i < n; ++i)
    {
        const sh_studio_bbox_t *bb = (const sh_studio_bbox_t *)(base + i * sizeof(sh_studio_bbox_t));

        if (bb->bone < 0 || bb->bone >= sh_max_bones)
            continue;

        snap->bbox[snap->bbox_count].bone = bb->bone;

        for (i32 k = 0; k < 3; ++k)
        {
            snap->bbox[snap->bbox_count].bbmin[k] = bb->bbmin[k];
            snap->bbox[snap->bbox_count].bbmax[k] = bb->bbmax[k];
        }

        ++snap->bbox_count;
    }
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
        sh_bones_read_bbox(snap, hdr);
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

    {
        i32 h = snap->hist_head;

        snap->hist[h].time = g_pmove ? g_pmove->time : 0.0f;
        snap->hist[h].valid = 1;

        for (i32 b = 0; b < sh_bone_count; ++b)
            snap->hist[h].pos[b] = snap->pos[b];

        snap->hist_head = (h + 1) % sh_bone_history;
    }

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

i32 sh_bones_bbox_corners(const sh_bone_snapshot_t *snap, i32 box, vec3_t out[8])
{
    if (!snap || !out || !snap->valid)
        return 0;

    if (box < 0 || box >= snap->bbox_count)
        return 0;

    const sh_bbox_snap_t *bb = &snap->bbox[box];

    i32 bi = -1;

    for (i32 b = 0; b < sh_bone_count; ++b)
    {
        if (snap->map[b] == bb->bone)
        {
            bi = b;
            break;
        }
    }

    if (bi < 0)
        return 0;

    const f32(*m)[4] = snap->mat[bi];

    if (m[0][0] == 0.0f && m[1][1] == 0.0f && m[2][2] == 0.0f)
        return 0;

    for (i32 c = 0; c < 8; ++c)
    {
        f32 lx = (c & 1) ? bb->bbmax[0] : bb->bbmin[0];
        f32 ly = (c & 2) ? bb->bbmax[1] : bb->bbmin[1];
        f32 lz = (c & 4) ? bb->bbmax[2] : bb->bbmin[2];

        out[c].x = m[0][0] * lx + m[0][1] * ly + m[0][2] * lz + m[0][3];
        out[c].y = m[1][0] * lx + m[1][1] * ly + m[1][2] * lz + m[1][3];
        out[c].z = m[2][0] * lx + m[2][1] * ly + m[2][2] * lz + m[2][3];
    }

    return 1;
}

i32 sh_bones_backtrack_frames(i32 index)
{
    if (index <= 0 || index >= sh_max_bone_players)
        return 0;

    const sh_bone_snapshot_t *snap = &g_bones[index];

    if (!snap->valid)
        return 0;

    i32 n = 0;

    for (i32 i = 0; i < sh_bone_history; ++i)
        if (snap->hist[i].valid)
            ++n;

    return n;
}

qbool sh_bones_backtrack_vec(i32 index, i32 bone, i32 back, vec3_t *out)
{
    if (!out || index <= 0 || index >= sh_max_bone_players)
        return 0;

    if (bone < 0 || bone >= sh_bone_count)
        return 0;

    const sh_bone_snapshot_t *snap = &g_bones[index];

    if (!snap->valid)
        return 0;

    if (back < 0)
        back = 0;
    if (back >= sh_bone_history)
        back = sh_bone_history - 1;

    i32 h = (snap->hist_head - 1 - back + sh_bone_history * 2) % sh_bone_history;

    if (!snap->hist[h].valid)
        return 0;

    *out = snap->hist[h].pos[bone];

    return 1;
}

static f32 sh_ang_diff(f32 a, f32 b)
{
    f32 d = a - b;

    while (d > 180.0f)
        d -= 360.0f;
    while (d < -180.0f)
        d += 360.0f;

    return d;
}

qbool sh_backtrack_lerp(i32 ent_index, f32 target_time, vec3_t *out_origin,
                        vec3_t *out_angles, qbool *extrap)
{
    if (!out_origin || !out_angles)
        return 0;

    if (ent_index <= 0 || ent_index >= sh_max_bone_players)
        return 0;

    if (!g_engfuncs || !g_engfuncs->pfnGetEntityByIndex)
        return 0;

    sh_cl_entity_t *ent = (sh_cl_entity_t *)g_engfuncs->pfnGetEntityByIndex(ent_index);

    if (!ent)
        return 0;

    i32 imod = ent->current_position & sh_hist_mask;
    i32 i0 = imod;
    i32 i1 = (imod - 1) & sh_hist_mask;
    qbool extrapolate = 1;

    if (target_time <= ent->ph[i0].animtime)
    {
        for (i32 i = 1; i < sh_hist_mask - 1; ++i)
        {
            f32 at = ent->ph[(imod - i) & sh_hist_mask].animtime;

            if (at == 0.0f)
                break;

            if (target_time > at)
            {
                i0 = ((imod - i) + 1) & sh_hist_mask;
                i1 = ((imod - i) + 0) & sh_hist_mask;
                extrapolate = 0;
                break;
            }
        }
    }

    const sh_position_history_t *p0 = &ent->ph[i0];
    const sh_position_history_t *p1 = &ent->ph[i1];

    f32 t0 = p0->animtime;
    f32 t1 = p1->animtime;

    if (t0 == 0.0f || t1 == 0.0f)
        return 0;

    f32 frac = 0.0f;

    if (t0 != t1)
        frac = (target_time - t0) / (t1 - t0);

    if (frac < 0.0f)
        frac = 0.0f;
    if (frac > 1.0f)
        frac = 1.0f;

    out_origin->x = p0->origin.x + (p1->origin.x - p0->origin.x) * frac;
    out_origin->y = p0->origin.y + (p1->origin.y - p0->origin.y) * frac;
    out_origin->z = p0->origin.z + (p1->origin.z - p0->origin.z) * frac;

    out_angles->x = p0->angles.x + (p1->angles.x - p0->angles.x) * frac;
    out_angles->y = p0->angles.y + sh_ang_diff(p1->angles.y, p0->angles.y) * frac;
    out_angles->z = p0->angles.z + (p1->angles.z - p0->angles.z) * frac;

    if (extrap)
        *extrap = extrapolate;

    return 1;
}

f32 sh_backtrack_time(i32 ent_index, i32 frames_back)
{
    if (ent_index <= 0 || ent_index >= sh_max_bone_players)
        return 0.0f;

    if (!g_engfuncs || !g_engfuncs->pfnGetEntityByIndex)
        return 0.0f;

    sh_cl_entity_t *ent = (sh_cl_entity_t *)g_engfuncs->pfnGetEntityByIndex(ent_index);

    if (!ent)
        return 0.0f;

    i32 imod = ent->current_position & sh_hist_mask;
    f32 newest = ent->ph[imod].animtime;

    if (newest <= 0.0f)
        return 0.0f;

    if (frames_back < 1)
        frames_back = 1;

    f32 tick = 1.0f / 100.0f;

    if (g_pmove && g_pmove->frametime > 0.0f)
        tick = g_pmove->frametime;

    f32 target = newest - tick * (f32)frames_back;

    f32 oldest = 0.0f;

    for (i32 i = 1; i < sh_hist_mask; ++i)
    {
        f32 at = ent->ph[(imod - i) & sh_hist_mask].animtime;

        if (at == 0.0f)
            break;

        oldest = at;
    }

    if (oldest > 0.0f && target < oldest)
        target = oldest;

    return target;
}
