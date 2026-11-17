#include "../../include/sh_features.h"

sh_msg_state_t g_msgs;

void sh_msgs_reset(void)
{
    for (u32 i = 0; i < sizeof(g_msgs); ++i)
        ((u8 *)&g_msgs)[i] = 0;
}

void sh_msg_score_attrib(i32 index, u8 attrib)
{
    if (index < 0 || index >= SH_MAX_ENTITIES)
        return;
    g_msgs.attrib[index] = attrib;
}

void sh_msg_round_time(i32 seconds)
{
    g_msgs.round_seconds = seconds;
}

i32 sh_player_is_dead(i32 index)
{
    if (index < 0 || index >= SH_MAX_ENTITIES)
        return 0;
    return (g_msgs.attrib[index] & 0x1) != 0;
}

void sh_msg_enemy_fire(i32 index)
{
    (void)index;
}
