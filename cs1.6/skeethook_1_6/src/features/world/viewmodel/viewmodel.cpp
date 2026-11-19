#include <cstdio>

#include <cstrike1_5/interfaces/cvars/engine_cvars.hpp>
#include <features/vars.hpp>

#include <features/world/viewmodel/viewmodel.hpp>

namespace viewmodel
{
    auto execute_float(const char* command_name, float value) -> void
    {
        char command[64];
        snprintf(command, sizeof(command), "%s %.1f", command_name, value);
        engine_cvars::execute(command);
    }

    auto poll() -> void
    {
        static bool was_enabled = vars::viewmodel.enabled;
        static float applied_fov = vars::viewmodel.fov;
        static float applied_offset_x = vars::viewmodel.offset_x;
        static float applied_offset_y = vars::viewmodel.offset_y;
        static float applied_offset_z = vars::viewmodel.offset_z;

        if (!vars::viewmodel.enabled)
        {
            was_enabled = false;
            applied_fov = vars::viewmodel.fov;
            applied_offset_x = vars::viewmodel.offset_x;
            applied_offset_y = vars::viewmodel.offset_y;
            applied_offset_z = vars::viewmodel.offset_z;
            return;
        }

        if (!was_enabled)
        {
            was_enabled = true;
            applied_fov = vars::viewmodel.fov;
            applied_offset_x = vars::viewmodel.offset_x;
            applied_offset_y = vars::viewmodel.offset_y;
            applied_offset_z = vars::viewmodel.offset_z;
            execute_float("viewmodel_fov", applied_fov);
            execute_float("viewmodel_offset_x", applied_offset_x);
            execute_float("viewmodel_offset_y", applied_offset_y);
            execute_float("viewmodel_offset_z", applied_offset_z);
            return;
        }

        if (applied_fov != vars::viewmodel.fov)
        {
            applied_fov = vars::viewmodel.fov;
            execute_float("viewmodel_fov", applied_fov);
        }

        if (applied_offset_x != vars::viewmodel.offset_x)
        {
            applied_offset_x = vars::viewmodel.offset_x;
            execute_float("viewmodel_offset_x", applied_offset_x);
        }

        if (applied_offset_y != vars::viewmodel.offset_y)
        {
            applied_offset_y = vars::viewmodel.offset_y;
            execute_float("viewmodel_offset_y", applied_offset_y);
        }

        if (applied_offset_z != vars::viewmodel.offset_z)
        {
            applied_offset_z = vars::viewmodel.offset_z;
            execute_float("viewmodel_offset_z", applied_offset_z);
        }
    }
}