#include "../../include/sh_bones.h"
#include "../../include/sh_features.h"
#include "../../include/sh_render.h"
#include "../../include/sh_math.h"

static sh_studio_api_t *g_studio;
static sh_bone_snapshot_t g_bones[SH_MAX_ENTITIES];

void sh_bones_studio_set(sh_studio_api_t *api)
{
    g_studio = api;
}

void sh_bones_clear(void)
{
    for (i32 i = 0; i < SH_MAX_ENTITIES; ++i)
        g_bones[i].valid = 0;
}

void sh_bones_capture(i32 index)
{
    if (!g_studio || !g_studio->studio_get_bone_transform)
        return;

    if (index <= 0 || index >= SH_MAX_ENTITIES)
        return;

    void ***raw = (void ***)g_studio->studio_get_bone_transform();

    if (!raw)
        return;

    sh_bone_snapshot_t *snap = &g_bones[index];

    i32 count = 0;

    for (i32 i = 0; i < sh_max_bones; ++i)
    {
        if (!raw[i])
            break;

        f32 **rows = (f32 **)raw[i];

        if (!rows || !rows[0] || !rows[1] || !rows[2])
            break;

        snap->pos[i].x = rows[0][3];
        snap->pos[i].y = rows[1][3];
        snap->pos[i].z = rows[2][3];

        ++count;
    }

    if (count < 17)
        return;

    snap->index = index;
    snap->count = count;
    snap->born = g_pmove ? g_pmove->time : 0.0f;
    snap->valid = 1;
}

const sh_bone_snapshot_t *sh_bones_get(i32 index)
{
    if (index <= 0 || index >= SH_MAX_ENTITIES)
        return 0;

    if (!g_bones[index].valid)
        return 0;

    return &g_bones[index];
}

qbool sh_bones_world_to_screen(const sh_bone_snapshot_t *snap, i32 bone, vec2_t *out)
{
    if (!snap || !out || bone < 0 || bone >= snap->count)
        return 0;

    return sh_world_to_screen(&snap->pos[bone], out);
}
