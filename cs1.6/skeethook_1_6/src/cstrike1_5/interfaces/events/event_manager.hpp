#pragma once

#include <cstdint>

#include <cstrike1_5/common.hpp>
#include <cstrike1_5/math/vector.hpp>

namespace event_manager
{
    enum class event_type
    {
        local_hurt,
        local_health,
        local_shot,
        player_health,
        player_hurt,
        player_death,
        player_money,
        player_attrib,
        player_radar,
        item_pickup
    };

    struct event_args_t
    {
        const char* message = nullptr;
        int entity_index = 0;
        int health = 0;
        int armor = 0;
        int money = 0;
        int weapon = 0;
        int flags = 0;
        int ammo_type = 0;
        int clip = 0;
        vec3_t origin;
    };

    struct message_reader_t
    {
        const uint8_t* data = nullptr;
        int size = 0;
        int position = 0;

        auto byte() -> int
        {
            if (position + 1 > size)
            {
                position = size;
                return -1;
            }

            return data[position++];
        }

        auto dword() -> int
        {
            if (position + 4 > size)
            {
                position = size;
                return -1;
            }

            const int value = static_cast<int>(static_cast<uint32_t>(data[position]) | (static_cast<uint32_t>(data[position + 1]) << 8) | (static_cast<uint32_t>(data[position + 2]) << 16) | (static_cast<uint32_t>(data[position + 3]) << 24));
            position += 4;

            return value;
        }

        auto coord() -> float
        {
            if (position + 2 > size)
            {
                position = size;
                return -0.125f;
            }

            const short value = static_cast<short>(data[position] | (data[position + 1] << 8));
            position += 2;

            return value * 0.125f;
        }
    };

    using event_callback_t = void (*)(event_type type, const event_args_t& args);
    using message_parser_t = void (*)(message_reader_t& reader, const char* message);
    using message_begin_t = int (__cdecl*)(int, int);

    struct message_binding_t
    {
        const char* message;
        const char* pattern;
        int size;
        message_parser_t parser;
        void* address;
    };

    auto parse_damage(message_reader_t& reader, const char* message) -> void;
    auto parse_health(message_reader_t& reader, const char* message) -> void;
    auto parse_ammox(message_reader_t& reader, const char* message) -> void;
    auto parse_health_info(message_reader_t& reader, const char* message) -> void;
    auto parse_account(message_reader_t& reader, const char* message) -> void;
    auto parse_score_attrib(message_reader_t& reader, const char* message) -> void;
    auto parse_radar(message_reader_t& reader, const char* message) -> void;
    auto parse_receive_weapon(message_reader_t& reader, const char* message) -> void;

    inline message_binding_t bindings[] = {
        { "Damage", patterns::message_damage, 137, parse_damage, nullptr },
        { "Health", patterns::message_health, 69, parse_health, nullptr },
        { "AmmoX", patterns::message_ammox, 61, parse_ammox, nullptr },
        { "HealthInfo", patterns::message_health_info, 66, parse_health_info, nullptr },
        { "Account", patterns::message_account, 66, parse_account, nullptr },
        { "ScoreAttrib", patterns::message_score_attrib, 120, parse_score_attrib, nullptr },
        { "Radar", patterns::message_radar, 113, parse_radar, nullptr },
        { "ReceiveW", patterns::message_receive_weapon, 36, parse_receive_weapon, nullptr }
    };

    inline bool initialized = false;
    inline event_callback_t callback = nullptr;
    inline void* message_begin_address = nullptr;
    inline message_begin_t message_begin_original = nullptr;
    inline int health_by_player[65] = {};
    inline int clip_by_ammo[256] = {};

    auto initialize() -> bool;
    auto shutdown() -> void;
    auto subscribe(event_callback_t handler) -> void;
    auto emit(event_type type, const event_args_t& args) -> void;
    auto binding_count() -> int;
    auto resolve(const void* caller) -> message_binding_t*;
    auto __cdecl on_message_begin(int buffer, int size) -> int;
}
