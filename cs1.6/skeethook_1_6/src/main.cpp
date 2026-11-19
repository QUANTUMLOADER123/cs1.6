#include <thread>
#include <chrono>
#include <minhook.h>
#include <cstrike1_5/interfaces/engine/engine.hpp>
#include <cstrike1_5/interfaces/globals/globals.hpp>
#include <cstrike1_5/interfaces/cvars/engine_cvars.hpp>
#include <cstrike1_5/interfaces/enginefuncs/enginefuncs.hpp>
#include <cstrike1_5/interfaces/renderer/renderer.hpp>
#include <cstrike1_5/interfaces/tracers/tracers.hpp>
#include <cstrike1_5/interfaces/studio/studio.hpp>
#include <cstrike1_5/interfaces/weapons/weapons.hpp>
#include <cstrike1_5/interfaces/events/event_manager.hpp>
#include <core/hooks.hpp>
#include <features/vars.hpp>
#include <core/console.hpp>
#include <core/debug.hpp>
#include <memory/scanner.hpp>
#include <render/menu.hpp>
#include <render/render.hpp>
#include <render/sprites.hpp>
#include <features/players/chams/chams.hpp>
#include <features/world/nightmode/nightmode.hpp>
#include <features/world/skybox/skybox.hpp>
#include <features/world/aspect_ratio/aspect_ratio.hpp>
#include <features/world/fov_changer/fov_changer.hpp>
#include <features/world/removals/removals.hpp>
#include <features/world/anim_breakers/anim_breakers.hpp>
#include <features/world/bullet_tracers/bullet_tracers.hpp>
#include <features/world/killsounds/killsounds.hpp>
#include <features/antihit/antihit.hpp>
#include <features/misc/misc.hpp>

#include <iostream>

static HMODULE module_handle = nullptr;
bool should_unload = false;


DWORD WINAPI main_thread(LPVOID parameter) {
    /* СВОЯ КОНСОЛЬ. Поднимаем её ПЕРВОЙ строкой, до всего
       остального: если игра падает на инициализации, окно всё
       равно будет и в нём будет видно, на чём именно.
       Она не зависит ни от экрана игры, ни от паттернов. */
    debug::enable();

    debug::print("[main] поток запущен");

    debug::print("[main] console::create() - ищем функцию печати игры...");
    const bool game_console = console::create();
    debug::print("[main] console::create() = %s", game_console ? "OK" : "НЕ НАЙДЕНА (лог игры недоступен, это не смертельно)");

    debug::print("[main] MH_Initialize()...");
    const MH_STATUS mh = MH_Initialize();
    debug::print("[main] MH_Initialize() = %d %s", static_cast<int>(mh), mh == MH_OK ? "OK" : "ОШИБКА");

    /* Порядок инициализации - РОВНО как у автора, не меняем:
       каждый следующий шаг опирается на указатели предыдущего.

       Добавлена только диагностика. Если какой-то паттерн не
       нашёлся, в консоли будет видно НОМЕР шага, а не просто
       тишина: раньше чит молча висел в retry навсегда.
       Поведение при этом прежнее: retry каждые 200 мс, пока все
       шаги не пройдут. */
    using init_fn = bool (*)();

    static const struct
    {
        const char* name;
        init_fn fn;
    } steps[] = {
        { "engine",        &engine::initialize        },
        { "globals",       &globals::initialize       },
        { "engine_cvars",  &engine_cvars::initialize  },
        { "enginefuncs",   &enginefuncs::initialize   },
        { "renderer",      &renderer::initialize      },
        { "engine_tracers",&engine_tracers::initialize},
        { "studio",        &studio::initialize        },
        { "weapons",       &weapons::initialize       },
        { "killsounds",    &killsounds::initialize    },
        { "chams",         &chams::initialize         },
        { "nightmode",     &nightmode::initialize     },
        { "skybox",        &skybox::initialize        },
        { "aspect_ratio",  &aspect_ratio::initialize  },
        { "fov_changer",   &fov_changer::initialize   },
        { "removals",      &removals::initialize      },
        { "bullet_tracers",&bullet_tracers::initialize},
        { "sprites",       &sprites::initialize       },
        { "antihit",       &antihit::initialize       },
        { "anim_breakers", &anim_breakers::initialize },
        { "hooks",         &hooks::initialize         },
        { "event_manager", &event_manager::initialize },
        { "misc",          &misc::initialize          },
    };

    constexpr int step_count = sizeof(steps) / sizeof(steps[0]);

    debug::print("[main] какие модули видит чит:");
    {
        const wchar_t* names[] = { L"next_engine_mini.dll", L"client.dll",
                                   L"client_mini.dll", L"hw.dll", L"mp.dll", L"SDL2.dll" };
        for (const wchar_t* name : names)
        {
            void* base = memory::get_module(name);
            debug::print("    %-22ls %s", name, base ? "загружен" : "НЕ ЗАГРУЖЕН");
        }
    }

    int attempts = 0;
    int logged = -1;

    while (!should_unload)
    {
        int failed = -1;

        for (int i = 0; i < step_count; i++)
        {
            if (!steps[i].fn())
            {
                failed = i;
                break;
            }
        }

        if (failed < 0)
            break;

        if (failed != logged)
        {
            logged = failed;
            debug::print("[init] ПРОВАЛ на шаге %d/%d: %s  (повтор каждые 200 мс)",
                         failed + 1, step_count, steps[failed].name);
            console::log("[gold] шаг %s (%d/%d) не прошёл, повтор",
                         steps[failed].name, failed + 1, step_count);
        }

        if (++attempts >= 150)
        {
            debug::print("[init] СДАЮСЬ: 150 попыток, последний шаг = %s", steps[failed].name);
            console::log("[gold] стоп после 150 попыток на шаге %s",
                         steps[failed].name);
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    debug::print("[init] ВСЕ ШАГИ ПРОЙДЕНЫ");
    console::log("[gold] initialized");

    /* Главная проверка: встал ли хук SwapBuffers.
       Именно он рисует меню. Не встал - меню не будет никогда,
       сколько ни жми Insert. */
    debug::print("[check] хук SwapBuffers: %s",
                 hooks::swap_buffers_hook.enabled() ? "ВСТАЛ" : "НЕ ВСТАЛ (меню не появится!)");
    debug::print("[hint] INSERT - открыть меню, END - выгрузить чит.");

    int idle = 0;
    while (!should_unload)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (++idle % 50 == 0)
            debug::print("[state] жив, меню %s", menu::opened ? "ОТКРЫТО" : "закрыто");
    }

    while (!should_unload)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    event_manager::shutdown();
    hooks::shutdown();
    anim_breakers::shutdown();
    aspect_ratio::shutdown();
    render::shutdown();

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    chams::model_hook.remove();
    chams::light_hook.remove();
    hooks::swap_buffers_hook.remove();
    hooks::create_move_hook.remove();
    hooks::player_move_hook.remove();
    hooks::post_run_cmd_hook.remove();
    hooks::vcalc_refdef_hook.remove();
    hooks::is_third_person_hook.remove();
    nightmode::build_hook.remove();
    skybox::draw_hook.remove();
    removals::flash_hook.remove();
    removals::smoke_hook.remove();
    removals::scope_overlay_hook.remove();
    removals::scope_overlay_draw_hook.remove();
    bullet_tracers::fire_bullets_hook.remove();
    bullet_tracers::bullet_impact_hook.remove();
    antihit::camera_hook.remove();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    MH_Uninitialize();
    console::destroy();

    if (module_handle)
        FreeLibraryAndExitThread(module_handle, 0);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        module_handle = module;

        /* Консоль поднимаем прямо здесь: DllMain держит loader lock,
           поэтому тяжёлое (сканирование) оставляем в потоке, но само
           окно и одну строку "загрузились" можно сделать сразу. */
        debug::enable();
        debug::print("[dll] DLL_PROCESS_ATTACH, база = %p", static_cast<void*>(module));

        HANDLE thread = CreateThread(nullptr, 0, main_thread, nullptr, 0, nullptr);
        if (thread)
            CloseHandle(thread);
        else
            debug::print("[dll] ОШИБКА: CreateThread не удался, код %u", GetLastError());
    }

    return TRUE;
}