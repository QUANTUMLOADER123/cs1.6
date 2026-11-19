#pragma once

#include <cstdint>
#include <cstddef>

/* ДЕБАГ-КОНСОЛЬ.

   Отдельное окно Windows (AllocConsole), не связанное с консолью
   игры. Появляется сразу при инжекте и живёт независимо от того,
   работает ли меню, найден ли паттерн и т.д.

   Зачем: console::log пишет через функцию игры, найденную по
   паттерну. Если паттерн не нашёлся - console_print == nullptr, и
   log() молча ничего не делает. В итоге при поломке не видно НИ
   ОДНОГО сообщения: снаружи выглядит как "DLL загрузилась и умерла".

   Плюс std::cout из scanner.cpp ("Pattern Not Found: ...") уходил в
   никуда: подсистема GUI, stdout не привязан ни к чему.
   debug::enable() перенаправляет stdout/stderr в наше окно. */

namespace debug
{
    /* Выделяет консоль и перенаправляет stdout/stderr в неё.
       Можно звать несколько раз - повторный вызов ничего не делает. */
    auto enable() -> bool;

    /* Печать в окно. Работает без std::cout и без экрана игры. */
    auto print(const char* format, ...) -> void;

    auto printf_impl(const char* format, void* arguments) -> void;

    /* Для scanner.cpp: пишет "Pattern Not Found" прямо в окно. */
    auto pattern_not_found(const char* module_name, const char* pattern) -> void;

    auto is_enabled() -> bool;

    /* Ждём Enter, чтобы окно не закрылось мгновенно в конце работы. */
    auto pause() -> void;

    auto close() -> void;
}
