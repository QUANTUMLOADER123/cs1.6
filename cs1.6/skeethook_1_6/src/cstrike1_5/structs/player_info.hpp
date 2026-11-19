#pragma once

#include <cstddef>
#include <cstdint>

#include <cstrike1_5/math/vector.hpp>

struct resource_t
{
    char szFileName[64];
    int type;
    int nIndex;
    int nDownloadSize;
    uint8_t ucFlags;
    uint8_t rgucMD5_hash[16];
    uint8_t playernum;
    uint8_t rguc_reserved[32];
    void* pNext;
    void* pPrev;
};

struct customization_t
{
    int bInUse;
    resource_t resource;
    int bTranslated;
    int nUserData1;
    int nUserData2;
    void* pInfo;
    void* pBuffer;
    void* pNext;
};

#pragma pack(push, 4)

struct player_info_t
{
    int userid;
    char userinfo[256];
    char name[32];
    int spectator;
    int ping;
    int packet_loss;
    char model[64];
    int topcolor;
    int bottomcolor;
    int renderframe;
    int gaitsequence;
    float gaitframe;
    float gaityaw;
    vec3_t prevgaitorigin;
    customization_t customdata;
    char hashedcdkey[16];
    uint64_t steamid;
};

#pragma pack(pop)

static_assert(sizeof(resource_t) == 136);
static_assert(sizeof(player_info_t) == 592);
static_assert(offsetof(player_info_t, userinfo) == 4);
static_assert(offsetof(player_info_t, name) == 260);
static_assert(offsetof(player_info_t, customdata) == 404);
