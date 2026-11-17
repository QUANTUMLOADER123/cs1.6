#include "../../include/sh_demo.h"

static i32 sh_memcmp_(const u8 *a, const char *b, i32 n)
{
    for (i32 i = 0; i < n; ++i)
    {
        if ((char)a[i] != b[i])
            return (i32)(char)a[i] - (i32)b[i];
    }
    return 0;
}

static void sh_memcpy_(void *dst, const void *src, i32 n)
{
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;

    for (i32 i = 0; i < n; ++i)
        d[i] = s[i];
}

static i32 sh_memstr_(const u8 *src, i32 n, char *dst, i32 dst_size)
{
    i32 i = 0;

    while (i < n && i < dst_size - 1)
    {
        dst[i] = (char)src[i];
        if (src[i] == 0)
            break;
        ++i;
    }

    dst[i] = 0;
    return i + 1;
}

qbool sh_demo_open(sh_demo_reader_t *r, const u8 *data, i32 size)
{
    if (!r || !data || size < 8)
        return 0;

    for (u32 i = 0; i < sizeof(sh_demo_header_t); ++i)
        ((u8 *)&r->header)[i] = 0;

    r->data = data;
    r->size = size;
    r->pos = 0;
    r->valid = 0;

    if (sh_memcmp_(data, "HLDEMO", 6) != 0 && sh_memcmp_(data, "HLDEMO\0", 7) != 0)
        return 0;

    sh_memcpy_(r->header.magic, data, 8);
    r->pos = 8;

    if (r->pos + 8 > size)
        return 0;

    sh_memcpy_(&r->header.demo_protocol, data + r->pos, 4);
    r->pos += 4;
    sh_memcpy_(&r->header.net_protocol, data + r->pos, 4);
    r->pos += 4;

    if (r->pos + 260 > size)
        return 0;

    sh_memstr_(data + r->pos, 260, r->header.mapname, 260);
    r->pos += 260;

    if (r->pos + 260 > size)
        return 0;

    sh_memstr_(data + r->pos, 260, r->header.gamedir, 260);
    r->pos += 260;

    if (r->pos + 4 <= size)
    {
        sh_memcpy_(&r->header.mapcrc, data + r->pos, 4);
        r->pos += 4;
    }

    r->valid = 1;
    return 1;
}

qbool sh_demo_next(sh_demo_reader_t *r, sh_demo_block_t *blk, const u8 **payload)
{
    if (!r || !r->valid || !blk)
        return 0;

    if (r->pos + 13 > r->size)
        return 0;

    sh_memcpy_(&blk->type, r->data + r->pos, 4);
    r->pos += 4;
    sh_memcpy_(&blk->time, r->data + r->pos, 4);
    r->pos += 4;
    sh_memcpy_(&blk->frame, r->data + r->pos, 4);
    r->pos += 4;

    if (blk->type == sh_demo_stop)
    {
        r->valid = 0;
        return 1;
    }

    if (blk->type == sh_demo_buffer || blk->type == sh_demo_file)
    {
        char name[260];
        i32 used = sh_memstr_(r->data + r->pos, r->size - r->pos, name, 260);
        blk->size = used;
        if (payload)
            *payload = r->data + r->pos;
        r->pos += used;
        return 1;
    }

    if (r->pos + 4 > r->size)
        return 0;

    sh_memcpy_(&blk->size, r->data + r->pos, 4);
    r->pos += 4;

    if (blk->size < 0 || r->pos + blk->size > r->size)
        return 0;

    if (payload)
        *payload = r->data + r->pos;

    r->pos += blk->size;
    return 1;
}

void sh_demo_rewind(sh_demo_reader_t *r)
{
    if (!r)
        return;

    r->pos = 8 + 8 + 260 + 260 + 4;
    r->valid = r->data != 0;
}
