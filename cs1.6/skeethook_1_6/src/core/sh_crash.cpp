#include "../../include/sh_crash.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_scanner.h"

#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <string.h>

/* Всё, что нужно при падении, лежит в статических буферах:
   в обработчике исключения нельзя звать malloc/new и нельзя
   рассчитывать, что куча цела. */
#define SH_CRASH_BUF 16384

static char g_report[SH_CRASH_BUF];
static int g_report_len;
static char g_stage[128] = "старт";
static PVOID g_handler;
static volatile LONG g_in_handler;
static void *g_write_dump;   /* MiniDumpWriteDump, ищем заранее */

/* ------------------------------------------------------------------ */
/* запись в отчёт                                                      */

static void sh_c_put(const char *s)
{
    if (!s)
        return;

    while (*s && g_report_len < SH_CRASH_BUF - 1)
        g_report[g_report_len++] = *s++;

    g_report[g_report_len] = 0;
}

static void sh_c_put_hex(u32 v, i32 digits)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[16];
    i32 i;

    /* больше 8 для u32 смысла нет, а сдвиг на 32 и выше - UB */
    if (digits > 8)
        digits = 8;

    if (digits < 1)
        digits = 1;

    for (i = 0; i < digits; ++i)
        buf[i] = hex[(v >> ((digits - 1 - i) * 4)) & 0xF];

    buf[digits] = 0;
    sh_c_put(buf);
}

static void sh_c_put_dec(i32 v)
{
    char buf[16];
    i32 i = 0;
    u32 u;

    if (v < 0)
    {
        sh_c_put("-");
        u = (u32)(-(v + 1)) + 1u;
    }
    else
    {
        u = (u32)v;
    }

    if (u == 0)
    {
        sh_c_put("0");
        return;
    }

    while (u && i < 15)
    {
        buf[i++] = (char)('0' + (u % 10));
        u /= 10;
    }

    while (i-- > 0)
    {
        char c[2];
        c[0] = buf[i];
        c[1] = 0;
        sh_c_put(c);
    }
}

static void sh_c_ptr(u32 v)
{
    sh_c_put("0x");
    sh_c_put_hex(v, 8);
}

/* имя модуля, которому принадлежит адрес */
static void sh_c_module_of(u32 addr, char *out, i32 cap)
{
    HMODULE mods[256];
    DWORD needed = 0;
    i32 n = 0;

    out[0] = 0;

    if (!cap)
        return;

    if (!EnumProcessModules(GetCurrentProcess(), mods, sizeof(mods), &needed))
        return;

    n = (i32)(needed / sizeof(HMODULE));

    if (n > 256)
        n = 256;

    for (i32 i = 0; i < n; ++i)
    {
        IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)mods[i];
        IMAGE_NT_HEADERS *nt;
        u32 base = (u32)(u32)(unsigned long)mods[i];
        u32 size;
        char name[MAX_PATH];

        if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE)
            continue;

        nt = (IMAGE_NT_HEADERS *)((u8 *)dos + dos->e_lfanew);

        if (nt->Signature != IMAGE_NT_SIGNATURE)
            continue;

        size = nt->OptionalHeader.SizeOfImage;

        if (addr < base || addr - base >= size)
            continue;

        name[0] = 0;

        if (GetModuleFileNameA(mods[i], name, MAX_PATH))
        {
            i32 last = -1;
            i32 k = 0;

            while (name[k] && k < MAX_PATH - 1)
            {
                if (name[k] == '\\' || name[k] == '/')
                    last = k;
                ++k;
            }

            const char *base_name = (last >= 0) ? name + last + 1 : name;
            i32 j = 0;

            while (base_name[j] && j < cap - 1)
            {
                out[j] = base_name[j];
                ++j;
            }

            out[j] = 0;
            return;
        }

        return;
    }

    return;
}

/* безопасное чтение памяти: ReadProcessMemory не падает на плохом
   адресе, а возвращает ошибку - в отличие от __try, которого нет
   при -fno-exceptions */
static qbool sh_c_safe_read(u32 addr, void *out, u32 size)
{
    SIZE_T got = 0;

    if (!addr)
        return 0;

    if (!ReadProcessMemory(GetCurrentProcess(), (const void *)(unsigned long)addr,
                           out, (SIZE_T)size, &got))
        return 0;

    return got == (SIZE_T)size;
}

/* ------------------------------------------------------------------ */
/* разбор исключения                                                   */

static const char *sh_c_exception_name(u32 code)
{
    switch (code)
    {
    case 0xC0000005: return "ACCESS_VIOLATION (чтение/запись по плохому адресу)";
    case 0xC0000006: return "IN_PAGE_ERROR";
    case 0xC000001D: return "ILLEGAL_INSTRUCTION";
    case 0xC0000094: return "INTEGER_DIVIDE_BY_ZERO";
    case 0xC0000096: return "PRIVILEGED_INSTRUCTION";
    case 0xC00000FD: return "STACK_OVERFLOW";
    case 0xC0000374: return "HEAP_CORRUPTION (порча кучи)";
    case 0xC0000409: return "STACK_BUFFER_OVERRUN / fastfail";
    case 0xC0000417: return "INVALID_CRUNTIME_PARAMETER";
    case 0xC0000602: return "FAIL_FAST";
    case 0xC000008C: return "ARRAY_BOUNDS_EXCEEDED";
    case 0xC000008E: return "FLOAT_DIVIDE_BY_ZERO";
    case 0xC0000135: return "DLL_NOT_FOUND";
    case 0xC0000142: return "DLL_INIT_FAILED";
    case 0x80000003: return "BREAKPOINT";
    case 0x80000004: return "SINGLE_STEP";
    default: return "неизвестный код";
    }
}

static void sh_c_fmt_impl(EXCEPTION_POINTERS *ep)
{
    EXCEPTION_RECORD *er;
    CONTEXT *ctx;
    u32 code = 0;
    u32 addr = 0;
    char mod[64];
    i32 i;

    g_report[0] = 0;
    g_report_len = 0;

    sh_c_put("=== skeethook_1_6: crash report ===\r\n");
    sh_c_put("stage   : ");
    sh_c_put(g_stage);
    sh_c_put("\r\n");

    if (!ep || !ep->ExceptionRecord)
    {
        sh_c_put("нет EXCEPTION_POINTERS\r\n");
        return;
    }

    er = ep->ExceptionRecord;
    ctx = ep->ContextRecord;
    code = (u32)er->ExceptionCode;

    sh_c_put("code    : ");
    sh_c_ptr(code);
    sh_c_put("  ");
    sh_c_put(sh_c_exception_name(code));
    sh_c_put("\r\n");

    /* адрес, где произошло исключение */
    addr = (u32)(unsigned long)er->ExceptionAddress;
    sh_c_put("на месте: ");
    sh_c_ptr(addr);

    sh_c_module_of(addr, mod, sizeof(mod));

    if (mod[0])
    {
        sh_c_put("  (");
        sh_c_put(mod);
        sh_c_put(")");
    }

    sh_c_put("\r\n");

    /* параметры исключения: для AV это [0]=чтение/запись, [1]=адрес */
    if (er->NumberParameters >= 2)
    {
        u32 kind = (u32)er->ExceptionInformation[0];
        u32 bad = (u32)(unsigned long)er->ExceptionInformation[1];

        sh_c_put("операция: ");
        sh_c_put(kind == 0 ? "чтение" :
                 kind == 1 ? "запись" :
                 kind == 8 ? "исполнение" : "?");

        sh_c_put("  адрес: ");
        sh_c_ptr(bad);

        sh_c_module_of(bad, mod, sizeof(mod));

        if (mod[0])
        {
            sh_c_put("  (");
            sh_c_put(mod);
            sh_c_put(")");
        }
        else
        {
            sh_c_put("  (вне известных модулей)");
        }

        sh_c_put("\r\n");
    }

    if (ctx)
    {
        sh_c_put("eip     : ");
        sh_c_ptr((u32)ctx->Eip);
        sh_c_module_of((u32)ctx->Eip, mod, sizeof(mod));
        if (mod[0])
        {
            sh_c_put("  (");
            sh_c_put(mod);
            sh_c_put(")");
        }
        sh_c_put("\r\n");

        sh_c_put("esp     : ");
        sh_c_ptr((u32)ctx->Esp);
        sh_c_put("  ebp: ");
        sh_c_ptr((u32)ctx->Ebp);
        sh_c_put("  eax: ");
        sh_c_ptr((u32)ctx->Eax);
        sh_c_put("\r\n");

        sh_c_put("ecx     : ");
        sh_c_ptr((u32)ctx->Ecx);
        sh_c_put("  edx: ");
        sh_c_ptr((u32)ctx->Edx);
        sh_c_put("  ebx: ");
        sh_c_ptr((u32)ctx->Ebx);
        sh_c_put("  esi: ");
        sh_c_ptr((u32)ctx->Esi);
        sh_c_put("  edi: ");
        sh_c_ptr((u32)ctx->Edi);
        sh_c_put("\r\n");

        sh_c_put("eflags  : ");
        sh_c_ptr((u32)ctx->EFlags);
        sh_c_put("\r\n");

        /* стек вызовов: 8 значений со стека - примитивно, но по ним
           обычно видно, была ли цепочка вызовов вообще (порча стека
           даст мусор, нормальный вызов - узнаваемые адреса модулей) */
        sh_c_put("стек    :\r\n");

        for (i = 0; i < 8; ++i)
        {
            u32 v = 0;
            u32 sp = (u32)ctx->Esp + (u32)(i * 4);

            if (!sh_c_safe_read(sp, &v, 4))
                v = 0xFFFFFFFF;

            sh_c_put("   [esp+");
            sh_c_put_dec(i * 4);
            sh_c_put("] ");
            sh_c_ptr(v);

            sh_c_module_of(v, mod, sizeof(mod));

            if (mod[0])
            {
                sh_c_put("  (");
                sh_c_put(mod);
                sh_c_put(")");
            }

            sh_c_put("\r\n");
        }
    }
    else
    {
        sh_c_put("нет CONTEXT\r\n");
    }

    /* состояние мода на момент падения */
    sh_c_put("--- состояние мода ---\r\n");
    sh_c_put("client.dll: ");
    sh_c_ptr((u32)(unsigned long)sh_module_base("client.dll"));
    sh_c_put("  hw.dll: ");
    sh_c_ptr((u32)(unsigned long)sh_module_base("hw.dll"));
    sh_c_put("\r\n");
    sh_c_put("g_pmove   : ");
    sh_c_ptr((u32)(unsigned long)g_pmove);
    sh_c_put("  g_engfuncs: ");
    sh_c_ptr((u32)(unsigned long)g_engfuncs);
    sh_c_put("\r\n");

    sh_c_put("=== конец ===\r\n");
}

static void sh_c_write_file(void)
{
    char dir[MAX_PATH];
    char path[MAX_PATH];
    DWORD len;
    FILE *f;
    i32 last = -1;

    dir[0] = 0;
    len = GetModuleFileNameA(0, dir, MAX_PATH);

    if (len == 0 || len >= MAX_PATH)
        return;

    dir[MAX_PATH - 1] = 0;
    dir[len] = 0;

    for (i32 i = 0; i < MAX_PATH - 1 && dir[i]; ++i)
    {
        if (dir[i] == '\\')
            last = i;
    }

    if (last < 0 || last + 1 >= MAX_PATH)
        return;

    dir[last + 1] = 0;

    snprintf(path, MAX_PATH, "%sskeethook_crash.log", dir);

    f = fopen(path, "wb");

    if (!f)
        return;

    fwrite(g_report, 1, (size_t)g_report_len, f);
    fclose(f);
}

/* Мини-дамп: если рядом с игрой есть dbghelp.dll, пишем полный дамп
   стека. Это необязательно - файл появляется только когда вышло.
   Загружаем dbghelp динамически, чтобы не тянуть зависимость. */
static void sh_c_write_dump(EXCEPTION_POINTERS *ep)
{
    typedef BOOL(WINAPI * mwd_fn)(HANDLE, DWORD, HANDLE, i32, void *, void *,
                                  void *);
    /* MINIDUMP_EXCEPTION_INFORMATION: ровно три поля, 12 байт.
       ExceptionPointers обязан указывать на настоящий
       EXCEPTION_POINTERS - иначе dbghelp читает мусор. */
    typedef struct
    {
        u32 thread_id;
        void *exception_pointers;
        i32 client_pointers;
    } dump_exinfo_t;

    static const char dll_name[] = "dbghelp.dll";

    char dir[MAX_PATH];
    char path[MAX_PATH];
    DWORD len;
    i32 last = -1;
    HMODULE dbg = 0;
    mwd_fn write_dump;
    HANDLE file;
    dump_exinfo_t exinfo;

    if (!ep)
        return;

    dir[0] = 0;
    len = GetModuleFileNameA(0, dir, MAX_PATH);

    if (len == 0 || len >= MAX_PATH)
        return;

    dir[MAX_PATH - 1] = 0;
    dir[len] = 0;

    for (i32 i = 0; i < MAX_PATH - 1 && dir[i]; ++i)
    {
        if (dir[i] == '\\')
            last = i;
    }

    if (last < 0 || last + 1 >= MAX_PATH)
        return;

    dir[last + 1] = 0;

    (void)dbg;
    (void)dll_name;

    /* dbghelp уже загружен в sh_crash_init: LoadLibrary внутри
       обработчика краша - риск дедлока на loader-lock */
    write_dump = (mwd_fn)g_write_dump;

    if (!write_dump)
        return;

    snprintf(path, MAX_PATH, "%sskeethook_crash.dmp", dir);

    file = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, 0);

    if (file == INVALID_HANDLE_VALUE)
        return;

    exinfo.thread_id = GetCurrentThreadId();
    exinfo.exception_pointers = (void *)ep;
    /* 0: EXCEPTION_POINTERS лежит в НАШЕМ процессе */
    exinfo.client_pointers = 0;

    /* MiniDumpWithDataSegs(0x1) | MiniDumpWithFullMemory(0x2) |
       MiniDumpWithThreadInfo(0x1000) */
    write_dump(GetCurrentProcess(), GetCurrentProcessId(), file,
               0x1u | 0x2u | 0x1000u, 0, &exinfo, 0);

    CloseHandle(file);
}

static void sh_c_to_clipboard(void)
{
    if (!OpenClipboard(0))
        return;

    EmptyClipboard();

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)g_report_len + 1);

    if (mem)
    {
        void *dst = GlobalLock(mem);

        if (dst)
        {
            for (i32 i = 0; i <= g_report_len; ++i)
                ((char *)dst)[i] = i < g_report_len ? g_report[i] : 0;

            GlobalUnlock(mem);

            if (!SetClipboardData(CF_TEXT, mem))
                GlobalFree(mem);
        }
        else
        {
            GlobalFree(mem);
        }
    }

    CloseClipboard();
}

/* записывает отчёт один раз, дальше молчит */
static void sh_c_capture(EXCEPTION_POINTERS *ep)
{
    if (InterlockedExchange(&g_in_handler, 1) != 0)
        return;

    /* без SEH: каждый шаг сам проверяет всё, что может не сработать.
       буферы статические, куча не используется - падать нечему */
    sh_c_fmt_impl(ep);
    sh_c_write_file();
    sh_c_write_dump(ep);
    sh_c_to_clipboard();
}

/* VEH: вызывается раньше всех, но и на ШТАТНЫХ исключениях движка
   тоже. Поэтому фильтруем по коду - пишем только на фатальные,
   которые иначе убьют процесс без отчёта. */
static LONG WINAPI sh_c_veh(EXCEPTION_POINTERS *ep)
{
    u32 code = 0;

    if (ep && ep->ExceptionRecord)
        code = (u32)ep->ExceptionRecord->ExceptionCode;

    switch (code)
    {
    case 0xC0000409: /* STACK_BUFFER_OVERRUN / fastfail */
    case 0xC0000374: /* HEAP_CORRUPTION */
    case 0xC0000602: /* FAIL_FAST */
        sh_c_capture(ep);
        break;
    default:
        break;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

static LONG WINAPI sh_c_ueh(EXCEPTION_POINTERS *ep)
{
    sh_c_capture(ep);

    /* EXCEPTION_CONTINUE_SEARCH: отдаём дальше, чтобы игра вела себя
       как обычно - своё окно или тихое закрытие */
    return EXCEPTION_CONTINUE_SEARCH;
}

void sh_crash_stage(const char *stage)
{
    if (!stage)
        return;

    /* вызывается несколько раз за кадр - не гоняем посимвольный
       цикл, lstrcpynA обрезает сам и быстрее в разы */
    lstrcpynA(g_stage, stage, (i32)sizeof(g_stage));
}

const char *sh_crash_last_report(void)
{
    return g_report;
}

void sh_c_crash_test(void)
{
    char mod[64];

    g_report[0] = 0;
    g_report_len = 0;

    sh_c_put("=== skeethook_1_6: тест обработчика (не краш) ===\r\n");
    sh_c_put("stage   : ");
    sh_c_put(g_stage);
    sh_c_put("\r\n");

    sh_c_put("client.dll: ");
    sh_c_ptr((u32)(unsigned long)sh_module_base("client.dll"));
    sh_c_module_of((u32)(unsigned long)sh_module_base("client.dll"), mod, sizeof(mod));

    if (mod[0])
    {
        sh_c_put("  (");
        sh_c_put(mod);
        sh_c_put(")");
    }

    sh_c_put("\r\n");
    sh_c_put("hw.dll    : ");
    sh_c_ptr((u32)(unsigned long)sh_module_base("hw.dll"));
    sh_c_put("\r\n");
    sh_c_put("dbghelp   : ");
    sh_c_ptr((u32)(unsigned long)g_write_dump);
    sh_c_put("  (MiniDumpWriteDump)\r\n");
    sh_c_put("g_pmove   : ");
    sh_c_ptr((u32)(unsigned long)g_pmove);
    sh_c_put("  g_engfuncs: ");
    sh_c_ptr((u32)(unsigned long)g_engfuncs);
    sh_c_put("\r\n");
    sh_c_put("=== конец теста ===\r\n");

    sh_c_write_file();
    sh_c_to_clipboard();
}

void sh_crash_init(void)
{
    if (g_handler)
        return;

    g_report[0] = 0;
    g_report_len = 0;

    /* dbghelp грузим ЗАРАНЕЕ. В обработчике краша LoadLibrary нельзя:
       он берёт loader-lock, а падение вполне может случиться в момент,
       когда этот лок уже кем-то другим захвачен. */
    {
        HMODULE dbg = LoadLibraryA("dbghelp.dll");

        if (!dbg)
        {
            char dir[MAX_PATH];
            char path[MAX_PATH];
            DWORD len = GetModuleFileNameA(0, dir, MAX_PATH);

            if (len > 0 && len < MAX_PATH)
            {
                i32 last = -1;

                dir[MAX_PATH - 1] = 0;
                dir[len] = 0;

                for (i32 i = 0; i < MAX_PATH - 1 && dir[i]; ++i)
                {
                    if (dir[i] == '\\')
                        last = i;
                }

                if (last >= 0 && last + 1 < MAX_PATH)
                {
                    dir[last + 1] = 0;
                    snprintf(path, MAX_PATH, "%sdbghelp.dll", dir);
                    dbg = LoadLibraryA(path);
                }
            }
        }

        if (dbg)
            g_write_dump = (void *)GetProcAddress(dbg, "MiniDumpWriteDump");
    }

    /* оба: UEH ловит обычные необработанные падения, VEH - фатальные
       коды, до которых UEH не доходит */
    SetUnhandledExceptionFilter(sh_c_ueh);
    g_handler = AddVectoredExceptionHandler(1, sh_c_veh);
}
