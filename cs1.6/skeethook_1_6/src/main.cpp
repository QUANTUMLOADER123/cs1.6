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

    while (!should_unload && !(engine::initialize() && globals::initialize() && engine_cvars::initialize() && enginefuncs::initialize() && renderer::initialize() && engine_tracers::initialize() && studio::initialize() && weapons::initialize() && killsounds::initialize() && chams::initialize() && nightmode::initialize() && skybox::initialize() && aspect_ratio::initialize() && fov_changer::initialize() && removals::initialize() && bullet_tracers::initialize() && sprites::initialize() && antihit::initialize() && anim_breakers::initialize() && hooks::initialize() && event_manager::initialize() && misc::initialize()))
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

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