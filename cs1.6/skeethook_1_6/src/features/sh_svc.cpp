#include "../../include/sh_svc.h"

static const char *g_svc_names[sh_svc_count] = {
    "bad", "nop", "disconnect", "event", "version", "setview", "sound",
    "time", "print", "stufftext", "setangle", "serverinfo", "lightstyle",
    "updateuserinfo", "deltadescription", "clientdata", "stopsound", "pings",
    "particle", "damage", "spawnstatic", "event_reliable", "spawnbaseline",
    "temp_entity", "setpause", "signonnum", "centerprint", "killedmonster",
    "foundsecret", "spawnstaticsound", "intermission", "finale", "cdtrack",
    "restore", "cutscene", "weaponanim", "decalname", "roomtype", "addangle",
    "newusermsg", "packetentities", "deltapacketentities", "choke",
    "resourcelist", "newmovevars", "resourcerequest", "customization",
    "crosshairangle", "soundfade", "filetxferfailed", "hltv", "director",
    "voiceinit", "voicedata", "sendextrainfo", "timescale",
    "resourcelocation", "sendcvarvalue", "exec"
};

const char *sh_svc_name(i32 opcode)
{
    if (opcode < 0 || opcode >= sh_svc_count)
        return "?";

    return g_svc_names[opcode];
}

qbool sh_svc_is_entity(i32 opcode)
{
    return opcode == sh_svc_packetentities ||
           opcode == sh_svc_deltapacketentities;
}

qbool sh_svc_is_local(i32 opcode)
{
    return opcode == sh_svc_clientdata || opcode == sh_svc_damage;
}
