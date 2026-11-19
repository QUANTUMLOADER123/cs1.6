#pragma once

#include <cstdint>

struct cvar_t
{
    const char* name;
    const char* string;
    int32_t flags;
    float value;
    cvar_t* next;
};