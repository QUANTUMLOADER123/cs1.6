#include <core/debug.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <io.h>

namespace
{
    bool console_allocated = false;
    bool console_owned = false;   /* true - консоль создали мы, а не игра */
    HANDLE output_handle = nullptr;
    CRITICAL_SECTION lock;
    bool lock_ready = false;

    auto ensure_lock() -> void
    {
        if (!lock_ready)
        {
            InitializeCriticalSection(&lock);
            lock_ready = true;
        }
    }

    /* Если инжектор запустил нас из своей консоли (cmd/powershell),
       AllocConsole не нужен - используем уже имеющуюся, но не
       закрываем её при выходе. */
    auto attach_existing() -> bool
    {
        if (GetStdHandle(STD_OUTPUT_HANDLE))
            return true;

        return AttachConsole(ATTACH_PARENT_PROCESS) != 0;
    }

    auto redirect_streams() -> void
    {
        if (!output_handle || output_handle == INVALID_HANDLE_VALUE)
            return;

        const int fd = _open_osfhandle(reinterpret_cast<intptr_t>(output_handle), _O_TEXT);
        if (fd != -1)
        {
            FILE* stream = _fdopen(fd, "w");
            if (stream)
            {
                *stdout = *stream;
                *stderr = *stream;
                setvbuf(stdout, nullptr, _IONBF, 0);
                setvbuf(stderr, nullptr, _IONBF, 0);
            }
        }
    }
}

auto debug::enable() -> bool
{
    ensure_lock();

    if (console_allocated)
        return true;

    /* 1. Уже есть консоль от родительского процесса (запуск из cmd). */
    if (attach_existing())
    {
        output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        console_owned = false;
    }
    else
    {
        /* 2. Своя консоль. */
        if (!AllocConsole())
            return false;

        output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        console_owned = true;
    }

    if (!output_handle || output_handle == INVALID_HANDLE_VALUE)
        return false;

    SetConsoleTitleA("skeethook 1.6 :: debug");

    /* Побольше буфер, чтобы история не терялась. */
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (GetConsoleScreenBufferInfo(output_handle, &info))
    {
        COORD size = info.dwSize;
        size.Y = 3000;
        SetConsoleScreenBufferSize(output_handle, size);
    }

    redirect_streams();

    console_allocated = true;

    debug::print("");
    debug::print("==================================================");
    debug::print(" skeethook 1.6 :: debug console");
    debug::print("==================================================");
    debug::print(" PID %u", static_cast<unsigned>(GetCurrentProcessId()));
    debug::print(" Процесс создан, консоль из инжектора: %s",
                 console_owned ? "нет (своя)" : "да");
    debug::print("");

    return true;
}

auto debug::is_enabled() -> bool
{
    return console_allocated;
}

auto debug::printf_impl(const char* format, void* arguments) -> void
{
    if (!console_allocated || !output_handle || output_handle == INVALID_HANDLE_VALUE)
        return;

    char buffer[2048];
    const int written = vsnprintf(buffer, sizeof(buffer) - 2, format, *static_cast<va_list*>(arguments));

    if (written < 0)
        return;

    size_t length = static_cast<size_t>(written);
    if (length > sizeof(buffer) - 2)
        length = sizeof(buffer) - 2;

    buffer[length] = '\n';
    buffer[length + 1] = '\0';

    DWORD out = 0;
    WriteConsoleA(output_handle, buffer, static_cast<DWORD>(length + 1), &out, nullptr);
}

auto debug::print(const char* format, ...) -> void
{
    if (!console_allocated)
        return;

    ensure_lock();
    EnterCriticalSection(&lock);

    va_list arguments;
    va_start(arguments, format);
    printf_impl(format, &arguments);
    va_end(arguments);

    LeaveCriticalSection(&lock);
}

auto debug::pattern_not_found(const char* module_name, const char* pattern) -> void
{
    debug::print("[pattern] НЕ НАЙДЕН в %s: %s",
                 module_name ? module_name : "?",
                 pattern ? pattern : "?");
}

auto debug::pause() -> void
{
    if (!console_allocated)
        return;

    SetConsoleTitleA("skeethook 1.6 :: debug (нажмите Enter)");
    debug::print("");
    debug::print("-----------------------------------------------");
    debug::print(" Нажмите Enter, чтобы закрыть это окно.");
    debug::print("-----------------------------------------------");

    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (input && input != INVALID_HANDLE_VALUE)
    {
        char buffer[8];
        DWORD read = 0;
        ReadConsoleA(input, buffer, sizeof(buffer) - 1, &read, nullptr);
    }
}

auto debug::close() -> void
{
    if (!console_allocated)
        return;

    console_allocated = false;

    /* Чужую консоль (от cmd) не освобождаем - её закроет инжектор. */
    if (console_owned)
    {
        FreeConsole();
        console_owned = false;
    }

    output_handle = nullptr;
}
