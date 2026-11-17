#include "../../include/sh_scanner.h"

#include <windows.h>
#include <string.h>

qbool sh_str_eq(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (*a != *b)
            return false;
        ++a;
        ++b;
    }
    return *a == *b;
}

void *sh_module_base(const char *module)
{
    HMODULE h = GetModuleHandleA(module);
    return (void *)h;
}

u32 sh_module_size(const char *module)
{
    HMODULE h = (HMODULE)sh_module_base(module);
    if (!h)
        return 0;

    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)h;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((u8 *)h + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    return nt->OptionalHeader.SizeOfImage;
}

static qbool sh_byte_match(const u8 *p, const u8 *bytes, const char *mask, i32 len)
{
    for (i32 i = 0; i < len; ++i)
    {
        if (mask[i] == '?')
            continue;
        if (p[i] != bytes[i])
            return false;
    }
    return true;
}

void *sh_find_pattern(void *base, u32 size, const sh_pattern_t *pattern)
{
    if (!base || !size || !pattern || !pattern->bytes || !pattern->mask)
        return 0;

    u8 *start = (u8 *)base;
    u8 *end = start + size - pattern->len;
    u8 first = pattern->bytes[0];

    for (u8 *p = start; p < end; ++p)
    {
        if (pattern->mask[0] != '?' && *p != first)
            continue;
        if (sh_byte_match(p, pattern->bytes, pattern->mask, pattern->len))
            return p;
    }
    return 0;
}

void *sh_find_pattern_in(const char *module, const sh_pattern_t *pattern)
{
    void *base = sh_module_base(module);
    u32 size = sh_module_size(module);
    if (!base || !size)
        return 0;
    return sh_find_pattern(base, size, pattern);
}

void *sh_pattern_call_target(void *at)
{
    if (!at)
        return 0;
    i32 rel = *(i32 *)((u8 *)at + 1);
    return (u8 *)at + 5 + rel;
}

void *sh_pattern_imm32(void *at)
{
    if (!at)
        return 0;
    return (void *)*(u32 *)at;
}

i32 sh_pattern_rel32(void *at)
{
    if (!at)
        return 0;
    return *(i32 *)at;
}
