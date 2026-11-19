#include <cstdint>
#include <cstddef>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/scanner.hpp>
#include <core/console.hpp>

#include <cstdarg>
#include <cstdio>

static void (__cdecl* console_print)(const char* format, ...);

auto console::create() -> bool
{
    uint8_t* match = memory::find_pattern(modules::renderer, patterns::console_print);
    if (!match)
        return false;

    console_print = reinterpret_cast<void (__cdecl*)(const char*, ...)>(match);

    return console_print != nullptr;
}

auto console::log(const char* format, ...) -> void
{
    if (!console_print)
        return;

    va_list arguments;
    va_start(arguments, format);

    char buffer[1024];
    const int written = vsnprintf(buffer, sizeof(buffer) - 1, format, arguments);

    va_end(arguments);

    const size_t length = written < 0 ? 0 : (written > static_cast<int>(sizeof(buffer) - 2) ? sizeof(buffer) - 2 : static_cast<size_t>(written));
    buffer[length] = '\n';
    buffer[length + 1] = '\0';

    console_print(buffer);
}

auto console::destroy() -> void
{
    console_print = nullptr;
}