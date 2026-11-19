#include <cstdint>
#include <cstddef>
#include <crt/crt.h>
#include <pe32/pe32.h>
#include <memory/scanner.hpp>

#include <iostream>
#include <malloc.h>
#include <string.h>

void* memory::get_module(const wchar_t* module_name)
{
    const _PEB* peb = get_peb();

    if (!module_name)
        return peb->ImageBaseAddress;

    for (LIST_ENTRY* entry_list = peb->Ldr->InMemoryOrderModuleList.Flink;
        entry_list != &peb->Ldr->InMemoryOrderModuleList;
        entry_list = entry_list->Flink)
    {
        const _LDR_DATA_TABLE_ENTRY* entry = CONTAINING_RECORD(entry_list, _LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (entry->BaseDllName.Buffer && _wcsicmp(module_name, entry->BaseDllName.Buffer) == 0)
            return entry->DllBase;
    }

    return nullptr;
}

void* memory::get_export(const void* module_base, const char* export_name)
{
    const auto base_address = static_cast<const uint8_t*>(module_base);

    const auto dos_header = static_cast<const IMAGE_DOS_HEADER*>(module_base);
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
        return nullptr;

    const auto nt_headers = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base_address + dos_header->e_lfanew);
    if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
        return nullptr;

    const auto optional_header = &nt_headers->OptionalHeader;

    const uintptr_t export_dir_size = optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    const uintptr_t export_dir_address = optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    if (!export_dir_size || !export_dir_address)
        return nullptr;

    const auto export_dir = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base_address + export_dir_address);

    const auto names_rva = reinterpret_cast<const uint32_t*>(base_address + export_dir->AddressOfNames);
    const auto ordinals_rva = reinterpret_cast<const uint16_t*>(base_address + export_dir->AddressOfNameOrdinals);
    const auto functions_rva = reinterpret_cast<const uint32_t*>(base_address + export_dir->AddressOfFunctions);

    size_t left = 0;
    size_t right = export_dir->NumberOfNames;

    while (left != right)
    {
        const size_t middle = left + ((right - left) >> 1);
        const int result = CRT::StringCompare(export_name, reinterpret_cast<const char*>(base_address + names_rva[middle]));

        if (result == 0)
        {
            const uint32_t function_rva = functions_rva[ordinals_rva[middle]];

            if (function_rva >= export_dir_address && function_rva - export_dir_address < export_dir_size)
                return nullptr;

            return const_cast<uint8_t*>(base_address) + function_rva;
        }

        if (result > 0)
            left = middle + 1;
        else
            right = middle;
    }

    return nullptr;
}

uint8_t* memory::find_pattern(const wchar_t* module_name, const char* pattern)
{
    const size_t approx_size = (CRT::StringLength(pattern) >> 1U) + 1U;
    uint8_t* byte_buffer = static_cast<uint8_t*>(_malloca(approx_size));
    char* mask_buffer = static_cast<char*>(_malloca(approx_size));

    pattern_to_bytes(pattern, byte_buffer, mask_buffer);

    uint8_t* result = find_pattern(module_name, reinterpret_cast<const char*>(byte_buffer), mask_buffer);

    if (!result)
        std::cout << "Pattern Not Found: " << pattern << std::endl;

    _freea(mask_buffer);
    _freea(byte_buffer);

    return result;
}

uint8_t* memory::find_pattern(const wchar_t* module_name, const char* pattern, const char* mask)
{
    const void* module_base = get_module(module_name);
    if (!module_base)
        return nullptr;

    const auto base_address = static_cast<const uint8_t*>(module_base);
    const auto dos_header = static_cast<const IMAGE_DOS_HEADER*>(module_base);

    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
        return nullptr;

    const auto nt_headers = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base_address + dos_header->e_lfanew);
    if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
        return nullptr;

    const uint8_t* byte_buffer = reinterpret_cast<const uint8_t*>(pattern);
    const size_t byte_count = CRT::StringLength(mask);

    uint8_t* result = find_pattern(base_address, nt_headers->OptionalHeader.SizeOfImage, byte_buffer, byte_count, mask);

    return result;
}

uint8_t* memory::find_pattern(const uint8_t* region_start, size_t region_size, const uint8_t* buffer, size_t count, const char* mask)
{
    const uint8_t* end = region_start + region_size - count;

    for (const uint8_t* current = region_start; current <= end; ++current)
    {
        bool matched = true;

        for (size_t i = 0; i < count; i++)
        {
            if (mask[i] != '?' && current[i] != buffer[i])
            {
                matched = false;
                break;
            }
        }

        if (matched)
            return const_cast<uint8_t*>(current);
    }

    return nullptr;
}

size_t memory::pattern_to_bytes(const char* pattern, uint8_t* out_buffer, char* out_mask_buffer)
{
    uint8_t* current_byte = out_buffer;

    while (*pattern != '\0')
    {
        if (*pattern == '?')
        {
            ++pattern;
            *current_byte++ = 0U;
            *out_mask_buffer++ = '?';
        }
        else if (*pattern != ' ')
        {
            uint8_t byte = static_cast<uint8_t>(CRT::CharToHexInt(*pattern) << 4);
            ++pattern;
            byte |= static_cast<uint8_t>(CRT::CharToHexInt(*pattern));

            *current_byte++ = byte;
            *out_mask_buffer++ = 'x';
        }

        ++pattern;
    }

    *current_byte = 0U;
    *out_mask_buffer = '\0';

    return current_byte - out_buffer;
}

size_t memory::bytes_to_pattern(const uint8_t* buffer, size_t count, char* out_buffer)
{
    char* current_pattern = out_buffer;

    for (size_t i = 0U; i < count; i++)
    {
        const char* hex_byte = &CRT::_TWO_DIGITS_HEX_LUT[buffer[i] * 2U];
        *current_pattern++ = hex_byte[0];
        *current_pattern++ = hex_byte[1];
        *current_pattern++ = ' ';
    }

    *--current_pattern = '\0';

    return current_pattern - out_buffer;
}
