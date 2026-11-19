#pragma once

#include <cstdint>
#include <minhook.h>

#include <pe32/pe32.h>

#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/structs/playermove.hpp>

extern bool should_unload;

class hook
{
    void* target_func_ptr = nullptr;
    void* detour_func_ptr = nullptr;
    void* original_func_ptr = nullptr;

public:
    auto create(void* target, void* detour) -> void {
        target_func_ptr = target;
        detour_func_ptr = detour;

        MH_CreateHook(target_func_ptr, detour_func_ptr, &original_func_ptr);
        MH_EnableHook(target_func_ptr);
    }

    auto create(uintptr_t target_address, void* detour) -> void {
        create(reinterpret_cast<void*>(target_address), detour);
    }

    auto enabled() const -> bool {
        return target_func_ptr != nullptr && detour_func_ptr != nullptr;
    }

    auto enable() -> void {
        if (!target_func_ptr || !detour_func_ptr)
            return;

        MH_EnableHook(target_func_ptr);
    }

    auto disable() -> void {
        if (!target_func_ptr || !detour_func_ptr)
            return;

        MH_DisableHook(target_func_ptr);
    }

    template <typename T>
    auto get_original() -> T {
        return reinterpret_cast<T>(original_func_ptr);
    }

    auto remove() -> void {
        if (!target_func_ptr || !detour_func_ptr)
            return;

        MH_DisableHook(target_func_ptr);
        MH_RemoveHook(target_func_ptr);
        target_func_ptr = nullptr;
        detour_func_ptr = nullptr;
        original_func_ptr = nullptr;
    }
};

namespace hooks
{
    static bool core_initialized = false;
    static bool client_initialized = false;
    static bool renderer_initialized = false;

    inline playermove_t* local_player_move = nullptr;

    inline hook swap_buffers_hook;
    inline hook create_move_hook;
    inline hook player_move_hook;
    inline hook post_run_cmd_hook;
    inline hook vcalc_refdef_hook;
    inline hook is_third_person_hook;

    auto initialize() -> bool;
    auto shutdown() -> void;
    auto player_move() -> playermove_t*;

    auto WINAPI on_swap_buffers(HDC device_context) -> BOOL;
    auto __cdecl on_create_move(float frametime, usercmd_t* cmd, int active) -> void;
    auto __cdecl on_player_move(playermove_t* pmove, int server) -> void;
    auto __cdecl on_post_run_cmd(local_state_t* from, local_state_t* to, usercmd_t* cmd, int runfuncs, double time, unsigned int random_seed) -> void;
    auto __cdecl on_vcalc_refdef(float* params) -> float*;
    auto __cdecl on_is_third_person() -> int;
}
