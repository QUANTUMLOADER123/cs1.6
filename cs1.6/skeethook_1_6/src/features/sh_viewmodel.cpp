#include "../../include/sh_viewmodel.h"
#include "../../include/sh_config.h"
#include "../../include/sh_features.h"
#include "../../include/sh_math.h"
#include "../../include/sh_sdk.h"

static sh_cl_entity_t *sh_viewmodel_ent(void)
{
    if (!g_engfuncs || !g_engfuncs->pfnGetViewModel)
        return 0;

    return (sh_cl_entity_t *)g_engfuncs->pfnGetViewModel();
}

void sh_viewmodel_tick(void)
{
    if (!g_cfg.visuals.viewmodel_offset && !g_cfg.visuals.viewmodel_fov)
        return;

    sh_cl_entity_t *vm = sh_viewmodel_ent();

    if (!vm)
        return;

    if (!g_pmove)
        return;

    if (g_pmove->spectator || g_pmove->deadflag)
        return;

    vec3_t fwd, right, up;
    vec3_t ang = g_pmove->angles;

    sh_angle_vectors(&ang, &fwd, &right, &up);

    if (g_cfg.visuals.viewmodel_offset)
    {
        f32 back = g_cfg.visuals.viewmodel_x;
        f32 side = g_cfg.visuals.viewmodel_y;
        f32 vert = g_cfg.visuals.viewmodel_z;

        vm->origin.x += fwd.x * back + right.x * side + up.x * vert;
        vm->origin.y += fwd.y * back + right.y * side + up.y * vert;
        vm->origin.z += fwd.z * back + right.z * side + up.z * vert;
    }
}

f32 sh_viewmodel_fov_scale(void)
{
    if (!g_cfg.visuals.viewmodel_fov)
        return 1.0f;

    f32 v = g_cfg.visuals.viewmodel_fov_value;

    if (v < 40.0f)
        v = 40.0f;
    if (v > 140.0f)
        v = 140.0f;

    f32 ref = g_cfg.visuals.fov_value;

    if (ref < 40.0f)
        ref = 90.0f;

    return v / ref;
}

void sh_fov_apply(void)
{
    if (!g_engfuncs || !g_engfuncs->pfnCvar_SetValue)
        return;

    if (!g_cfg.visuals.fov_enabled)
        return;

    f32 f = g_cfg.visuals.fov_value;

    if (f < 40.0f)
        f = 40.0f;
    if (f > 160.0f)
        f = 160.0f;

    g_engfuncs->pfnCvar_SetValue("default_fov", f);
}

void sh_aspect_apply(void)
{
    if (!g_engfuncs || !g_engfuncs->pfnCvar_SetValue)
        return;

    if (!g_cfg.visuals.aspect_ratio)
    {
        g_engfuncs->pfnCvar_SetValue("r_aspectratio", 0.0f);
        return;
    }

    f32 a = g_cfg.visuals.aspect_value;

    if (a < 0.5f)
        a = 0.5f;
    if (a > 3.0f)
        a = 3.0f;

    g_engfuncs->pfnCvar_SetValue("r_aspectratio", a);
}

void sh_remove_bob(void)
{
    if (!g_engfuncs || !g_engfuncs->pfnCvar_SetValue)
        return;

    if (!g_cfg.visuals.no_bob)
        return;

    g_engfuncs->pfnCvar_SetValue("cl_bob", 0.0f);
    g_engfuncs->pfnCvar_SetValue("cl_bobcycle", 0.0f);
}
