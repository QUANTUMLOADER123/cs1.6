#pragma once

#include <cstddef>
#include <cstdint>

#include <cstrike1_5/math/vector.hpp>

struct clipnode_t
{
    int32_t planenum;
    int16_t children[2];
};

static_assert(sizeof(clipnode_t) == 8);

struct mplane_t
{
    vec3_t normal;
    float dist;
    uint8_t type;
    uint8_t signbits;
    uint8_t pad[2];
};

static_assert(sizeof(mplane_t) == 20);

struct hull_t
{
    clipnode_t* clipnodes;
    mplane_t* planes;
    int32_t firstclipnode;
    int32_t lastclipnode;
    vec3_t clip_mins;
    vec3_t clip_maxs;
};

static_assert(sizeof(hull_t) == 40);
static_assert(offsetof(hull_t, firstclipnode) == 8);
static_assert(offsetof(hull_t, clip_mins) == 16);
static_assert(offsetof(hull_t, clip_maxs) == 28);

struct model_t
{
    char name[64];
    int32_t needload;
    int32_t type;
    int32_t numframes;
    int32_t synctype;
    int32_t flags;
    vec3_t mins;
    vec3_t maxs;
    float radius;
    int32_t firstmodelsurface;
    int32_t nummodelsurfaces;
    int32_t numsubmodels;
    void* submodels;
    int32_t numplanes;
    mplane_t* planes;
    int32_t numleafs;
    void* leafs;
    int32_t numvertexes;
    void* vertexes;
    int32_t numedges;
    void* edges;
    int32_t numnodes;
    void* nodes;
    int32_t numtexinfo;
    void* texinfo;
    int32_t numsurfaces;
    void* surfaces;
    int32_t numsurfedges;
    void* surfedges;
    int32_t numclipnodes;
    clipnode_t* clipnodes;
    int32_t nummarksurfaces;
    void* marksurfaces;
    hull_t hulls[4];
    int32_t numtextures;
    void** textures;
    void* visdata;
    void* lightdata;
    char* entities;
    void* cache;
};

static_assert(offsetof(model_t, hulls) == 208);
