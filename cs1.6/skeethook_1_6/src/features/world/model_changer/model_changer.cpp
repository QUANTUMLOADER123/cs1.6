#include <cstddef>
#include <crt/crt.h>
#include <cstrike1_5/structs/cl_entity.hpp>
#include <cstrike1_5/structs/player_info.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/structs/bsp.hpp>
#include <cstrike1_5/interfaces/engine/engine.hpp>
#include <features/vars.hpp>

#include <features/world/model_changer/model_changer.hpp>

namespace model_changer
{
    static auto find_precache(const char* name) -> int
    {
        client_state_t* state = engine::client_state();
        if (!state || !name || !name[0])
            return 0;

        for (int index = 1; index < state->model_precache_count; index++)
        {
            model_t* model = reinterpret_cast<model_t*>(state->model_precache[index]);
            if (!model || !model->name[0])
                continue;

            if (CRT::StringString(model->name, name))
                return index;
        }

        return 0;
    }

    static auto extract_name(const char* path, char* out, size_t size) -> bool
    {
        if (!path || !path[0] || size == 0)
            return false;

        const char* begin = path;
        const char* end = path + CRT::StringLength(path);

        if (*begin == '"')
            begin++;
        if (end > begin && end[-1] == '"')
            end--;

        const char* slash = begin;
        for (const char* cursor = begin; cursor < end; cursor++)
        {
            if (*cursor == '\\' || *cursor == '/')
                slash = cursor + 1;
        }

        size_t count = static_cast<size_t>(end - slash);
        if (count >= 4 && slash[count - 4] == '.' && (slash[count - 3] == 'm' || slash[count - 3] == 'M') && (slash[count - 2] == 'd' || slash[count - 2] == 'D') && (slash[count - 1] == 'l' || slash[count - 1] == 'L'))
            count -= 4;

        if (count == 0 || count >= size)
            return false;

        CRT::StringCopyN(out, slash, count);
        out[count] = '\0';
        return out[0] != '\0';
    }

    static auto apply_model(cl_entity_t* entity, player_info_t* info, const char* name) -> void
    {
        CRT::StringCopyN(info->model, name, sizeof(info->model) - 1);
        info->model[sizeof(info->model) - 1] = '\0';

        const int model_index = find_precache(name);
        if (model_index <= 0)
            return;

        entity->curstate.modelindex = model_index;
        entity->model = engine::client_state()->model_precache[model_index];
    }

    auto poll() -> void
    {
        if (!vars::model_changer.enabled)
            return;

        const int local_index = engine::local_player_index();
        cl_entity_t* entity = engine::entity(local_index);
        player_info_t* info = engine::player_info(local_index);
        if (!entity || !info)
            return;

        if (vars::model_changer.type == 1)
        {
            char name[64] = {};
            if (!extract_name(vars::model_changer.path, name, sizeof(name)))
                return;

            apply_model(entity, info, name);
            return;
        }

        const int selected = vars::model_changer.model;
        if (selected < 0 || selected >= 11)
            return;

        apply_model(entity, info, model_names[selected]);
    }
}
