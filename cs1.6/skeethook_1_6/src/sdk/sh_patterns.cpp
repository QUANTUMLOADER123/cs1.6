#include "../../include/sh_sdk.h"
#include "../../include/sh_config.h"
#include "../../include/sh_scanner.h"

#include <windows.h>

sh_engfuncs_t *g_engfuncs = 0;
sh_playermove_t *g_pmove = 0;
sh_globalvars_t *g_globals = 0;
void **g_entity_list = 0;
sh_cl_entity_t *(*g_get_entity)(i32 index) = 0;
i32 g_local_player = 0;
void *g_module_client = 0;
void *g_module_hw = 0;
sh_triapi_t *g_tri_api = 0;
void **g_pmove_slot = 0;
void **g_engfuncs_slot = 0;

static const u8 pat_init[] = {
    0x8B, 0x44, 0x24, 0x08, 0x81, 0xEC, 0x08, 0x04, 0x00, 0x00, 0xB9,
    0x87, 0x00, 0x00, 0x00, 0x83, 0xF8, 0x07, 0x53, 0x56, 0x8B, 0xB4};
static const char pat_init_mask[] = "xxxxxxxxxxxxxxxxxxxxxx";

static const u8 pat_player_move[] = {
    0x8B, 0x54, 0x24, 0x08, 0x8B, 0x44, 0x24, 0x04, 0x33, 0xC9, 0xA3,
    0x00, 0x00, 0x00, 0x00, 0x85, 0xD2, 0x0F, 0x95, 0xC1, 0x51, 0xE8};
static const char pat_player_move_mask[] = "xxxxxxxxxxx????xxxxxxx";

static const u8 pat_player_move_init[] = {
    0x8B, 0x44, 0x24, 0x04, 0xA3, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00,
    0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x05, 0x80};
static const char pat_player_move_init_mask[] = "xxxxx????x????x????xxx";

static const u8 pat_create_move[] = {
    0xA0, 0x00, 0x00, 0x00, 0x00, 0x83, 0xEC, 0x0C, 0x53, 0xBB, 0x01,
    0x00, 0x00, 0x00, 0x84, 0xC3, 0x56, 0x75, 0x17, 0x8A, 0xD0, 0x68};
static const char pat_create_move_mask[] = "x????xxxxxxxxxxxxxxxxx";

static const u8 pat_hud_redraw[] = {
    0x8B, 0x44, 0x24, 0x08, 0x8B, 0x4C, 0x24, 0x04, 0x50, 0x51, 0xB9,
    0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x01,
    0x00, 0x00, 0x00, 0xC3};
static const char pat_hud_redraw_mask[] = "xxxxxxxxxxx????x????xxxxxx";


static const u8 pat_process_state[] = {
    0x56, 0x8B, 0x74, 0x24, 0x0C, 0x57, 0x8B, 0x7C, 0x24, 0x0C, 0x8B,
    0x46, 0x04, 0x8B, 0x8E, 0x10, 0x01, 0x00, 0x00, 0x89};
static const char pat_process_state_mask[] = "xxxxxxxxxxxxxxxxxxxx";

static const u8 pat_add_entity[] = {
    0x8B, 0x44, 0x24, 0x04, 0x56, 0x8B, 0x74, 0x24, 0x0C, 0x85, 0xC0,
    0x7C, 0x53, 0x83, 0xF8, 0x01, 0x7F, 0x4E, 0x8B, 0x4E};
static const char pat_add_entity_mask[] = "xxxxxxxxxxxxxxxxxxxx";

static const u8 pat_txfer_overrides[] = {
    0x8B, 0x44, 0x24, 0x08, 0x8B, 0x4C, 0x24, 0x04, 0x8B, 0x10, 0x89,
    0x51, 0x10, 0x8B, 0x50, 0x04, 0x89, 0x51, 0x14, 0x8B};
static const char pat_txfer_overrides_mask[] = "xxxxxxxxxxxxxxxxxxxx";

typedef struct sh_named_pat_s
{
    const char *name;
    const sh_pattern_t pat;
} sh_named_pat_t;

static const sh_named_pat_t g_sig_table[] = {
    {"initialize", {"initialize", pat_init, pat_init_mask, (i32)sizeof(pat_init)}},
    {"player_move", {"player_move", pat_player_move, pat_player_move_mask,
                     (i32)sizeof(pat_player_move)}},
    {"player_move_init", {"player_move_init", pat_player_move_init,
                          pat_player_move_init_mask, (i32)sizeof(pat_player_move_init)}},
    {"create_move", {"create_move", pat_create_move, pat_create_move_mask,
                     (i32)sizeof(pat_create_move)}},
    {"hud_redraw", {"hud_redraw", pat_hud_redraw, pat_hud_redraw_mask,
                    (i32)sizeof(pat_hud_redraw)}},
    {"process_state", {"process_state", pat_process_state, pat_process_state_mask,
                       (i32)sizeof(pat_process_state)}},
    {"add_entity", {"add_entity", pat_add_entity, pat_add_entity_mask,
                    (i32)sizeof(pat_add_entity)}},
    {"txfer_overrides", {"txfer_overrides", pat_txfer_overrides,
                         pat_txfer_overrides_mask, (i32)sizeof(pat_txfer_overrides)}},
};

/* ВАЖНО: операнды вида "mov reg, imm32" в клиенте содержат АБСОЛЮТНЫЙ
   виртуальный адрес (ASLR в GoldSrc для клиентских модулей нет, адреса
   фиксированы при загрузке). Пересчитывать их от базы модуля НЕЛЬЗЯ:
   вычитание базы даёт мусор, и дальше читаются байты кода как
   указатели.

   Именно на этом падало: BF C0 5C A2 01 = mov edi, 0x01A25CC0,
   а 0x01A25CC0 лежит ВНЕ секций client.dll. sh_va_to_ptr вычитал
   базу client.dll (0x21240000 в игре), получал отрицательное
   смещение, и в g_engfuncs попадал адрес внутри .text hw.dll. */
static void *sh_abs_ptr(u32 va)
{
    if (!va)
        return 0;

    /* отсекаем очевидный мусор: адрес обязан быть в user-space
       и выровнен хотя бы на 4 */
    if (va < 0x00010000u || va >= 0x7FFF0000u)
        return 0;

    if (va & 3u)
        return 0;

    return (void *)(unsigned long)va;
}

void *sh_find_named_sig(const char *name)
{
    for (u32 i = 0; i < sh_countof(g_sig_table); ++i)
    {
        if (sh_str_eq(g_sig_table[i].name, name))
            return sh_find_pattern_in("client.dll", &g_sig_table[i].pat);
    }
    return 0;
}

/* Проверка, что адрес лежит в СЕКЦИИ ДАННЫХ какого-нибудь модуля,
   а не в коде. Для таблицы enginefuncs это критично: если указатель
   попал в .text, дальше мы читаем байты кода как адреса функций и
   падаем на первом же вызове (0xC0000005/IP=мусор).

   Возвращает 1, если адрес в .data/.rdata/.bss любого загруженного
   модуля или вообще не в модуле (тогда доверяем вызывающему). */
static qbool sh_addr_not_code(u32 addr)
{
    void *mods[3];
    const char *names[3] = { "client.dll", "hw.dll", 0 };
    i32 n = 0;

    for (i32 i = 0; i < 2; ++i)
    {
        void *b = sh_module_base(names[i]);

        if (b)
            mods[n++] = b;
    }

    for (i32 i = 0; i < n; ++i)
    {
        u8 *base = (u8 *)mods[i];
        IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
        IMAGE_NT_HEADERS *nt;
        u32 rva;

        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            continue;

        nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);

        if (nt->Signature != IMAGE_NT_SIGNATURE)
            continue;

        if (addr < (u32)(unsigned long)base ||
            addr - (u32)(unsigned long)base >= nt->OptionalHeader.SizeOfImage)
            continue;

        rva = addr - (u32)(unsigned long)base;

        {
            i32 nsec = nt->FileHeader.NumberOfSections;
            IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);

            for (i32 k = 0; k < nsec; ++k)
            {
                u32 va = sec[k].VirtualAddress;
                u32 sz = sec[k].Misc.VirtualSize;

                if (!sz)
                    sz = sec[k].SizeOfRawData;

                if (rva < va || rva - va >= sz)
                    continue;

                /* секция кода: имя начинается с ".text" */
                if (sec[k].Name[0] == '.' && sec[k].Name[1] == 't' &&
                    sec[k].Name[2] == 'e' && sec[k].Name[3] == 'x')
                    return 0;

                return 1;
            }
        }

        /* внутри образа, но вне описанных секций - не доверяем */
        return 0;
    }

    /* не в известных модулях - вызывающий сам решит */
    return 1;
}

/* адрес обязан лежать в .text какого-нибудь модуля, иначе это не
   функция (а байты кода/данных), и вызывать его нельзя */
static qbool sh_addr_executable(u32 addr)
{
    const char *names[3] = { "client.dll", "hw.dll", 0 };

    if (addr < 0x00010000u || addr >= 0x7FFF0000u)
        return 0;

    for (i32 i = 0; i < 2; ++i)
    {
        u8 *base = (u8 *)sh_module_base(names[i]);
        IMAGE_DOS_HEADER *dos;
        IMAGE_NT_HEADERS *nt;

        if (!base)
            continue;

        dos = (IMAGE_DOS_HEADER *)base;

        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            continue;

        nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);

        if (nt->Signature != IMAGE_NT_SIGNATURE)
            continue;

        if (addr < (u32)(unsigned long)base ||
            addr - (u32)(unsigned long)base >= nt->OptionalHeader.SizeOfImage)
            continue;

        {
            u32 rva = addr - (u32)(unsigned long)base;
            i32 nsec = nt->FileHeader.NumberOfSections;
            IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);

            for (i32 k = 0; k < nsec; ++k)
            {
                u32 va = sec[k].VirtualAddress;
                u32 sz = sec[k].Misc.VirtualSize;

                if (!sz)
                    sz = sec[k].SizeOfRawData;

                if (rva < va || rva - va >= sz)
                    continue;

                if (sec[k].Name[0] == '.' && sec[k].Name[1] == 't' &&
                    sec[k].Name[2] == 'e' && sec[k].Name[3] == 'x')
                    return 1;

                return 0;
            }
        }
    }

    return 0;
}

static void sh_resolve_initialize(void)
{
    u8 *hit = (u8 *)sh_find_named_sig("initialize");
    if (!hit)
        return;

    /* Ищем "BF ?? ?? ?? ?? F3 A5" - mov edi,<abs> + rep movsd.
       Это ТОЧНЫЙ признак копирования таблицы enginefuncs:
       BF задаёт приёмник, а F3 A5 подтверждает, что за ним идёт
       копирование блоком (а не произвольный mov). Без F3 A5 можно
       поймать любой "mov edi,const". */
    u8 *p = hit;
    i32 guard = 0;
    qbool found = 0;

    while (guard++ < 128)
    {
        if (p[0] == 0xBF && p[5] == 0xF3 && p[6] == 0xA5)
        {
            found = 1;
            break;
        }
        ++p;
    }
    if (!found)
        return;

    void **slot = (void **)sh_abs_ptr(*(u32 *)(p + 1));
    if (!slot)
        return;

    /* ВАЖНО: операнд BF - это АДРЕС САМОЙ ТАБЛИЦЫ, а не слота с
       указателем на неё.

       Код в client.dll, где нашлась сигнатура:
           B9 87 00 00 00        mov  ecx,0x87      ; 135 dword
           BF C0 5C A2 01        mov  edi,<buf>
           F3 A5                 rep movsd          ; копия таблицы
       135*4 = 0x21C = sizeof(enginefuncs_t) - совпадает точно.
       Значит <buf> - это копия таблицы движка, лежащая в .data.

       Раньше мы делали *(u32*)slot, то есть брали ПЕРВЫЙ ЭЛЕМЕНТ
       таблицы (указатель на функцию) и считали его самой таблицей.
       Дальше pfnGetLocalPlayer читался из середины кода функции
       (hw.dll+0x116E0 - начало .text), возвращал 0xC70CC483
       (байты "83 C4 70 8B") и падал при вызове. */
    {
        sh_engfuncs_t *table = (sh_engfuncs_t *)slot;

        if (!sh_addr_not_code((u32)(unsigned long)table))
        {
            g_engfuncs_slot = 0;
            g_engfuncs = 0;
            return;
        }

        g_engfuncs_slot = (void **)table;
        g_engfuncs = table;
    }
}

static void sh_resolve_player_move(void)
{
    u8 *hit = (u8 *)sh_find_named_sig("player_move");
    if (!hit)
        return;

    void **slot = (void **)sh_abs_ptr(*(u32 *)(hit + 12));
    if (!slot)
        return;

    g_pmove_slot = slot;
    g_pmove = (sh_playermove_t *)*slot;
}

void sh_sdk_resolve(void)
{
    g_module_client = sh_module_base("client.dll");
    g_module_hw = sh_module_base("hw.dll");

    sh_resolve_initialize();
    sh_resolve_player_move();

    if (g_engfuncs)
    {
        /* каждый указатель из таблицы проверяем на правдоподобность:
           вызов мусорного адреса = мгновенное падение с IP внутри
           кучи/кода. Проверка дешёвая, а падение дорогое. */
        void *ge = (void *)g_engfuncs->pfnGetEntityByIndex;
        void *gl = (void *)g_engfuncs->pfnGetLocalPlayer;

        if (sh_addr_executable((u32)(unsigned long)ge))
            g_get_entity = (sh_cl_entity_t * (*)(i32)) ge;
        else
            g_get_entity = 0;

        g_tri_api = (sh_triapi_t *)g_engfuncs->p_tri_api;

        if (sh_addr_executable((u32)(unsigned long)gl))
        {
            /* pfnGetLocalPlayer отдаёт cl_entity_s*, а нам нужен ИНДЕКС.
               Берём указатель и ищем его в списке сущностей - это
               единственный надёжный способ, т.к. отдельной функции
               "дай индекс локального игрока" в GoldSrc нет. */
            void *local = g_engfuncs->pfnGetLocalPlayer();

            g_local_player = 0;

            if (local && g_get_entity)
            {
                for (i32 i = 1; i < SH_MAX_ENTITIES; ++i)
                {
                    if (g_get_entity(i) == (sh_cl_entity_t *)local)
                    {
                        g_local_player = i;
                        break;
                    }
                }
            }
        }
    }
}

qbool sh_sdk_ready(void)
{
    return g_engfuncs != 0 && g_pmove != 0;
}

sh_cl_entity_t *sh_get_entity(i32 index)
{
    if (index < 0 || index >= SH_MAX_ENTITIES)
        return 0;
    if (!g_get_entity)
        return 0;
    return g_get_entity(index);
}

void sh_sdk_set_entity_fn(sh_cl_entity_t *(*fn)(i32))
{
    g_get_entity = fn;
}
