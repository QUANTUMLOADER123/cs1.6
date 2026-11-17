#!/usr/bin/env python3
"""embed_assets.py — встраивает бинарные ассеты меню в C-заголовки.

Зачем: DLL должна содержать НАСТОЯЩИЕ ассеты gamesense (спрайт иконок и
Tahoma Bold) внутри себя, чтобы работать без внешних файлов.

  assets/tab_icons.png  -> include/gui_icons_data.h   (g_tab_icons_png)
  assets/Tahoma-Bold.ttf -> include/gui_font_data.h   (g_tahoma_bold)

Запуск (из cs1.6/menu):  python3 ../../tools/embed_assets.py
"""
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent / "cs1.6" / "menu"
ASSETS = HERE / "assets"
INCLUDE = HERE / "include"


def embed(src: Path, dst: Path, symbol: str, comment: str) -> None:
    data = src.read_bytes()
    lines = [
        f"// {dst.name} — {comment}",
        f"// Источник: {src.relative_to(HERE)} ({len(data)} байт).",
        "// Сгенерировано tools/embed_assets.py, не редактировать вручную.",
        "#pragma once",
        "#include <cstddef>",
        "",
        f"static const unsigned char {symbol}[] = {{",
    ]
    for i in range(0, len(data), 16):
        lines.append("    " + ",".join(f"0x{b:02x}" for b in data[i:i + 16]) + ",")
    lines += ["};", f"static const size_t {symbol}_size = {len(data)};", ""]
    dst.write_text("\n".join(lines), encoding="utf-8")
    print(f"[+] {dst.relative_to(HERE)}  {len(data)} байт")


def main() -> None:
    embed(ASSETS / "tab_icons.png", INCLUDE / "gui_icons_data.h", "g_tab_icons_png",
          "ОРИГИНАЛЬНЫЙ спрайт иконок gamesense (384x48, 8 плиток 48x48).")
    embed(ASSETS / "Tahoma-Bold.ttf", INCLUDE / "gui_font_data.h", "g_tahoma_bold",
          "НАСТОЯЩИЙ Tahoma Bold — шрифт меню gamesense.")


if __name__ == "__main__":
    main()
