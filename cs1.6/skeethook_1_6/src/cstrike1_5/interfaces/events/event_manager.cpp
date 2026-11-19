#include <cstdint>
#include <minhook.h>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/interfaces/events/event_manager.hpp>

#include <intrin.h>

auto event_manager::binding_count() -> int
{
    return static_cast<int>(sizeof(bindings) / sizeof(message_binding_t));
}

auto event_manager::resolve(const void* caller) -> message_binding_t*
{
    const auto address = reinterpret_cast<uintptr_t>(caller);

    for (int i = 0; i < binding_count(); i++)
    {
        message_binding_t& binding = bindings[i];
        if (!binding.address)
            continue;

        const auto start = reinterpret_cast<uintptr_t>(binding.address);
        if (address >= start && address < start + binding.size)
            return &binding;
    }

    return nullptr;
}

auto event_manager::initialize() -> bool
{
    if (initialized)
        return true;

    for (int i = 0; i < binding_count(); i++)
        bindings[i].address = memory::find_pattern(modules::client, bindings[i].pattern);

    void* address = memory::find_pattern(modules::client, patterns::message_begin);
    if (!address)
        return false;

    void* original = nullptr;
    if (MH_CreateHook(address, reinterpret_cast<void*>(on_message_begin), &original) != MH_OK)
        return false;

    if (MH_EnableHook(address) != MH_OK)
    {
        MH_RemoveHook(address);
        return false;
    }

    message_begin_address = address;
    message_begin_original = reinterpret_cast<message_begin_t>(original);
    initialized = true;

    return true;
}

auto event_manager::shutdown() -> void
{
    if (message_begin_address)
    {
        MH_DisableHook(message_begin_address);
        MH_RemoveHook(message_begin_address);
    }

    message_begin_address = nullptr;
    message_begin_original = nullptr;
    callback = nullptr;
    initialized = false;
}

auto event_manager::subscribe(event_callback_t handler) -> void
{
    callback = handler;
}

auto event_manager::emit(event_type type, const event_args_t& args) -> void
{
    if (!callback)
        return;

    callback(type, args);
}

auto __cdecl event_manager::on_message_begin(int buffer, int size) -> int
{
    const int result = message_begin_original(buffer, size);

    message_binding_t* binding = resolve(__builtin_return_address(0));
    if (!binding)
        return result;

    message_reader_t reader = { reinterpret_cast<const uint8_t*>(buffer), size };
    binding->parser(reader, binding->message);

    return result;
}

auto event_manager::parse_damage(message_reader_t& reader, const char* message) -> void
{
    event_args_t args;
    args.message = message;
    args.armor = reader.byte();
    args.health = reader.byte();
    args.flags = reader.dword();

    const float x = reader.coord();
    const float y = reader.coord();
    const float z = reader.coord();

    args.origin = vec3_t(x, y, z);

    emit(event_type::local_hurt, args);
}

auto event_manager::parse_health(message_reader_t& reader, const char* message) -> void
{
    event_args_t args;
    args.message = message;
    args.health = reader.byte();

    emit(event_type::local_health, args);
}

auto event_manager::parse_ammox(message_reader_t& reader, const char* message) -> void
{
    event_args_t args;
    args.message = message;
    args.ammo_type = reader.byte();
    args.clip = reader.byte();

    if (args.ammo_type < 0 || args.clip < 0)
        return;

    const int previous_clip = clip_by_ammo[args.ammo_type];
    clip_by_ammo[args.ammo_type] = args.clip;

    if (previous_clip <= 0 || args.clip >= previous_clip)
        return;

    emit(event_type::local_shot, args);
}

auto event_manager::parse_health_info(message_reader_t& reader, const char* message) -> void
{
    event_args_t args;
    args.message = message;
    args.entity_index = reader.byte();
    args.health = reader.dword();

    if (args.entity_index <= 0 || args.entity_index > 64 || args.health < 0)
        return;

    const int previous_health = health_by_player[args.entity_index];
    health_by_player[args.entity_index] = args.health;

    emit(event_type::player_health, args);

    if (args.health < previous_health)
        emit(event_type::player_hurt, args);

    if (args.health == 0 && previous_health > 0)
        emit(event_type::player_death, args);
}

auto event_manager::parse_account(message_reader_t& reader, const char* message) -> void
{
    event_args_t args;
    args.message = message;
    args.entity_index = reader.byte();
    args.money = reader.dword();

    if (args.entity_index <= 0 || args.entity_index > 64 || args.money < 0)
        return;

    emit(event_type::player_money, args);
}

auto event_manager::parse_score_attrib(message_reader_t& reader, const char* message) -> void
{
    event_args_t args;
    args.message = message;
    args.entity_index = reader.byte();
    args.flags = reader.byte();

    emit(event_type::player_attrib, args);
}

auto event_manager::parse_radar(message_reader_t& reader, const char* message) -> void
{
    event_args_t args;
    args.message = message;
    args.entity_index = reader.byte();

    const float x = reader.coord();
    const float y = reader.coord();
    const float z = reader.coord();

    args.origin = vec3_t(x, y, z);

    emit(event_type::player_radar, args);
}

auto event_manager::parse_receive_weapon(message_reader_t& reader, const char* message) -> void
{
    event_args_t args;
    args.message = message;
    args.weapon = reader.byte();

    emit(event_type::item_pickup, args);
}
