#pragma once

#include <cstdint>

namespace memory
{
    template <typename type = uint8_t>
    inline type* absolute(type* relative_address, int pre_offset = 0x0, int post_offset = 0x0)
    {
        if (relative_address)
        {
            relative_address += pre_offset;
            relative_address += sizeof(int32_t) + *reinterpret_cast<int32_t*>(relative_address);
            relative_address += post_offset;
        }

        return relative_address;
    }

    inline uint8_t* global_address(uint8_t* match, int displacement)
    {
        if (!match)
            return nullptr;

        return reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(*reinterpret_cast<uint32_t*>(match + displacement)));
    }

    inline uint8_t* relative(uint8_t* address, uint32_t rva_offset, uint32_t rip_offset)
    {
        if (!address)
            return nullptr;

        const uint32_t rva = *reinterpret_cast<uint32_t*>(address + rva_offset);
        const uintptr_t rip = reinterpret_cast<uintptr_t>(address) + rip_offset;

        return reinterpret_cast<uint8_t*>(rip + rva);
    }

    template <typename type>
    static type get_vfunc(void* thisptr, uintptr_t index)
    {
        return reinterpret_cast<type>((*static_cast<uintptr_t**>(thisptr))[index]);
    }

    template <typename type, typename... args>
    type call_vfunc(void* instance, unsigned int index, args... parameters)
    {
        auto func = get_vfunc<type(__thiscall*)(void*, args...)>(instance, index);
        return func(instance, parameters...);
    }
}
