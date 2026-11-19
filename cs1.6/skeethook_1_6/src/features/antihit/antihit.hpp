#pragma once

#include <cstdint>

#include <core/hooks.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/math/vector.hpp>

struct cl_entity_t;

namespace antihit
{
    inline vec3_t real_angles{};
    inline vec3_t fake_angles{};
    inline hook camera_hook;
    inline bool jitter_side = false;
    inline float spin_angle = 0.0f;
    inline int jitter_step = 0;
    inline float swing_phase = 0.0f;
    inline unsigned int jitter_random = 2123809381u;
    inline vec3_t* view_angles_address = nullptr;
    inline vec3_t* camera_angles_address = nullptr;
    inline uintptr_t* send_packet_operand = nullptr;
    inline uintptr_t send_packet_backup = 0;
    inline unsigned long send_packet_protection = 0;
    inline double fake_lag_block = -1.0e30;
    inline int fake_lag_counter = 0;
    inline int fake_lag_good = 0;
    inline bool fake_lag_jitter = false;

    inline const char* fake_lag_type_items[] =
    {
        "dynamic",
        "maximum",
        "fluxture",
        "break lag comp",
    };

    inline const char* fake_lag_move_items[] =
    {
        "always",
        "on land",
        "on move",
        "in air",
    };

    auto initialize() -> bool;
    auto shutdown() -> void;
    auto on_create_move(float frametime, usercmd_t* cmd) -> void;
    auto fake_lag(float frametime, usercmd_t* cmd) -> void;
    auto send_packet(bool value) -> void;
    auto jitter_offset(int step, int sides, float amount) -> float;
    auto random_jitter(float amount) -> float;
    auto __cdecl on_camera_calc() -> int;
}