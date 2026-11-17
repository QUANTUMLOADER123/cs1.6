#pragma once

#include "sh_types.h"

#define sh_demo_file 1
#define sh_demo_sound 2
#define sh_demo_cmd 3
#define sh_demo_net 4
#define sh_demo_strings 5
#define sh_demo_buffer 6
#define sh_demo_stop 0

typedef struct sh_demo_header_s
{
    char magic[8];
    i32 demo_protocol;
    i32 net_protocol;
    char mapname[260];
    char gamedir[260];
    i32 mapcrc;
    i32 dirs_offset;
} sh_demo_header_t;

typedef struct sh_demo_block_s
{
    i32 type;
    f32 time;
    i32 frame;
    i32 size;
} sh_demo_block_t;

typedef struct sh_demo_reader_s
{
    const u8 *data;
    i32 size;
    i32 pos;
    qbool valid;
    sh_demo_header_t header;
} sh_demo_reader_t;

qbool sh_demo_open(sh_demo_reader_t *r, const u8 *data, i32 size);
qbool sh_demo_next(sh_demo_reader_t *r, sh_demo_block_t *blk, const u8 **payload);
void  sh_demo_rewind(sh_demo_reader_t *r);
