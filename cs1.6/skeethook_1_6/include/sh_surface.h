#pragma once

#include "sh_types.h"
#include "sh_sdk.h"

enum sh_surface_type
{
    sh_surface_unknown = 0,
    sh_surface_concrete,
    sh_surface_metal,
    sh_surface_dirt,
    sh_surface_vent,
    sh_surface_grate,
    sh_surface_tile,
    sh_surface_slosh,
    sh_surface_wood,
    sh_surface_computer,
    sh_surface_grass,
    sh_surface_glass,
    sh_surface_flesh
};

char sh_surface_char(const char *name);
i32 sh_surface_from_char(char c);
const char *sh_surface_name(i32 type);

char sh_surface_under_origin(const vec3_t *origin);
i32 sh_surface_under_player(i32 index);

f32 sh_surface_jump_multiplier(i32 type);
f32 sh_surface_friction_multiplier(i32 type);

char sh_surface_local(void);
