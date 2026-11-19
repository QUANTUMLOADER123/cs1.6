#include <cstdint>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/address.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/structs/cvar.hpp>
#include <features/vars.hpp>

#include <cstrike1_5/interfaces/cvars/engine_cvars.hpp>

static cvar_t* (__cdecl* find_cvar)(const char* name);
static void (__cdecl* execute_command)(const char* text, int source);

auto engine_cvars::initialize() -> bool
{
    uint8_t* findvar_match = memory::find_pattern(modules::engine, patterns::cvar_findvar);
    if (!findvar_match)
        return false;

    uint8_t* execute_match = memory::find_pattern(modules::renderer, patterns::execute_command);
    if (!execute_match)
        return false;

    find_cvar = *reinterpret_cast<cvar_t* (__cdecl**)(const char*)>(memory::global_address(findvar_match, 1));
    execute_command = reinterpret_cast<void (__cdecl*)(const char*, int)>(execute_match);

    return execute_command != nullptr;
}

auto engine_cvars::find(const char* name) -> cvar_t*
{
    return find_cvar ? find_cvar(name) : nullptr;
}

auto engine_cvars::get_float(const char* name) -> float
{
    cvar_t* variable = find(name);
    return variable ? variable->value : 0.0f;
}

auto engine_cvars::get_int(const char* name) -> int
{
    cvar_t* variable = find(name);
    return variable ? static_cast<int>(variable->value) : 0;
}

auto engine_cvars::get_string(const char* name) -> const char*
{
    cvar_t* variable = find(name);
    return variable ? variable->string : "";
}

auto engine_cvars::execute(const char* command) -> void
{
    if (!execute_command || !command || !*command)
        return;

    execute_command(command, 1);
}