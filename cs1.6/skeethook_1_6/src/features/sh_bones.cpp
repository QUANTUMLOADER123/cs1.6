#include "../../include/sh_bones.h"
#include "../../include/sh_math.h"
#include "../../include/sh_config.h"

static sh_bone_snapshot_t g_bones[SH_MAX_ENTITIES];

void sh_bones_clear(void)
{
    for (i32 i = 0; i < SH_MAX_ENTITIES; ++i)
        g_bones[i].valid = 0;
}

void sh_bones_capture(i32 index, f32 (*matrix)[3][4], i32 count)
{
    if (index < 0 || index >= SH_MAX_ENTITIES || !matrix)
        return;

    sh_bone_snapshot_t *s = &g_bones[index];
    s->index = index;
    s->count = sh_clampi(count, 0, sh_max_bones);
    s->valid = s->count > 0;

    for (i32 b = 0; b < s->count; ++b)
    {
        s->pos[b].x = matrix[b][0][3];
        s->pos[b].y = matrix[b][1][3];
        s->pos[b].z = matrix[b][2][3];
    }
}

const sh_bone_snapshot_t *sh_bones_get(i32 index)
{
    if (index < 0 || index >= SH_MAX_ENTITIES)
        return 0;
    if (!g_bones[index].valid)
        return 0;
    return &g_bones[index];
}

qbool sh_bones_world_to_screen(const sh_bone_snapshot_t *snap, i32 bone, vec2_t *out)
{
    if (!snap || !snap->valid || bone < 0 || bone >= snap->count)
        return 0;
    return sh_world_to_screen(&snap->pos[bone], out);
}
