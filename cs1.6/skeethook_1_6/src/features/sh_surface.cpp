#include "../../include/sh_surface.h"
#include "../../include/sh_features.h"
#include "../../include/sh_math.h"

#include <string.h>

static char sh_upper(char c)
{
    if (c >= 'a' && c <= 'z')
        return (char)(c - 32);

    return c;
}

i32 sh_surface_from_char(char c)
{
    switch (sh_upper(c))
    {
    case 'C': return sh_surface_concrete;
    case 'M': return sh_surface_metal;
    case 'D': return sh_surface_dirt;
    case 'V': return sh_surface_vent;
    case 'G': return sh_surface_grate;
    case 'T': return sh_surface_tile;
    case 'S': return sh_surface_slosh;
    case 'W': return sh_surface_wood;
    case 'P': return sh_surface_computer;
    case 'X': return sh_surface_grass;
    case 'Y': return sh_surface_glass;
    case 'F': return sh_surface_flesh;
    default: return sh_surface_unknown;
    }
}

const char *sh_surface_name(i32 type)
{
    static const char *names[] = {
        "unknown", "concrete", "metal", "dirt", "vent",
        "grate", "tile", "slosh", "wood", "computer",
        "grass", "glass", "flesh"
    };

    if (type < 0 || type >= (i32)(sizeof(names) / sizeof(names[0])))
        return "unknown";

    return names[type];
}

char sh_surface_char(const char *name)
{
    if (!name || !name[0])
        return 0;

    return sh_upper(name[0]);
}

char sh_surface_under_origin(const vec3_t *origin)
{
    if (!origin)
        return 0;

    if (g_pmove && g_pmove->pm_trace_texture)
    {
        vec3_t start = *origin;
        vec3_t end = *origin;

        start.z += 4.0f;
        end.z -= 32.0f;

        const char *tex = g_pmove->pm_trace_texture(1, (f32 *)&start,
                                                    (f32 *)&end);

        if (tex && tex[0])
            return sh_upper(tex[0]);
    }

    return 0;
}

i32 sh_surface_under_player(i32 index)
{
    sh_player_t *p = sh_player_get(index);

    if (!p)
        return sh_surface_unknown;

    vec3_t foot = p->origin;
    foot.z = p->mins.z;

    char c = sh_surface_under_origin(&foot);

    return sh_surface_from_char(c);
}

char sh_surface_local(void)
{
    if (!g_pmove)
        return 0;

    return sh_upper(g_pmove->chtexturetype);
}

f32 sh_surface_jump_multiplier(i32 type)
{
    switch (type)
    {
    case sh_surface_slosh:
    case sh_surface_dirt:
    case sh_surface_grass:
        return 0.85f;
    case sh_surface_metal:
    case sh_surface_grate:
        return 1.1f;
    case sh_surface_wood:
    case sh_surface_tile:
        return 1.0f;
    default:
        return 1.0f;
    }
}

f32 sh_surface_friction_multiplier(i32 type)
{
    switch (type)
    {
    case sh_surface_slosh:
        return 0.4f;
    case sh_surface_glass:
        return 0.2f;
    case sh_surface_grass:
    case sh_surface_dirt:
        return 0.75f;
    default:
        return 1.0f;
    }
}
