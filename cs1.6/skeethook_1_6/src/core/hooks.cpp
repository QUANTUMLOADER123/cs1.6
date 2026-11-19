#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/scanner.hpp>
#include <cstrike1_5/structs/playermove.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/structs/ref_params.hpp>
#include <cstrike1_5/interfaces/studio/studio.hpp>
#include <cstrike1_5/spread/spread.hpp>
#include <core/hooks.hpp>
#include <core/debug.hpp>
#include <features/vars.hpp>
#include <core/misc/cursor/cursor.hpp>
#include <render/render.hpp>
#include <render/menu.hpp>
#include <features/players/chams/chams.hpp>
#include <features/world/third_person/third_person.hpp>
#include <features/world/nightmode/nightmode.hpp>
#include <features/world/skybox/skybox.hpp>
#include <features/world/aspect_ratio/aspect_ratio.hpp>
#include <features/world/viewmodel/viewmodel.hpp>
#include <features/world/model_changer/model_changer.hpp>
#include <features/world/fov_changer/fov_changer.hpp>
#include <features/world/removals/removals.hpp>
#include <features/antihit/antihit.hpp>
#include <features/ragebot/ragebot.hpp>
#include <features/ragebot/nospread/nospread.hpp>
#include <features/legitbot/legitbot.hpp>
#include <features/misc/misc.hpp>
#include <features/misc/texturebug.hpp>

namespace hooks
{
    auto initialize() -> bool
    {

        if (!core_initialized)
        {
            void* gdi32_module = memory::get_module(modules::gdi32);
            if (!gdi32_module)
                return false;

            void* target = reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(gdi32_module), "SwapBuffers"));
            if (!target)
            {
                debug::print("[hook] SwapBuffers НЕ НАЙДЕН в gdi32.dll");
                return false;
            }

            swap_buffers_hook.create(target, reinterpret_cast<void*>(on_swap_buffers));
            core_initialized = true;

            debug::print("[hook] SwapBuffers перехвачен, адрес = %p", target);
            debug::print("[hook] меню рисуется именно отсюда");
        }

        if (!renderer_initialized)
        {
            void* is_third_person_address = memory::find_pattern(modules::renderer, patterns::is_third_person);
            if (is_third_person_address)
            {
                is_third_person_hook.create(is_third_person_address, reinterpret_cast<void*>(on_is_third_person));
                renderer_initialized = true;
            }
        }

        if (client_initialized)
            return true;

        void* client_module = memory::get_module(modules::client);
        if (!client_module)
            return false;

        void* create_move_address = memory::get_export(client_module, "CL_CreateMove");
        void* player_move_address = memory::get_export(client_module, "HUD_PlayerMove");
        void* post_run_cmd_address = memory::get_export(client_module, "HUD_PostRunCmd");
        void* vcalc_refdef_address = memory::get_export(client_module, "V_CalcRefdef");
        if (!create_move_address || !player_move_address || !post_run_cmd_address || !vcalc_refdef_address)
            return false;

        create_move_hook.create(create_move_address, reinterpret_cast<void*>(on_create_move));
        player_move_hook.create(player_move_address, reinterpret_cast<void*>(on_player_move));
        post_run_cmd_hook.create(post_run_cmd_address, reinterpret_cast<void*>(on_post_run_cmd));
        vcalc_refdef_hook.create(vcalc_refdef_address, reinterpret_cast<void*>(on_vcalc_refdef));
        client_initialized = true;

        return true;
    }

    auto shutdown() -> void
    {
        chams::shutdown();
        swap_buffers_hook.disable();
        create_move_hook.disable();
        player_move_hook.disable();
        post_run_cmd_hook.disable();
        vcalc_refdef_hook.disable();
        is_third_person_hook.disable();
        nightmode::build_hook.disable();
        skybox::draw_hook.disable();
        removals::flash_hook.disable();
        removals::smoke_hook.disable();
        removals::scope_overlay_hook.disable();
        removals::scope_overlay_draw_hook.disable();
        antihit::camera_hook.disable();
        antihit::shutdown();
    }

    auto player_move() -> playermove_t*
    {
        return local_player_move;
    }

    auto WINAPI on_swap_buffers(HDC device_context) -> BOOL
    {
        auto original = swap_buffers_hook.get_original<decltype(&on_swap_buffers)>();

        if (GetAsyncKeyState(VK_INSERT) & 1)
        {
            menu::opened = !menu::opened;
            debug::print("[input] INSERT -> меню %s", menu::opened ? "ОТКРЫТО" : "закрыто");
        }

        cursor::poll();

        if (GetAsyncKeyState(VK_END) & 1)
            should_unload = true;

        if (GetAsyncKeyState('V') & 1 && vars::third_person.enabled)
            vars::third_person.active = !vars::third_person.active;

        render::draw(device_context);
        studio::begin_frame();
        nightmode::poll();
        skybox::poll();
        aspect_ratio::poll();
        viewmodel::poll();
        model_changer::poll();
        fov_changer::poll();
        removals::poll();

        return original(device_context);
    }

    auto __cdecl on_create_move(float frametime, usercmd_t* cmd, int active) -> void
    {
        auto original = create_move_hook.get_original<decltype(&on_create_move)>();
        original(frametime, cmd, active);

        if (!active)
            return;

        misc::on_create_move(frametime, cmd);
        legitbot::on_create_move(frametime, cmd);
        antihit::on_create_move(frametime, cmd);
        ragebot::on_create_move(frametime, cmd);
        antihit::fake_lag(frametime, cmd);
        texturebug::on_create_move(cmd);
    }

    auto __cdecl on_player_move(playermove_t* pmove, int server) -> void
    {
        auto original = player_move_hook.get_original<decltype(&on_player_move)>();

        if (pmove)
            local_player_move = pmove;

        original(pmove, server);
    }

    auto __cdecl on_post_run_cmd(local_state_t* from, local_state_t* to, usercmd_t* cmd, int runfuncs, double time, unsigned int random_seed) -> void
    {
        auto original = post_run_cmd_hook.get_original<decltype(&on_post_run_cmd)>();

        if (to && cmd)
            spread::poll(*to, *cmd, runfuncs != 0, static_cast<float>(time), local_player_move);

        nospread::capture(random_seed);

        original(from, to, cmd, runfuncs, time, random_seed);
    }

    auto __cdecl on_vcalc_refdef(float* params) -> float*
    {
        auto view_params = reinterpret_cast<ref_params_t*>(params);
        auto original = vcalc_refdef_hook.get_original<decltype(&on_vcalc_refdef)>();

        float saved_punch[3] = {};
        if (view_params && vars::removals.toggles[removals::visual_recoil])
        {
            saved_punch[0] = view_params->punchangle.x;
            saved_punch[1] = view_params->punchangle.y;
            saved_punch[2] = view_params->punchangle.z;

            view_params->punchangle.x = 0.0f;
            view_params->punchangle.y = 0.0f;
            view_params->punchangle.z = 0.0f;
        }

        float* result = original(params);

        if (view_params && vars::removals.toggles[removals::landing_bob] && view_params->viewentity == view_params->playernum)
            view_params->vieworg.z = view_params->simorg.z + view_params->viewheight.z;

        if (view_params && vars::removals.toggles[removals::visual_recoil])
        {
            view_params->punchangle.x = saved_punch[0];
            view_params->punchangle.y = saved_punch[1];
            view_params->punchangle.z = saved_punch[2];
        }

        if (view_params)
            chams::view_forward = view_params->forward;

        if (vars::third_person.enabled)
            third_person::apply(view_params);

        return result;
    }

    auto __cdecl on_is_third_person() -> int
    {
        auto original = is_third_person_hook.get_original<decltype(&on_is_third_person)>();

        if (vars::third_person.enabled && vars::third_person.active)
            return 1;

        return original();
    }
}
