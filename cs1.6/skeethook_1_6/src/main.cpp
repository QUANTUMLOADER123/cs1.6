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
    console::create();

    MH_Initialize();

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
            console::log("[gold] шаг %s (%d/%d) не прошёл, повтор",
                         steps[failed].name, failed + 1, step_count);
        }

        if (++attempts >= 150)
        {
            console::log("[gold] стоп после 150 попыток на шаге %s",
                         steps[failed].name);
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    console::log("[gold] initialized");

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

        HANDLE thread = CreateThread(nullptr, 0, main_thread, nullptr, 0, nullptr);
        if (thread)
            CloseHandle(thread);
    }

    return TRUE;
}