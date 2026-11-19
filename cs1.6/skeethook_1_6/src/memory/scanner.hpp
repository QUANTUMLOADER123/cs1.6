#pragma once

#include <cstddef>
#include <cstdint>

namespace memory {
    void* get_module(const wchar_t* module_name);
    void* get_export(const void* module_base, const char* export_name);

    uint8_t* find_pattern(const wchar_t* module_name, const char* pattern);
    uint8_t* find_pattern(const wchar_t* module_name, const char* pattern, const char* mask);
    uint8_t* find_pattern(const uint8_t* region_start, size_t region_size, const uint8_t* buffer, size_t count, const char* mask = nullptr);

    size_t pattern_to_bytes(const char* pattern, uint8_t* out_buffer, char* out_mask_buffer);
    size_t bytes_to_pattern(const uint8_t* buffer, size_t count, char* out_buffer);
}