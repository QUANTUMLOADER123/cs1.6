#pragma once

#include "sh_types.h"
#include "sh_sdk.h"

#define sh_max_weapon_icons 32

typedef struct sh_weapon_icon_s
{
    i32 weapon_id;
    i32 spr;
    i32 frame;
    sh_spr_rect_t rect;
    qbool loaded;
    i32 attempts;
} sh_weapon_icon_t;

void sh_icons_init(void);
qbool sh_icons_draw(i32 weapon_id, f32 x, f32 y, f32 scale);
f32 sh_icons_width(i32 weapon_id, f32 scale);
f32 sh_icons_height(i32 weapon_id, f32 scale);
const char *sh_icons_name(i32 weapon_id);
