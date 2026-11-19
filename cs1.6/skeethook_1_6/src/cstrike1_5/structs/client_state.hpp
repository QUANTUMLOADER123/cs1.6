#pragma once

#include <cstddef>
#include <cstdint>

#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/structs/entity_state.hpp>
#include <cstrike1_5/structs/cl_entity.hpp>
#include <cstrike1_5/structs/player_info.hpp>

struct event_args_t
{
    int flags;
    int entindex;
    float origin[3];
    float angles[3];
    float velocity[3];
    int ducking;
    float fparam1;
    float fparam2;
    int iparam1;
    int iparam2;
    int bparam1;
    int bparam2;
};

struct event_info_t
{
    unsigned short index;
    short packet_index;
    short entity_index;
    float fire_time;
    event_args_t args;
    int flags;
};

struct event_state_t
{
    event_info_t ei[64];
};

struct event_t
{
    unsigned short index;
    const char* filename;
    int filesize;
    const char* pszScript;
};

struct sfx_t
{
    char name[64];
    void* cache_data;
    int servercount;
};

struct consistency_t
{
    char* filename;
    int issound;
    int orig_index;
    int value;
    int check_type;
    float mins[3];
    float maxs[3];
};

struct usercmd_t
{
    short lerp_msec;
    uint8_t msec;
    vec3_t viewangles;
    float forwardmove;
    float sidemove;
    float upmove;
    uint8_t lightlevel;
    unsigned short buttons;
    uint8_t impulse;
    uint8_t weaponselect;
    int impact_index;
    vec3_t impact_position;
};

struct screenfade_t
{
    float fadeSpeed;
    float fadeEnd;
    float fadeTotalEnd;
    float fadeReset;
    uint8_t fader;
    uint8_t fadeg;
    uint8_t fadeb;
    uint8_t fadealpha;
    int fadeFlags;
};

struct clientdata_t
{
    vec3_t origin;
    vec3_t velocity;
    int viewmodel;
    vec3_t punchangle;
    int flags;
    int waterlevel;
    int watertype;
    vec3_t view_ofs;
    float health;
    int bInDuck;
    int weapons;
    int flTimeStepSound;
    int flDuckTime;
    int flSwimTime;
    int waterjumptime;
    float maxspeed;
    float fov;
    int weaponanim;
    int m_iId;
    int ammo_shells;
    int ammo_nails;
    int ammo_cells;
    int ammo_rockets;
    float m_flNextAttack;
    int tfstate;
    int pushmsec;
    int deadflag;
    char physinfo[256];
    int iuser1;
    int iuser2;
    int iuser3;
    int iuser4;
    float fuser1;
    float fuser2;
    float fuser3;
    float fuser4;
    vec3_t vuser1;
    vec3_t vuser2;
    vec3_t vuser3;
    vec3_t vuser4;
};

struct weapon_data_t
{
    int m_iId;
    int m_iClip;
    float m_flNextPrimaryAttack;
    float m_flNextSecondaryAttack;
    float m_flTimeWeaponIdle;
    int m_fInReload;
    int m_fInSpecialReload;
    float m_flNextReload;
    float m_flPumpTime;
    float m_fReloadTime;
    float m_fAimedDamage;
    float m_fNextAimBonus;
    int m_fInZoom;
    int m_iWeaponState;
    int iuser1;
    int iuser2;
    int iuser3;
    int iuser4;
    float fuser1;
    float fuser2;
    float fuser3;
    float fuser4;
};

struct packet_entities_t
{
    int num_entities;
    uint8_t flags[32];
    void* entities;
};

struct frame_t
{
    double receivedtime;
    double latency;
    int invalid;
    int choked;
    entity_state_t playerstate[32];
    double time;
    clientdata_t clientdata;
    weapon_data_t weapondata[64];
    packet_entities_t packet_entities;
    unsigned short clientbytes;
    unsigned short playerinfobytes;
    unsigned short packetentitybytes;
    unsigned short tentitybytes;
    unsigned short soundbytes;
    unsigned short eventbytes;
    unsigned short usrbytes;
    unsigned short voicebytes;
    unsigned short msgbytes;
};

struct cmd_t
{
    usercmd_t cmd;
    float senttime;
    float receivedtime;
    float frame_lerp;
    int processedfuncs;
    int heldback;
    int sendsize;
};

struct local_state_t
{
    entity_state_t playerstate;
    clientdata_t client;
    weapon_data_t weapondata[64];
};

struct client_state_t
{
    int max_edicts;
    resource_t resourcesonhand;
    resource_t resourcesneeded;
    resource_t resourcelist[1280];
    int num_resources;
    int need_force_consistency_response;
    char serverinfo[512];
    int servercount;
    int validsequence;
    int parsecount;
    int parsecountmod;
    int stats[32];
    int weapons;
    usercmd_t cmd;
    vec3_t viewangles;
    vec3_t punchangle;
    vec3_t crosshairangle;
    vec3_t simorg;
    vec3_t simvel;
    vec3_t simangles;
    float predicted_origins[64][3];
    vec3_t prediction_error;
    float idealpitch;
    vec3_t viewheight;
    screenfade_t sf;
    int paused;
    int onground;
    int moving;
    int waterlevel;
    int usehull;
    float maxspeed;
    int pushmsec;
    int light_level;
    int intermission;
    double mtime[2];
    double time;
    double oldtime;
    frame_t frames[64];
    cmd_t commands[64];
    local_state_t predicted_frames[64];
    int delta_sequence;
    int playernum;
    event_t event_precache[256];
    void* model_precache[512];
    int model_precache_count;
    void* sound_precache[512];
    consistency_t consistency_list[512];
    int num_consistency;
    int highentity;
    char levelname[40];
    int maxclients;
    int gametype;
    int viewentity;
    void* worldmodel;
    void* free_efrags;
    int num_entities;
    int num_statics;
    cl_entity_t viewent;
    int cdtrack;
    int looptrack;
    uint32_t serverCRC;
    uint8_t clientdllmd5[16];
    float weaponstarttime;
    int weaponsequence;
    int fPrecaching;
    void* pLight;
    uint8_t reserved[4];
    player_info_t players[32];
    entity_state_t instanced_baseline[64];
    int instanced_baseline_number;
    uint32_t mapCRC;
    event_state_t events;
    char downloadUrl[128];
};

static_assert(sizeof(entity_state_t) == 340);
static_assert(sizeof(usercmd_t) == 52);
static_assert(sizeof(screenfade_t) == 24);
static_assert(sizeof(clientdata_t) == 476);
static_assert(sizeof(weapon_data_t) == 88);
static_assert(sizeof(packet_entities_t) == 40);
static_assert(sizeof(frame_t) == 17080);
static_assert(sizeof(cmd_t) == 76);
static_assert(sizeof(event_t) == 16);
static_assert(sizeof(sfx_t) == 72);
static_assert(sizeof(consistency_t) == 44);
static_assert(sizeof(event_info_t) == 88);
static_assert(offsetof(client_state_t, model_precache) == 1690800);
static_assert(offsetof(client_state_t, maxclients) == 1717476);
static_assert(offsetof(client_state_t, viewentity) == 1717484);
static_assert(offsetof(client_state_t, worldmodel) == 1717488);
static_assert(offsetof(client_state_t, viewent) == 1717504);
static_assert(offsetof(client_state_t, players) == 1720552);
