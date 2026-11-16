#!/usr/bin/env python3
"""
r2_decompile.py — прогон radare2 по DLL и выгрузка декомпиляции всех функций.

Делает:
  1. r2 -A (aa) — базовый анализ
  2. список функций + размеры
  3. pdc по каждой функции -> analysis/<name>/decomp/<addr>_<func>.c
  4. список экспортов/импортов с адресами -> analysis/<name>/symbols.txt
  5. vtable-скан (avrr) -> analysis/<name>/vtables.txt
  6. строки с адресами -> analysis/<name>/strings.txt

Использование:
  python3 tools/analysis/r2_decompile.py cs1.6/cstrike/cl_dll/client.dll
  python3 tools/analysis/r2_decompile.py file.dll --min-size 16 --jobs 8
"""
from __future__ import annotations

import argparse
import concurrent.futures as cf
import json
import os
import re
import subprocess
import sys
from pathlib import Path

R2 = os.environ.get("R2", "r2")


def run_r2(target: Path, commands: str, timeout: int = 600, project: Path | None = None,
           extra: list[str] | None = None) -> str:
    """Запуск r2. Если project задан — грузим готовый проект (-p), а не бинарник."""
    if project is not None:
        cmd = [R2, "-q", "-e", "scr.color=0", "-p", str(project)]
    else:
        cmd = [R2, "-q", "-e", "scr.color=0", "-e", "bin.relocs.apply=true"]
    if extra:
        cmd.extend(extra)
    cmd.extend(["-c", commands, str(target)])
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return ""
    out = p.stdout
    # отрезаем INFO/HINT строки анализа
    return "\n".join(l for l in out.splitlines() if not l.startswith(("INFO:", "HINT:")))


def build_project(path: Path, project: Path, level: str = "aaa") -> bool:
    """Один раз анализируем бинарник и сохраняем проект r2 — чтобы не гонять aaa на каждую функцию."""
    project.parent.mkdir(parents=True, exist_ok=True)
    if project.exists():
        return True
    cmd = [R2, "-q", "-e", "scr.color=0", "-e", "bin.relocs.apply=true",
           "-c", f"{level}; Ps {project}; q", str(path)]
    try:
        subprocess.run(cmd, capture_output=True, text=True, timeout=3600)
    except subprocess.TimeoutExpired:
        return False
    return project.exists()


def list_functions(path: Path, project: Path | None = None) -> list[dict]:
    """aflj -> JSON со всеми функциями."""
    raw = run_r2(path, "aflj", timeout=900, project=project)
    start = raw.find("[")
    if start < 0:
        return []
    try:
        data = json.loads(raw[start:])
    except json.JSONDecodeError:
        return []
    funcs = []
    for f in data:
        funcs.append({
            "name": f.get("name", "?"),
            "offset": f.get("addr", f.get("offset", 0)),
            "size": f.get("size", 0),
            "nbbs": f.get("nbbs", 0),
            "cc": f.get("cc", 0),
            "type": f.get("type", ""),
        })
    return funcs


def decompile_one(path: Path, func: dict, project: Path | None = None) -> str | None:
    """pdc одной функции (в отдельном процессе — параллелится)."""
    cmds = f"s {func['offset']}; pdc"
    out = run_r2(path, cmds, timeout=300, project=project)
    # убираем мусор от aa
    lines = [l for l in out.splitlines()
             if l.strip() and not l.startswith(("INFO:", "HINT:", "WARN:"))]
    body = "\n".join(lines).strip()
    if not body or "Cannot seek" in body:
        return None
    return body


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("binary", type=Path)
    ap.add_argument("-o", "--out", type=Path, help="папка вывода (по умолчанию cs1.6/analysis/<stem>)")
    ap.add_argument("--jobs", type=int, default=min(16, (os.cpu_count() or 4)))
    ap.add_argument("--min-size", type=int, default=8, help="пропускать функции меньше N байт")
    ap.add_argument("--max-funcs", type=int, default=0, help="0 = все")
    args = ap.parse_args()

    b = args.binary
    if not b.exists():
        sys.exit(f"нет файла: {b}")

    out = args.out or Path("cs1.6/analysis") / b.stem
    (out / "decomp").mkdir(parents=True, exist_ok=True)

    print(f"[*] {b} ({b.stat().st_size:,} байт)")
    proj = out / f"{b.stem}.r2proj"
    print(f"[*] анализ (aaa -> проект {proj.name}) ...")
    if not build_project(b, proj):
        print("[!] проект не создался, работаю без кэша (медленно)")
        proj = None
    funcs = list_functions(b, proj)
    print(f"[*] функций: {len(funcs)}")

    funcs_sorted = sorted(funcs, key=lambda f: -f["size"])
    selected = [f for f in funcs_sorted if f["size"] >= args.min_size]
    if args.max_funcs:
        selected = selected[: args.max_funcs]
    print(f"[*] декомпилирую {len(selected)} (jobs={args.jobs}) ...")

    # symbols
    print("[*] экспорты/импорты ...")
    sym = run_r2(b, "is; ii", timeout=600, project=proj)
    (out / "symbols.txt").write_text(sym, encoding="utf-8")

    # vtables
    print("[*] vtables ...")
    vt = run_r2(b, "avrr; avr", timeout=600, project=proj)
    (out / "vtables.txt").write_text(vt, encoding="utf-8")

    # strings with addresses
    print("[*] строки ...")
    strs = run_r2(b, "izz", timeout=600, project=proj)
    (out / "strings.txt").write_text(strs, encoding="utf-8")

    done = 0
    failed = 0
    index = []

    def work(f):
        return f, decompile_one(b, f, proj)

    with cf.ThreadPoolExecutor(max_workers=args.jobs) as ex:
        for f, code in ex.map(work, selected):
            safe = re.sub(r"[^\w.@$?-]", "_", f["name"])[:80]
            fn = out / "decomp" / f"{f['offset']:08x}_{safe}.c"
            if code:
                header = (
                    f"// {f['name']}  @ {f['offset']:#x}  size={f['size']}  "
                    f"blocks={f['nbbs']}  complexity={f['cc']}\n\n"
                )
                fn.write_text(header + code + "\n", encoding="utf-8")
                done += 1
                index.append({"name": f["name"], "offset": hex(f["offset"]),
                              "size": f["size"], "file": fn.name})
            else:
                failed += 1

    (out / "index.json").write_text(
        json.dumps({"binary": str(b), "functions": funcs, "decompiled": index},
                   indent=2, ensure_ascii=False), encoding="utf-8")

    # сводка
    summary = [
        f"# {b.name}",
        f"",
        f"- размер: {b.stat().st_size:,} байт",
        f"- функций: {len(funcs)}",
        f"- декомпилировано: {done}",
        f"- не удалось: {failed}",
        f"",
        "## Функции (по размеру)",
        "",
        "| адрес | имя | размер | блоков | сложность |",
        "| --- | --- | --- | --- | --- |",
    ]
    for f in funcs_sorted[:80]:
        summary.append(f"| `{f['offset']:#x}` | `{f['name']}` | {f['size']} | {f['nbbs']} | {f['cc']} |")
    (out / "README.md").write_text("\n".join(summary) + "\n", encoding="utf-8")

    print(f"[+] готово: {out}  ({done} декомпилировано, {failed} пропущено)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
