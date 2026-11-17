#!/usr/bin/env python3
"""
pe_triage.py — быстрый статический разбор PE-файлов (CS 1.6: hw.dll, client.dll, cs.dll).

Что делает, без запуска файла:
  * заголовки: машина, таймстамп, характеристики, imagebase, entrypoint
  * секции: энтропия, подозрительность (W+X, пустые имена, огромные raw-размеры)
  * импорты/экспорты: по библиотекам, поиск "интересных" Win32 API
  * экспорты-интерфейсы GoldSrc: CreateInterface, GetEntityAPI, GiveFnptrsToDll
  * RTTI: поиск имён классов (CHud*, CBaseEntity и т.п.)
  * строки: ASCII/UTF-16, фильтр по ключевым словам
  * сигнатуры: печать MD5/SHA256

Использование:
  python3 tools/analysis/pe_triage.py cs1.6/cstrike/cl_dll/client.dll
  python3 tools/analysis/pe_triage.py cs1.6/**/*.dll --json out.json
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

try:
    import pefile
except ImportError:
    sys.exit("нужен pefile: pip install --break-system-packages pefile")

INTERESTING_APIS = {
    # память / процессы
    "VirtualAlloc", "VirtualProtect", "VirtualAllocEx", "WriteProcessMemory",
    "ReadProcessMemory", "CreateRemoteThread", "OpenProcess", "NtAllocateVirtualMemory",
    "NtProtectVirtualMemory", "NtWriteVirtualMemory", "NtCreateThreadEx",
    # загрузка кода
    "LoadLibraryA", "LoadLibraryW", "LoadLibraryExA", "LoadLibraryExW",
    "GetProcAddress", "LdrLoadDll", "LdrGetProcedureAddress",
    # хуки / детуры
    "SetWindowsHookExA", "SetWindowsHookExW", "SetWindowLongA", "SetWindowLongPtrA",
    "GetWindowLongA", "GetAsyncKeyState", "GetKeyState", "GetKeyboardState",
    # графика / ввод (для оверлеев)
    "wglSwapBuffers", "wglMakeCurrent", "wglGetProcAddress", "ChoosePixelFormat",
    "SetPixelFormat", "DescribePixelFormat", "SwapBuffers", "PeekMessageA",
    "PeekMessageW", "GetDC", "ReleaseDC", "RegisterClassA", "CreateWindowExA",
    "ScreenToClient", "ClientToScreen", "GetCursorPos", "SetCursorPos",
    # сеть (для читов/ботов)
    "WS2_32.dll", "ws2_32.dll", "send", "recv", "sendto", "recvfrom",
    "connect", "bind", "socket", "WSAStartup", "gethostbyname", "inet_addr",
    # файлы / конфиг
    "CreateFileA", "CreateFileW", "ReadFile", "WriteFile", "GetPrivateProfileIntA",
    "GetPrivateProfileStringA", "WritePrivateProfileStringA",
    # системная инфа
    "GetComputerNameA", "GetVolumeInformationA", "GetAdaptersInfo",
    "RegOpenKeyExA", "RegQueryValueExA",
    # крипто
    "CryptEncrypt", "CryptDecrypt", "CryptAcquireContextA", "BCryptEncrypt",
}

GOLDSRC_EXPORTS = {
    "CreateInterface": "фабрика интерфейсов Source/GoldSrc (ClientFactory, VClient, VGUI)",
    "GetEntityAPI": "GameDLL v1 (сервер: entity-хуки)",
    "GetEntityAPI2": "GameDLL v2",
    "GiveFnptrsToDll": "GameDLL: движок передаёт указатели на engine-функции",
    "Server_GetBlendingInterface": "клиент: интерфейс блендинга моделей",
    "F": "точка входа GameDLL (экспорт 'F' — стандарт HL)",
    "GetClientAPI": "клиентская API-таблица",
    "HUD_GetHudModuleInterface": "HUD",
    "VEngineClient014": "интерфейс движка v14 (Source, но встречается в ребилдах)",
}

INTERESTING_STRINGS = [
    "cl_", "sv_", "hud_", "weapon_", "ammo_", "item_", "func_", "trigger_",
    "player/", "models/", "sprites/", "sound/",
    "cheat", "aim", "esp", "wall", "trigger", "bhop", "strafe", "jump",
    "speedhack", "noclip", "godmode", "infinite", "unlimited",
    "ClientFactory", "VClient", "VEngineClient", "CBaseEntity", "CHud",
    "CStudioModelRenderer", "CViewRender", "CEntity",
    "http://", "https://", "api.", ".ru", ".com",
]


def entropy(data: bytes) -> float:
    if not data:
        return 0.0
    counts = Counter(data)
    n = len(data)
    return -sum((c / n) * math.log2(c / n) for c in counts.values())


def section_report(pe) -> list[dict]:
    out = []
    for s in pe.sections:
        name = s.Name.rstrip(b"\x00").decode("latin-1", "replace")
        raw = s.get_data() if s.SizeOfRawData else b""
        ent = entropy(raw)
        flags = []
        if s.Characteristics & 0x20000000:
            flags.append("X")
        if s.Characteristics & 0x80000000:
            flags.append("W")
        if s.Characteristics & 0x40000000:
            flags.append("R")
        suspicious = []
        if "W" in flags and "X" in flags:
            suspicious.append("W+X")
        if ent > 7.2 and len(raw) > 512:
            suspicious.append("high-entropy")
        if not name:
            suspicious.append("empty-name")
        out.append({
            "name": name,
            "vaddr": hex(s.VirtualAddress),
            "vsize": s.Misc_VirtualSize,
            "rawsize": s.SizeOfRawData,
            "entropy": round(ent, 3),
            "flags": "".join(flags),
            "suspicious": suspicious,
        })
    return out


def import_report(pe) -> dict:
    libs: dict[str, list[str]] = defaultdict(list)
    if not hasattr(pe, "DIRECTORY_ENTRY_IMPORT"):
        return {"libs": {}, "interesting": []}
    interesting = []
    for entry in pe.DIRECTORY_ENTRY_IMPORT:
        dll = entry.dll.decode("latin-1", "replace")
        for imp in entry.imports:
            fn = (imp.name or b"").decode("latin-1", "replace") if imp.name else f"ord_{imp.ordinal}"
            libs[dll].append(fn)
            if fn in INTERESTING_APIS or dll.lower() in INTERESTING_APIS:
                interesting.append(f"{dll}!{fn}")
    # импорты по ординалу (часто упаковщики)
    ord_only = [f"{d}!ord" for d, fns in libs.items() if all(f.startswith("ord_") for f in fns)]
    return {"libs": dict(libs), "interesting": interesting, "ordinal_only_dlls": ord_only}


def export_report(pe) -> dict:
    if not hasattr(pe, "DIRECTORY_ENTRY_EXPORT"):
        return {"count": 0, "goldSrc": [], "all": []}
    exports = []
    gold = []
    for exp in pe.DIRECTORY_ENTRY_EXPORT.symbols:
        name = (exp.name or b"").decode("latin-1", "replace") if exp.name else f"ord_{exp.ordinal}"
        exports.append({"name": name, "ordinal": exp.ordinal, "addr": hex(exp.address)})
        if name in GOLDSRC_EXPORTS:
            gold.append({"name": name, "desc": GOLDSRC_EXPORTS[name], "addr": hex(exp.address)})
        elif name.startswith("VEngine") or name.startswith("VClient") or name.startswith("VSERVER"):
            gold.append({"name": name, "desc": "интерфейс движка (vtable-класс)", "addr": hex(exp.address)})
    return {"count": len(exports), "goldSrc": gold, "all": exports}


def string_report(pe, min_len: int = 5, limit: int = 200) -> dict:
    data = pe.__data__
    ascii_re = re.compile(rb"[\x20-\x7e]{%d,}" % min_len)
    utf16_re = re.compile((rb"(?:[\x20-\x7e]\x00){%d,}" % min_len))

    ascii_strings = [m.group().decode("latin-1") for m in ascii_re.finditer(data)]
    utf16_strings = [m.group().decode("utf-16-le") for m in utf16_re.finditer(data)]

    def hits(strings):
        seen = set()
        out = []
        for s in strings:
            low = s.lower()
            for kw in INTERESTING_STRINGS:
                if kw.lower() in low and s not in seen:
                    seen.add(s)
                    out.append(s.strip())
                    break
            if len(out) >= limit:
                break
        return out

    return {
        "ascii_total": len(ascii_strings),
        "utf16_total": len(utf16_strings),
        "interesting": hits(ascii_strings + utf16_strings),
        "sample": [s.strip() for s in ascii_strings[:40] if len(s.strip()) > 8],
    }


def rtti_report(pe) -> list[str]:
    """Имена классов из RTTI (.?AVClassName@@). Работает для MSVC-сборок."""
    data = pe.__data__
    pat = re.compile(rb"\.\?A[VU]\?([A-Za-z_][\w@:<>,$]*?)@@")
    names = []
    seen = set()
    for m in pat.finditer(data):
        n = m.group(1).decode("latin-1", "replace")
        if n not in seen:
            seen.add(n)
            names.append(n)
    # GoldSrc часто без RTTI (C, не C++) — тогда ловим "CClass::Method" из строк
    method_pat = re.compile(rb"\b([A-Z][A-Za-z0-9_]{2,30})::([A-Za-z_][A-Za-z0-9_]{2,40})\b")
    methods = []
    seen_m = set()
    for m in method_pat.finditer(data):
        key = m.group(0).decode("latin-1")
        if key not in seen_m:
            seen_m.add(key)
            methods.append(key)
    return names + methods[:200]


def analyze(path: Path) -> dict:
    raw = path.read_bytes()
    pe = pefile.PE(data=raw, fast_load=False)
    pe.__data__ = raw  # pefile не хранит raw целиком

    machine = pefile.MACHINE_TYPE.get(pe.FILE_HEADER.Machine, hex(pe.FILE_HEADER.Machine))
    is_32 = pe.FILE_HEADER.Machine == 0x14c

    result = {
        "file": str(path),
        "size": len(raw),
        "md5": hashlib.md5(raw).hexdigest(),
        "sha256": hashlib.sha256(raw).hexdigest(),
        "machine": machine,
        "arch": "x86 (32-bit)" if is_32 else "x64/other",
        "timestamp": pe.FILE_HEADER.TimeDateStamp,
        "characteristics": {
            "dll": bool(pe.FILE_HEADER.Characteristics & 0x2000),
            "exe": not bool(pe.FILE_HEADER.Characteristics & 0x2000),
        },
        "imagebase": hex(pe.OPTIONAL_HEADER.ImageBase),
        "entrypoint": hex(pe.OPTIONAL_HEADER.AddressOfEntryPoint),
        "subsystem": pe.OPTIONAL_HEADER.Subsystem,
        "sections": section_report(pe),
        "imports": import_report(pe),
        "exports": export_report(pe),
        "strings": string_report(pe),
        "rtti": rtti_report(pe),
        "protection": {
            "aslr": bool(pe.OPTIONAL_HEADER.DllCharacteristics & 0x40),
            "dep": bool(pe.OPTIONAL_HEADER.DllCharacteristics & 0x100),
            "cfg": bool(pe.OPTIONAL_HEADER.Milliseconds if False else (pe.OPTIONAL_HEADER.DllCharacteristics & 0x4000)),
        },
        "packer_hints": [],
    }

    # эвристики на упаковку
    ep_sec = None
    ep = pe.OPTIONAL_HEADER.AddressOfEntryPoint
    for s in pe.sections:
        if s.VirtualAddress <= ep < s.VirtualAddress + max(s.Misc_VirtualSize, s.SizeOfRawData):
            ep_sec = s.Name.rstrip(b"\x00").decode("latin-1", "replace")
            break
    result["entrypoint_section"] = ep_sec
    for sec in result["sections"]:
        if "high-entropy" in sec["suspicious"] and sec["name"] != ".rsrc":
            result["packer_hints"].append(f"секция {sec['name']} с высокой энтропией {sec['entropy']}")
    if result["imports"].get("ordinal_only_dlls"):
        result["packer_hints"].append("импорты только по ординалу: " + ", ".join(result["imports"]["ordinal_only_dlls"]))
    if ep_sec and ep_sec not in (".text", "CODE", ".code"):
        result["packer_hints"].append(f"entrypoint в секции {ep_sec}")

    pe.close()
    return result


def print_report(r: dict) -> None:
    print("=" * 78)
    print(f"{r['file']}")
    print("=" * 78)
    print(f"  размер   : {r['size']:,} байт")
    print(f"  md5      : {r['md5']}")
    print(f"  sha256   : {r['sha256']}")
    print(f"  машина   : {r['machine']}  ({r['arch']})")
    print(f"  тип      : {'DLL' if r['characteristics']['dll'] else 'EXE'}")
    print(f"  imagebase: {r['imagebase']}   entrypoint: {r['entrypoint']} (секция {r.get('entrypoint_section')})")
    prot = r["protection"]
    print(f"  защита   : ASLR={'да' if prot['aslr'] else 'нет'}  DEP={'да' if prot['dep'] else 'нет'}  CFG={'да' if prot['cfg'] else 'нет'}")

    print("\n  СЕКЦИИ")
    print(f"  {'имя':<10} {'vaddr':<10} {'vsize':>9} {'raw':>9} {'энтр':>6} {'флаги':<6} примечания")
    for s in r["sections"]:
        note = ", ".join(s["suspicious"]) if s["suspicious"] else ""
        print(f"  {s['name']:<10} {s['vaddr']:<10} {s['vsize']:>9} {s['rawsize']:>9} {s['entropy']:>6} {s['flags']:<6} {note}")

    ex = r["exports"]
    print(f"\n  ЭКСПОРТЫ: {ex['count']}")
    if ex["goldSrc"]:
        print("  GoldSrc-интерфейсы:")
        for g in ex["goldSrc"]:
            print(f"    {g['addr']:>10}  {g['name']:<32} {g['desc']}")

    im = r["imports"]
    libs = im.get("libs", {})
    print(f"\n  ИМПОРТЫ: {len(libs)} библиотек, {sum(len(v) for v in libs.values())} функций")
    for dll, fns in sorted(libs.items(), key=lambda kv: -len(kv[1]))[:15]:
        preview = ", ".join(fns[:8]) + (" ..." if len(fns) > 8 else "")
        print(f"    {dll:<24} {len(fns):>4}  {preview}")
    if im.get("interesting"):
        print("  Интересные импорты:")
        for i in im["interesting"][:40]:
            print(f"    ! {i}")

    st = r["strings"]
    print(f"\n  СТРОКИ: ascii={st['ascii_total']}, utf16={st['utf16_total']}")
    if st["interesting"]:
        print("  По ключевым словам:")
        for s in st["interesting"][:60]:
            print(f"    {s[:110]}")

    if r["rtti"]:
        print(f"\n  RTTI/символы классов: {len(r['rtti'])}")
        for n in r["rtti"][:60]:
            print(f"    {n}")

    if r["packer_hints"]:
        print("\n  ПРИЗНАКИ УПАКОВКИ/ЗАЩИТЫ")
        for h in r["packer_hints"]:
            print(f"    ? {h}")


def main() -> int:
    ap = argparse.ArgumentParser(description="Быстрый статический триаж PE (без запуска)")
    ap.add_argument("files", nargs="+", type=Path)
    ap.add_argument("--json", type=Path, help="сохранить полный отчёт в JSON")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    reports = []
    for f in args.files:
        if not f.exists():
            print(f"[!] нет файла: {f}", file=sys.stderr)
            continue
        try:
            r = analyze(f)
        except pefile.PEFormatError as e:
            print(f"[!] не PE: {f}: {e}", file=sys.stderr)
            continue
        reports.append(r)
        if not args.quiet:
            print_report(r)
            print()

    if args.json:
        args.json.write_text(json.dumps(reports, indent=2, ensure_ascii=False), encoding="utf-8")
        print(f"JSON: {args.json}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
