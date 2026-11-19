#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
};

static_assert(sizeof(_UNICODE_STRING) == 0x8);

struct _LDR_DATA_TABLE_ENTRY
{
    _LIST_ENTRY InLoadOrderLinks;
    _LIST_ENTRY InMemoryOrderLinks;
    _LIST_ENTRY InInitializationOrderLinks;
    VOID* DllBase;
    VOID* EntryPoint;
    ULONG SizeOfImage;
    _UNICODE_STRING FullDllName;
    _UNICODE_STRING BaseDllName;
};

static_assert(sizeof(_LDR_DATA_TABLE_ENTRY) == 0x34);

struct _PEB_LDR_DATA
{
    ULONG Length;
    UCHAR Initialized;
    VOID* SsHandle;
    _LIST_ENTRY InLoadOrderModuleList;
    _LIST_ENTRY InMemoryOrderModuleList;
    _LIST_ENTRY InInitializationOrderModuleList;
};

struct _PEB
{
    UCHAR InheritedAddressSpace;
    UCHAR ReadImageFileExecOptions;
    UCHAR BeingDebugged;
    UCHAR BitField;
    VOID* Mutant;
    VOID* ImageBaseAddress;
    _PEB_LDR_DATA* Ldr;
};

inline _PEB* get_peb()
{
    return reinterpret_cast<_PEB*>(__readfsdword(0x30));
}
