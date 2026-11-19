#include <utility>

#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/interfaces/engine/engine.hpp>
#include <cstrike1_5/interfaces/globals/globals.hpp>
#include <cstrike1_5/interfaces/cvars/engine_cvars.hpp>

#include <features/vars.hpp>
#include <features/ragebot/lag_record.hpp>

namespace lag_record
{
    auto resolve_cvars() -> void
    {
        if (!cvar_max_unlag)
            cvar_max_unlag = engine_cvars::find("sv_maxunlag");

        if (!cvar_unlag_push)
            cvar_unlag_push = engine_cvars::find("sv_unlagpush");
    }

    auto available_window() -> float
    {
        if (!vars::ragebot.backtrack)
            return 0.0f;

        client_state_t* state = engine::client_state();
        if (!state)
            return 0.0f;

        resolve_cvars();

        const float lerp = static_cast<float>(state->cmd.lerp_msec) * 0.001f;
        const float lerp_capped = lerp < 0.1f ? lerp : 0.1f;

        float max_unlag = cvar_max_unlag ? cvar_max_unlag->value : 0.2f;
        if (max_unlag <= 0.0f)
            max_unlag = 0.2f;

        const float push = cvar_unlag_push ? cvar_unlag_push->value : 0.03f;

        const float rewind_bound = max_unlag < lerp_capped ? max_unlag : lerp_capped;
        float window = rewind_bound + push;

        const float configured = vars::ragebot.backtrack_ms * 0.001f;
        if (configured > 0.0f && window > configured)
            window = configured;

        return window;
    }

    auto newest_stamp(cl_entity_t* entity) -> float
    {
        if (!entity)
            return 0.0f;

        const int latest = entity->current_position;
        const float recorded = entity->ph[latest & (max_records - 1)].animtime;
        if (recorded > 0.0f)
            return recorded;

        return entity->curstate.animtime;
    }

    auto collect(int player_index, cl_entity_t* entity, float window) -> void
    {
        if (window <= 0.0f)
        {
            record_counts[player_index] = 0;
            return;
        }

        const float reference = newest_stamp(entity);
        if (reference <= 0.0f)
        {
            record_counts[player_index] = 0;
            return;
        }

        const int latest = entity->current_position;
        int count = 0;

        for (int step = 0; step < max_records; step++)
        {
            const position_history_t& entry = entity->ph[(latest - step) & (max_records - 1)];
            if (entry.animtime <= 0.0f || entry.animtime > reference)
                break;

            if (reference - entry.animtime > window)
                break;

            if (step > 0)
            {
                const position_history_t& newer = entity->ph[(latest - step + 1) & (max_records - 1)];
                if (newer.animtime == entry.animtime)
                    continue;
            }

            record_t& record = records[player_index][count];
            record.origin = entry.origin;
            record.angles = entry.angles;
            record.simulation_time = entry.animtime;
            count++;
        }

        for (int left = 0, right = count - 1; left < right; left++, right--)
            std::swap(records[player_index][left], records[player_index][right]);

        record_counts[player_index] = count;
    }

    auto update() -> void
    {
        const int local_index = engine::local_player_index();
        const int max_clients = engine::max_clients();
        const int local_team = engine::player_team(local_index);
        const float window = available_window();

        cl_entity_t* local_entity = engine::entity(local_index);
        const float local_stamp = newest_stamp(local_entity);
        current_time = local_stamp > 0.0f ? local_stamp : static_cast<float>(globals::time());

        for (int index = 1; index <= max_clients && index < max_players; index++)
        {
            cl_entity_t* entity = engine::entity(index);
            const int team = engine::player_team(index);

            if (index == local_index || !entity || !entity->player || entity->curstate.health <= 0 || team == 0 || team == local_team)
            {
                record_counts[index] = 0;
                continue;
            }

            collect(index, entity, window);
        }
    }

    auto reference_time() -> float
    {
        return current_time;
    }

    auto record_count(int player_index) -> int
    {
        if (player_index < 0 || player_index >= max_players)
            return 0;

        return record_counts[player_index];
    }

    auto get_record(int player_index, int slot) -> const record_t&
    {
        const int count = record_count(player_index);
        if (count <= 0)
            return empty_record;

        if (slot < 0)
            return records[player_index][0];

        if (slot >= count)
            return records[player_index][count - 1];

        return records[player_index][slot];
    }
}
