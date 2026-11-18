#include "../../include/sh_hooks.h"
#include "../../include/sh_config.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_types.h"
#include "../../include/sh_scanner.h"

#include <windows.h>

static HMODULE g_self;
static HANDLE g_thread;
static volatile LONG g_detach_done;

static DWORD WINAPI sh_detach_worker(void *param)
{
    (void)param;

    sh_hooks_uninstall();

    return 0;
}

static DWORD WINAPI sh_worker(void *param)
{
    (void)param;

    for (i32 i = 0; i < 600; ++i)
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
            Sleep(100);
            continue;
        }

        sh_config_load();

        if (sh_hooks_install())
            return 0;

        Sleep(100);
    }

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
