#include "../../include/sh_hooks.h"
#include "../../include/sh_config.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_types.h"
#include "../../include/sh_scanner.h"
#include "../../include/sh_crash.h"

#include <windows.h>
#include <stdio.h>

static HMODULE g_self;
static HANDLE g_thread;
static volatile LONG g_detach_done;

static DWORD WINAPI sh_detach_worker(void *param)
{
    (void)param;

    sh_hooks_uninstall();

    return 0;
}

/* маячок для диагностики: пишем в тот же каталог, что и конфиг,
   чтобы при «ничего не произошло» было видно, докуда дошёл воркер */
static void sh_trace(const char *stage)
{
    char path[MAX_PATH];
    char dir[MAX_PATH];

    DWORD len = GetModuleFileNameA(0, dir, MAX_PATH);

    if (len == 0 || len >= MAX_PATH)
        return;

    dir[MAX_PATH - 1] = 0;
    dir[len] = 0;

    i32 last = -1;

    for (i32 i = 0; i < MAX_PATH - 1 && dir[i]; ++i)
    {
        if (dir[i] == '\\')
            last = i;
    }

    if (last < 0 || last + 1 >= MAX_PATH)
        return;

    dir[last + 1] = 0;

    snprintf(path, MAX_PATH, "%sskeethook_trace.log", dir);

    FILE *f = fopen(path, "ab");

    if (!f)
        return;

    fprintf(f, "%s\n", stage);
    fclose(f);
}

static DWORD WINAPI sh_worker(void *param)
{
    (void)param;

    sh_crash_init();
    sh_crash_stage("воркер запущен");

    /* до 10 минут: инжект в главном меню не даёт g_pmove, пока не
       загрузится карта - ждать надо столько, сколько игрок сидит в меню */
    for (i32 i = 0; i < 12000; ++i)
    {
        if (!sh_module_base("client.dll"))
        {
            Sleep(50);
            continue;
        }

        Sleep(120);
        sh_sdk_resolve();

        if (!sh_sdk_ready())
        {
            if ((i % 600) == 0)
                sh_trace("ждём player_move (инжект в меню? нужна карта)");

            Sleep(100);
            continue;
        }

        sh_crash_stage("конфиг");
        sh_config_load();

        sh_crash_stage("установка хуков");

        if (sh_hooks_install())
        {
            sh_crash_stage("работает");
            sh_trace("хуки установлены");
            return 0;
        }

        if ((i % 20) == 0)
            sh_trace("хуки не встали, повтор");
        Sleep(100);
    }

    sh_trace("истекло время ожидания");

    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_self = module;
        DisableThreadLibraryCalls(module);

        g_thread = CreateThread(0, 0, sh_worker, 0, 0, 0);
        if (g_thread)
            CloseHandle(g_thread);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        /* reserved != 0 - процесс завершается, разбирать хуки нельзя:
           MinHook замораживает все потоки, а под loader-lock это
           дедлок. При завершении процесса память и так отдаст ОС. */
        if (reserved == 0)
            sh_hooks_uninstall();
    }

    return TRUE;
}

SH_EXPORT const char *sh_get_name(void)
{
    return SH_NAME;
}

SH_EXPORT const char *sh_get_version(void)
{
    return SH_VERSION;
}

SH_EXPORT sh_config *sh_get_config(void)
{
    return &g_cfg;
}

SH_EXPORT qbool sh_is_attached(void)
{
    return sh_sdk_ready();
}

/* Проверка обработчика крашей без падения игры: пишет отчёт
   в skeethook_crash.log и в буфер обмена. */
SH_EXPORT qbool sh_test_crash_report(void)
{
    sh_c_crash_test();

    return 1;
}
