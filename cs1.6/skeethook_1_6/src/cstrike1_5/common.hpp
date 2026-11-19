#pragma once

namespace patterns
{
    constexpr auto client = "8B 0D ? ? ? ? 56 8B 81 B4 CC 19 00 89 81 F0 34 1A 00";
    constexpr auto entities = "8B 0D ? ? ? ? 83 39 00 75 1C E8 ? ? ? ?";
    constexpr auto world_transform = "55 8B EC 51 8B 45 08 8B 4D 0C D9 05 ? ? ? ? D8 08 D9 05 ? ? ? ? D8 48 08 DE C1 D9 05 ? ? ? ?";
    constexpr auto screen = "8B 3D ? ? ? ? A1 ? ? ? ? 8B 15 ? ? ? ? 8B F3 2B 35 ? ? ? ?";
    constexpr auto viewport = "8B 0D ? ? ? ? 83 C4 04 52 8B 15 ? ? ? ?";

    constexpr auto bone_globals = "55 8B EC 83 EC 48 A1 ? ? ? ? 8B 0D ? ? ? ? 53 56 8B 90 DC 02 00 00 8B B1 A4 00 00 00 3B D6 57 7C 0A";
    constexpr auto bone_matrices = "B8 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 B8 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 33 C0 C3";

    constexpr auto chams_model = "55 8B EC 83 EC 48 A1 ? ? ? ? 8B 0D ? ? ? ? 53 56 8B 70 54 8B 40 60 03 C1 57 89 45 EC";
    constexpr auto chams_light = "55 8B EC 83 EC 24 8B 0D ? ? ? ? 56 57 33 FF 3B CF";
    constexpr auto chams_alpha = "55 8B EC A1 ? ? ? ? 8B 4D 08 50 89 88 FC 02 00 00 E8 ? ? ? ? 89 45 08 83 C4 04 DB 45 08 D9 1D ? ? ? ? D9 05 ? ? ? ? DC 0D ? ? ? ? D9 1D ? ? ? ?";

    constexpr auto local_weapon = "8B 0D ? ? ? ? 8B 6C 24 18 89 2D ? ? ? ?";

    constexpr auto engine_funcs = "BF ? ? ? ? F3 A5 74 0C 5F 5E";

    constexpr auto studio_draw_player = "8B 0D ? ? ? ? 50 6A 03 FF 51 08";

    constexpr auto is_third_person = "A1 ? ? ? ? 85 C0 74 02 FF E0 33 C0 C3 90 90 55 8B EC";

    constexpr auto build_lightmap = "55 8B EC 83 EC 1C D9 05 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 44";

    constexpr auto lightmap_format = "A1 ? ? ? ? 2D 08 19 00 00";

    constexpr auto rebuild_lightmaps = "55 8B EC 51 53 56 57 68 00 80 00 00 6A 00 68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 83 C4 0C BF 01 00 00 00";
    constexpr auto cvar_findvar = "A1 ? ? ? ? 68 ? ? ? ? FF D0 83 C4 04";
    constexpr auto execute_command = "55 8B EC 8B 45 0C 8B 4D 08 6A 01 50 51 E8 0E FE";
    constexpr auto console_print = "55 8B EC B8 00 10 00 00 E8 43 B2 0D 00 8B 4D 08 8D 45 0C";
    constexpr auto fov_scaled_write = "F3 0F 11 05 ? ? ? ? 8B 8C 24";

    constexpr auto draw_skybox = "55 8B EC 83 EC 1C A1 ? ? ? ? 53 56 BB 00 00 80 3F 57 C7 45 F8 00 00 00 00";
    constexpr auto load_skys = "55 8B EC 83 EC 6C A1 ? ? ? ? 56 57 33 FF 3B C7 89 7D F4 75 25";
    constexpr auto load_skys_name = "50 68 ? ? ? ? 68 ? ? ? ? 6A 40 51 33 FF";
    constexpr auto environment_flag = "A1 ? ? ? ? 53 56 BB 00 00 80 3F 57";
    constexpr auto environment_color = "74 33 D9 05 ? ? ? ? D8 0D ? ? ? ?";

    constexpr auto camera_frustum = "DD 45 08 DC 0D ?? ?? ?? ?? 83 EC 08 DC 35 ?? ?? ?? ?? DD 1C 24 E8 ?? ?? ?? ?? DC 4D 18 8B 45 24 8B 4D 20 8B 55 1C 83 C4 08 DD 5D 08 DD 45 08 D9 E0 DD 5D F8 DD 45 F8 DC 4D 10 50 8B 45 18 51 8B 4D 0C 52 8B 55 08 DD 5D F0 DD 45 08 DC 4D 10 50 8B 45 FC 51 8B 4D F8 52 50 DD 5D 10 8B 55 14 8B 45 10 51 8B 4D F4 52 8B 55 F0 50 51 52 FF 15 ?? ?? ?? ??";

    constexpr auto fire_bullets = "81 EC 08 01 00 00 B8 28 00 00 00 53 8B 9C 24 34 01 00 00 89 44 24 18 89 44 24 1C 89 44 24 20 8B 84 24 40 01 00 00 C7 44 24 24 CD CC CC 3E 89 44 24 34 8D 43 FF 83 F8 0E";
    constexpr auto bullet_impact = "83 EC 18 8B 0D ?? ?? ?? ?? 56 8B 74 24 20 8B 46 30 50 FF 51 24 83 C4 04 85 C0 0F 84 ?? ?? ?? ?? 83 78 60 04 0F 85 ?? ?? ?? ??";

    constexpr auto killfeed_saytext = "83 EC 24 53 8B 44 24 30 55 56 57 8B 79 10 83 CF 01 50 89 79 10 8B 4C 24 44 51 E8";
    constexpr auto no_flash = "55 8B EC 83 EC ? A1 ? ? ? ? 33 C5 89 45 ? 53 8B 5D ? 56 57 8B F9 89 5D ? B9 ? ? ? ? E8 ? ? ? ? 8B 47";
    constexpr auto no_smoke = "51 A1 ? ? ? ? 53 33 DB";
    constexpr auto no_scope = "83 EC ? A1 ? ? ? ? 89 4C 24 ? 85 C0 74";
    constexpr auto no_scope_overlay = "55 8B EC A1 ? ? ? ? 85 C0 74 ? 8B 0D ? ? ? ? 8B 15";
    constexpr auto scoped_fov = "83 3D ? ? ? ? 5A";
    constexpr auto team_names = "8D 0C 85 ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 83 C4 0C";
    constexpr auto antihit_camera_hook = "55 8B EC 83 EC 40 8D 45 C0 8D 4D D8 50 8D 55 E4 51 52 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 33 C0 D9 05 ? ? ? ? D8 4C 05 E4 83 C0 04 83 F8 0C";
    constexpr auto antihit_camera_angles = "89 35 ? ? ? ? 89 35 ? ? ? ? 89 35 ? ? ? ? E8 ? ? ? ? E8";
    constexpr auto antihit_view_angles = "8B 15 ? ? ? ? 8B 0D ? ? ? ? A3 ? ? ? ? A1 ? ? ? ? 89 15 ? ? ? ? 3B C3";
    constexpr auto fake_lag_global_time = "75 13 D9 05 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 ? ? ? D9 05 ? ? ? ? DC 1D ? ? ? ? DF E0 F6 C4 41";

    constexpr auto message_damage = "8B 44 24 08 8B 4C 24 0C 83 EC 0C 53 55 56 57 50 51 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 8B E8 E8 ? ? ? ? 8B F8 E8";
    constexpr auto message_health = "8B 44 24 08 8B 4C 24 0C 50 51 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 8B 15 ? ? ? ? 8B 0D ? ? ? ? 83 CA 01 3B C1";
    constexpr auto message_health_info = "8B 44 24 08 8B 4C 24 0C 56 50 51 E8 ? ? ? ? 83 C4 08 E8 18 04 ? ? 8B F0 E8 ? ? ? ? 85 F6 7E 18 83 FE 40 7F 13";
    constexpr auto message_account = "8B 44 24 08 8B 4C 24 0C 56 50 51 E8 ? ? ? ? 83 C4 08 E8 68 04 ? ? 8B F0 E8 ? ? ? ? 85 F6 7E 18 83 FE 40 7F 13";
    constexpr auto message_score_attrib = "8B 44 24 08 8B 4C 24 0C 56 50 51 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 8B F0 E8 ? ? ? ? 66 85 F6 7E 4D 66 83 FE 40";
    constexpr auto message_radar = "83 EC 0C 8B 44 24 14 8B 4C 24 18 56 50 51 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 8B F0 E8 ? ? ? ? D9 5C 24 18";
    constexpr auto message_receive_weapon = "8B 44 24 08 8B 4C 24 0C 50 51 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? A3 ? ? ? ? B8 01 00 00 00 C2 0C 00";

    constexpr auto message_ammox = "8B 44 24 08 8B 4C 24 0C 56 50 51 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 8B F0 E8 ? ? ? ? 8B 0D ? ? ? ? 99 33 C2 2B C2 89 04 B5 ? ? ? ? 8B 11 FF 52 04 B8 01 00 00 00 5E C3 90";
    constexpr auto message_begin = "8B 4C 24 04 33 C0 A3 ? ? ? ? A3 ? ? ? ? 8B 44 24 08 89 0D ? ? ? ? A3 ? ? ? ? C3";

    constexpr auto user_tracer = "55 8B EC 8B 4D ?? 85 C9 7D ?? 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 5D C3 83 F9 0C 76 ?? 6A 0C 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 5D C3 A1 ? ? ? ? 85 C0 74 ?? 8B 50 10 53 D9 45 ?? DC 05 ? ? ? ? 89 15 ? ? ? ? 8B 15 ? ? ? ? 89 50 10 8B 55 ?? 66 89 48 0C 8B 4D ?? D9 58 24";
    constexpr auto tracer_pool_head = "A1 ? ? ? ? 85 C0 74 ?? 8B 50 10 53 D9 45 ?? DC 05 ? ? ? ??";
}

namespace exports
{
    constexpr auto create_interface = "CreateInterface";
}
