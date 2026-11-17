#!/usr/bin/env python3
"""compare_reference.py — попиксельная сверка порта меню с оригиналом gamesense.

Сравнивает скриншот нашего меню (731x617, нативный режим) с референсом и
печатает среднюю разницу и долю точно совпадающих пикселей.

  cd cs1.6/menu && ./menu.png shots/skins.png 1 5
  python3 ../../tools/compare_reference.py
"""
import sys
from pathlib import Path

from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parent.parent
REF = ROOT / "image-search" / "gamesense-skeet-csgo-cheat-menu-gui-1.png"
MINE = ROOT / "cs1.6" / "menu" / "shots" / "skins.png"


def main() -> int:
    ref_path = Path(sys.argv[1]) if len(sys.argv) > 1 else REF
    mine_path = Path(sys.argv[2]) if len(sys.argv) > 2 else MINE

    ref = Image.open(ref_path).convert("RGB")
    mine = Image.open(mine_path).convert("RGB")
    if ref.size != mine.size:
        print(f"[!] размеры не совпадают: {ref.size} vs {mine.size}")
        return 1

    diff = ImageChops.difference(ref, mine)
    vals = [sum(p) / 3 for p in diff.get_flattened_data()]
    mean = sum(vals) / len(vals)
    exact = sum(1 for v in vals if v <= 1.5) / len(vals) * 100
    off = sum(1 for v in vals if v > 30) / len(vals) * 100

    print(f"размер:        {ref.size[0]}x{ref.size[1]}")
    print(f"средняя разница: {mean:.2f} / 255")
    print(f"в пределах ±1/255: {exact:.2f} %")
    print(f"сильно отлично:  {off:.2f} % (антиалиасинг глифов)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
