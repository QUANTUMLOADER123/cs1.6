#include "../../include/sh_math.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_features.h"

#include <math.h>

f32 sh_deg(f32 rad) { return rad * sh_rad2deg; }
f32 sh_rad(f32 deg) { return deg * sh_deg2rad; }

f32 sh_clampf(f32 v, f32 lo, f32 hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

i32 sh_clampi(i32 v, i32 lo, i32 hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

f32 sh_len2d(const vec3_t *v) { return sqrtf(v->x * v->x + v->y * v->y); }
f32 sh_len3d(const vec3_t *v) { return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z); }

f32 sh_dot(const vec3_t *a, const vec3_t *b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

void sh_sub(vec3_t *out, const vec3_t *a, const vec3_t *b)
{
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
}

void sh_add(vec3_t *out, const vec3_t *a, const vec3_t *b)
{
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
}

void sh_scale(vec3_t *out, const vec3_t *v, f32 s)
{
    out->x = v->x * s;
    out->y = v->y * s;
    out->z = v->z * s;
}

void sh_normalize(vec3_t *v)
{
    f32 len = sh_len3d(v);
    if (len == 0.0f)
    {
        v->x = v->y = v->z = 0.0f;
        return;
    }
    f32 inv = 1.0f / len;
    v->x *= inv;
    v->y *= inv;
    v->z *= inv;
}

void sh_cross(vec3_t *out, const vec3_t *a, const vec3_t *b)
{
    vec3_t t;
    t.x = a->y * b->z - a->z * b->y;
    t.y = a->z * b->x - a->x * b->z;
    t.z = a->x * b->y - a->y * b->x;
    *out = t;
}

void sh_angle_vectors(const vec3_t *angles, vec3_t *forward, vec3_t *right, vec3_t *up)
{
    f32 sp = sinf(sh_rad(angles->x));
    f32 cp = cosf(sh_rad(angles->x));
    f32 sy = sinf(sh_rad(angles->y));
    f32 cy = cosf(sh_rad(angles->y));
    f32 sr = sinf(sh_rad(angles->z));
    f32 cr = cosf(sh_rad(angles->z));

    if (forward)
    {
        forward->x = cp * cy;
        forward->y = cp * sy;
        forward->z = -sp;
    }
    if (right)
    {
        right->x = -1.0f * sr * sp * cy + -1.0f * cr * -sy;
        right->y = -1.0f * sr * sp * sy + -1.0f * cr * cy;
        right->z = -1.0f * sr * cp;
    }
    if (up)
    {
        up->x = cr * sp * cy + -sr * -sy;
        up->y = cr * sp * sy + -sr * cy;
        up->z = cr * cp;
    }
}

void sh_vectors_angle(const vec3_t *forward, vec3_t *angles)
{
    f32 yaw;
    f32 pitch;

    if (forward->x == 0.0f && forward->y == 0.0f)
    {
        yaw = 0.0f;
        pitch = (forward->z > 0.0f) ? 90.0f : -90.0f;
    }
    else
    {
        yaw = sh_deg(atan2f(forward->y, forward->x));
        if (yaw < 0.0f) yaw += 360.0f;
        f32 hyp = sqrtf(forward->x * forward->x + forward->y * forward->y);
        pitch = sh_deg(atan2f(-forward->z, hyp));
        if (pitch > 90.0f) pitch = 90.0f;
        if (pitch < -90.0f) pitch = -90.0f;
    }

    angles->x = pitch;
    angles->y = yaw;
    angles->z = 0.0f;
}

f32 sh_angle_delta(f32 a, f32 b)
{
    f32 d = a - b;
    while (d > 180.0f) d -= 360.0f;
    while (d < -180.0f) d += 360.0f;
    return d;
}

void sh_angle_normalize(vec3_t *angles)
{
    for (i32 i = 0; i < 3; ++i)
    {
        f32 *v = &((f32 *)angles)[i];
        while (*v > 180.0f) *v -= 360.0f;
        while (*v < -180.0f) *v += 360.0f;
    }
}

f32 sh_normalize_yaw(f32 y)
{
    while (y > 180.0f) y -= 360.0f;
    while (y < -180.0f) y += 360.0f;
    return y;
}

qbool sh_world_to_screen(const vec3_t *origin, vec2_t *out)
{
    if (sh_matrix_ready() && sh_matrix_world_to_screen(origin, out))
        return true;

    if (g_tri_api && g_tri_api->world_to_screen)
    {
        f32 scr[2];
        if (!g_tri_api->world_to_screen((f32 *)origin, scr))
            return false;
        out->x = scr[0];
        out->y = scr[1];
        return true;
    }

    if (!g_globals)
        return false;

    sh_player_t *me = sh_player_local();

    if (!me)
        return false;

    vec3_t eye = me->origin;

    vec3_t delta;
    sh_sub(&delta, origin, &eye);

    f32 fwd = sh_dot(&delta, (const vec3_t *)&g_globals->v_forward);
    if (fwd < 0.01f)
        return false;

    i32 sw = 0, sh = 0;
    if (g_engfuncs && g_engfuncs->pfnGetScreenInfo)
    {
        sh_screeninfo_t info;
        if (g_engfuncs->pfnGetScreenInfo(&info))
        {
            sw = info.width;
            sh = info.height;
        }
    }
    if (!sw || !sh)
    {
        sw = 640;
        sh = 480;
    }

    f32 right = sh_dot(&delta, (const vec3_t *)&g_globals->v_right);
    f32 up = sh_dot(&delta, (const vec3_t *)&g_globals->v_up);

    f32 fov = 90.0f;
    f32 half_x = tanf(sh_rad(fov) * 0.5f);
    f32 aspect = (f32)sw / (f32)sh;
    f32 half_y = half_x / aspect;

    f32 nx = right / (fwd * half_x);
    f32 ny = up / (fwd * half_y);

    out->x = (nx + 1.0f) * 0.5f * (f32)sw;
    out->y = (1.0f - ny) * 0.5f * (f32)sh;
    return true;
}

f32 sh_sqrt(f32 v)
{
    if (v <= 0.0f)
        return 0.0f;

    f32 x = v;
    f32 y = 1.0f;

    for (i32 i = 0; i < 24; ++i)
        y = (y + x / y) * 0.5f;

    return y;
}

f32 sh_sin(f32 rad)
{
    while (rad > sh_pi)  rad -= 2.0f * sh_pi;
    while (rad < -sh_pi) rad += 2.0f * sh_pi;

    f32 t = rad;
    f32 s = rad;
    f32 t2 = rad * rad;

    for (i32 i = 1; i < 9; ++i)
    {
        t *= -t2 / ((f32)(2 * i) * (f32)(2 * i + 1));
        s += t;
    }

    return s;
}

f32 sh_cos(f32 rad)
{
    return sh_sin(rad + sh_pi * 0.5f);
}

f32 sh_atan(f32 v)
{
    f32 ax = (v < 0.0f) ? -v : v;
    f32 s = 0.0f;
    f32 x = v;

    if (ax > 1.0f)
    {
        x = 1.0f / v;
        s = (v > 0.0f) ? (sh_pi * 0.5f) : (-sh_pi * 0.5f);
    }

    x = x / (1.0f + sh_sqrt(1.0f + x * x));
    x = x / (1.0f + sh_sqrt(1.0f + x * x));

    f32 x2 = x * x;
    f32 t = x;
    f32 r = 0.0f;

    for (i32 i = 1; i < 40; i += 2)
    {
        f32 term = t / (f32)i;
        r += ((i / 2) % 2 == 0) ? term : -term;
        t *= x2;
    }

    r *= 4.0f;

    return (ax > 1.0f) ? (s - r) : r;
}

f32 sh_atan2(f32 y, f32 x)
{
    if (x == 0.0f)
        return (y > 0.0f) ? (sh_pi * 0.5f) : ((y < 0.0f) ? (-sh_pi * 0.5f) : 0.0f);

    f32 a = sh_atan(y / x);

    return (x < 0.0f) ? (a + ((y >= 0.0f) ? sh_pi : -sh_pi)) : a;
}

f32 sh_acos(f32 v)
{
    if (v >= 1.0f)
        return 0.0f;
    if (v <= -1.0f)
        return sh_pi;

    f32 ax = (v < 0.0f) ? -v : v;
    f32 r = sh_sqrt(1.0f - ax);

    f32 a = -0.0187293f;
    a = a * ax + 0.0742610f;
    a = a * ax - 0.2121144f;
    a = a * ax + 1.5707288f;
    a = a * r;

    return (v < 0.0f) ? (sh_pi - a) : a;
}
