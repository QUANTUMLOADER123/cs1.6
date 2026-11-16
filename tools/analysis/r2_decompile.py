#!/usr/bin/env python3
"""
r2_decompile.py — прогон radare2 по DLL и выгрузка декомпиляции всех функций.

Работает через один долгоживущий процесс r2 (pipe-режим `r2 -q -`), поэтому
анализ `aaa` делается ровно один раз, а не на каждую функцию. Это в сотни раз
быстрее запуска отдельного процесса на функцию.

Что выгружает в <out>/:
  decomp/<addr>_<name>.c   — pdc-декомпиляция каждой функции
  symbols.txt              — экспорты и импорты с адресами
  vtables.txt              — vtables (C++ классы)
  strings.txt              — строки с адресами
  index.json               — машинный индекс
  README.md                — сводка: топ функций по размеру

Использование:
  python3 tools/analysis/r2_decompile.py hw.dll
  python3 tools/analysis/r2_decompile.py hw.dll -o out --min-size 32
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

R2 = os.environ.get("R2", "r2")


class R2Pipe:
    """Клиент radare2 в pipe-режиме через PTY.

    Почему PTY, а не обычные pipe'ы: r2 определяет, что stdout не терминал,
    и уходит в полностью буферизованный вывод — ответы приходят пачкой в конце.
    С псевдотерминалом буферизация построчная, и диалог идёт в реальном времени.
    """

    def __init__(self, path: Path):
        import pty
        self.master, slave = pty.openpty()
        self.p = subprocess.Popen(
            [R2, "-q", "-e", "scr.color=0", "-e", "bin.relocs.apply=true", "-N", str(path)],
            stdin=slave, stdout=slave, stderr=slave, close_fds=True)
        os.close(slave)
        # ждём, пока r2 поднимется и загрузит бинарник: иначе первые команды
        # уходят в пустоту, пока идёт инициализация
        self._warmup()

    def _warmup(self, timeout: float = 120.0) -> None:
        import select
        deadline = time.time() + timeout
        while time.time() < deadline:
            r, _, _ = select.select([self.master], [], [], 0.3)
            if r:
                try:
                    if os.read(self.master, 65536):
                        break
                except OSError:
                    break
        # пробный запрос: должен вернуть непустой ответ
        for _ in range(5):
            probe = self._raw("iI", timeout=30)
            if "arch" in probe or "bits" in probe:
                return
            time.sleep(0.3)

    def _raw(self, command: str, timeout: float = 600.0) -> str:
        marker = b"__R2_MARK_%d__" % (int(time.time() * 1000) % 100000)
        try:
            os.write(self.master, command.encode() + b"\n?e " + marker + b"\n")
        except OSError:
            return ""
        import select
        buf = b""
        deadline = time.time() + timeout
        while time.time() < deadline:
            r, _, _ = select.select([self.master], [], [], 1.0)
            if not r:
                if self.p.poll() is not None:
                    break
                continue
            try:
                chunk = os.read(self.master, 65536)
            except OSError:
                break
            if not chunk:
                break
            buf += chunk
            if marker in buf:
                break
        return buf.decode("utf-8", "replace")

    def cmd(self, command: str, timeout: float = 600.0) -> str:
        import select
        marker = b"__R2_MARK_%d__" % (int(time.time() * 1000) % 100000)
        try:
            os.write(self.master, command.encode() + b"\n?e " + marker + b"\n")
        except OSError:
            return ""

        buf = b""
        deadline = time.time() + timeout
        while time.time() < deadline:
            r, _, _ = select.select([self.master], [], [], 1.0)
            if not r:
                if self.p.poll() is not None:
                    break
                continue
            try:
                chunk = os.read(self.master, 65536)
            except OSError:
                break
            if not chunk:
                break
            buf += chunk
            if marker in buf:
                break

        text = buf.decode("utf-8", "replace")
        text = text.replace(marker.decode(), "")
        # эхо команды назад + escape-последовательности терминала
        text = re.sub(r"\x1b\[[0-9;?]*[a-zA-Z]", "", text)
        lines = text.splitlines()
        # PTY эхо: первая строка может быть остатком предыдущей команды
        # или эхом текущей. Отбрасываем всё, что похоже на «s <адрес>».
        if lines:
            first = lines[0].strip()
            if first == command.strip() or re.fullmatch(r"s\s+(0x)?[0-9a-fA-F]+", first):
                lines = lines[1:]
        # и ещё раз: иногда эхо протекает вторым куском
        while lines and re.fullmatch(r"(s|pdc|aaa)\s*(0x)?[0-9a-fA-F]*", lines[0].strip()):
            lines = lines[1:]
        if command.strip() in ("aflj", "afij", "iij", "iSj"):
            # JSON отдаём как есть: фильтрация по префиксам может испортить структуру
            return text.strip()
        lines = [l for l in lines if l.strip() and not l.startswith(("INFO:", "HINT:", "WARN:"))]
        return "\n".join(lines)

    def close(self):
        try:
            os.write(self.master, b"q\n")
            self.p.wait(timeout=10)
        except Exception:
            self.p.kill()
        finally:
            try:
                os.close(self.master)
            except OSError:
                pass


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("binary", type=Path)
    ap.add_argument("-o", "--out", type=Path)
    ap.add_argument("--min-size", type=int, default=8)
    ap.add_argument("--max-funcs", type=int, default=0)
    ap.add_argument("--limit", type=int, default=0, help="взять только N самых крупных (для теста)")
    args = ap.parse_args()

    b = args.binary
    if not b.exists():
        sys.exit(f"нет файла: {b}")

    out = args.out or Path("cs1.6/analysis") / b.stem
    (out / "decomp").mkdir(parents=True, exist_ok=True)

    print(f"[*] {b} ({b.stat().st_size:,} байт)")
    print("[*] поднимаю r2 ...")
    r2 = R2Pipe(b)
    try:
        t0 = time.time()
        print("[*] aaa ...")
        r2.cmd("aaa", timeout=3600)
        print(f"    заняло {time.time() - t0:.1f}с")

        # aaa иногда ещё дописывает вывод, когда мы уже просим aflj:
        # повторяем, пока не получим валидный JSON
        raw = ""
        funcs = []
        for attempt in range(5):
            raw = r2.cmd("aflj", timeout=600)
            if raw.find("[") >= 0 and raw.rfind("]") > raw.find("["):
                break
            print(f"    [!] aflj пуст (попытка {attempt + 1}), жду ...")
            time.sleep(2)
        start, end = raw.find("["), raw.rfind("]")
        funcs = []
        if start >= 0 and end > start:
            try:
                for f in json.loads(raw[start:end + 1]):
                    funcs.append({
                        "name": f.get("name", "?"),
                        "offset": f.get("addr", 0),
                        "size": f.get("size", 0),
                        "nbbs": f.get("nbbs", 0),
                        "cc": f.get("cc", 0),
                    })
            except json.JSONDecodeError as e:
                print(f"[!] JSON не разобрался: {e}")
        print(f"[*] функций: {len(funcs)}")

        funcs_sorted = sorted(funcs, key=lambda f: -f["size"])
        selected = [f for f in funcs_sorted if f["size"] >= args.min_size]
        if args.limit:
            selected = selected[: args.limit]
        if args.max_funcs:
            selected = selected[: args.max_funcs]
        print(f"[*] декомпилирую {len(selected)} ...")

        # служебные выгрузки — до декомпиляции, они быстрые
        print("[*] символы ...")
        (out / "symbols.txt").write_text(r2.cmd("is; ii", 600), encoding="utf-8")
        print("[*] vtables ...")
        (out / "vtables.txt").write_text(r2.cmd("avrr; avr", 900), encoding="utf-8")
        print("[*] строки ...")
        (out / "strings.txt").write_text(r2.cmd("izz", 900), encoding="utf-8")

        done, failed = 0, 0
        index = []
        t0 = time.time()
        for i, f in enumerate(selected):
            code = r2.cmd(f"s {f['offset']}; pdc", timeout=300)
            safe = re.sub(r"[^\w.@$?-]", "_", f["name"])[:80]
            fn = out / "decomp" / f"{f['offset']:08x}_{safe}.c"
            if code and "Cannot seek" not in code:
                header = (f"// {f['name']}  @ {f['offset']:#x}  size={f['size']}  "
                          f"blocks={f['nbbs']}  complexity={f['cc']}\n\n")
                fn.write_text(header + code + "\n", encoding="utf-8")
                done += 1
                index.append({"name": f["name"], "offset": hex(f["offset"]),
                              "size": f["size"], "file": fn.name})
            else:
                failed += 1
            if (i + 1) % 50 == 0 or i + 1 == len(selected):
                rate = (i + 1) / max(time.time() - t0, 0.001)
                eta = (len(selected) - i - 1) / rate if rate > 0 else 0
                print(f"    {i+1}/{len(selected)}  {rate:.1f} ф/с  ETA {eta/60:.1f} мин", flush=True)

        (out / "index.json").write_text(
            json.dumps({"binary": str(b), "functions": funcs, "decompiled": index},
                       indent=2, ensure_ascii=False), encoding="utf-8")

        summary = [f"# {b.name}", "",
                   f"- размер: {b.stat().st_size:,} байт",
                   f"- функций: {len(funcs)}",
                   f"- декомпилировано: {done}",
                   f"- пропущено: {failed}", "",
                   "## Крупнейшие функции", "",
                   "| адрес | имя | размер | блоков | сложность |",
                   "| --- | --- | --- | --- | --- |"]
        for f in funcs_sorted[:100]:
            summary.append(f"| `{f['offset']:#x}` | `{f['name']}` | {f['size']} | "
                           f"{f['nbbs']} | {f['cc']} |")
        (out / "README.md").write_text("\n".join(summary) + "\n", encoding="utf-8")

        print(f"[+] готово: {out}  ({done} декомпилировано, {failed} пропущено)")
    finally:
        r2.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
