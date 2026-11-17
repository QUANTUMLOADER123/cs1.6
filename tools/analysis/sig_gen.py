#!/usr/bin/env python3
"""
sig_gen.py — генерация и проверка сигнатур (pattern scan) по PE-бинарнику.

Правило реверса: НИ ОДНОГО хардкод-оффсета в чите. Любой адрес находится
через сигнатуру + относительное смещение от найденного места.

Как строится сигнатура:
  1. берём окно байт от целевого адреса
  2. байты, входящие в адреса/смещения (VA образа, rel32, imm32) — маскируем
  3. проверяем число вхождений во всём исполняемом сегменте
  4. цель — ровно 1 вхождение

    python3 tools/analysis/sig_gen.py client.dll --at 0x1944710 --len 24
    python3 tools/analysis/sig_gen.py client.dll --batch sigs.txt
"""
import struct
import sys
import argparse
import json

sys.path.insert(0, __file__.rsplit('/', 2)[0] + '/tools/analysis')
from xref import PE


def is_imm_like(pe, chunk):
    """Похоже ли 4 байта на абсолютный адрес внутри образа."""
    v = struct.unpack('<I', chunk)[0]
    if pe.base <= v < pe.base + 0x4000000:
        return True
    if v < 0x10000000 and v > 0x1000:
        return True
    return False


def mask_window(pe, va, length):
    """Строит (bytes, mask) для окна. mask=True означает «значимый байт»."""
    off = pe.va2off(va)
    if off is None:
        raise ValueError(f'VA 0x{va:X} вне образа')
    raw = pe.data[off:off + length]
    mask = [True] * len(raw)
    i = 0
    while i < len(raw) - 4:
        op = raw[i]
        if op in (0xE8, 0xE9):          # call/jmp rel32
            for k in range(i + 1, i + 5):
                mask[k] = False
            i += 5
            continue
        if op in (0x0F,) and i + 1 < len(raw) and raw[i + 1] in (0x84, 0x85):
            for k in range(i + 2, i + 6):   # jz/jnz rel32
                mask[k] = False
            i += 6
            continue
        if op in (0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0x68, 0xA0, 0xA3):
            # mov reg,imm32 / push imm32 / mov al,[moffs] / mov [moffs],eax
            if i + 4 < len(raw) and is_imm_like(pe, raw[i + 1:i + 5]):
                for k in range(i + 1, i + 5):
                    mask[k] = False
            i += 5
            continue
        if op == 0x8B and i + 1 < len(raw):
            modrm = raw[i + 1]
            if (modrm & 0xC7) == 0x05:      # mov reg,[disp32]
                if i + 6 <= len(raw):
                    for k in range(i + 2, i + 6):
                        mask[k] = False
                i += 6
                continue
        if op == 0x89 and i + 1 < len(raw):
            modrm = raw[i + 1]
            if (modrm & 0xC7) == 0x05:      # mov [disp32],reg
                if i + 6 <= len(raw):
                    for k in range(i + 2, i + 6):
                        mask[k] = False
                i += 6
                continue
        if op == 0x6A:                      # push imm8
            i += 2
            continue
        i += 1
    return raw, mask


def to_ida(raw, mask):
    out = []
    for b, m in zip(raw, mask):
        out.append(f'{b:02X}' if m else '??')
    return ' '.join(out)


def scan(pe, raw, mask):
    """Сколько раз сигнатура встречается в исполняемых секциях."""
    hits = []
    first = None
    for s in pe.secs:
        if not (s['chars'] & 0x20000000):
            continue
        data = pe.data
        start, end = s['raw'], s['raw'] + s['rsize']
        i = start
        while True:
            i = data.find(bytes([raw[0]]), i, end)
            if i < 0:
                break
            ok = True
            for k in range(1, len(raw)):
                if mask[k] and data[i + k] != raw[k]:
                    ok = False
                    break
            if ok:
                rva = pe.off2rva(i)
                hits.append(pe.base + rva)
                if len(hits) > 400:
                    return hits
            i += 1
    return hits


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('binary')
    ap.add_argument('--at', type=lambda x: int(x, 0), nargs='+', default=[])
    ap.add_argument('--len', type=int, default=24)
    ap.add_argument('--name', default='')
    ap.add_argument('--min', type=int, default=10)
    ap.add_argument('--json')
    args = ap.parse_args()

    pe = PE(args.binary)
    results = []
    for va in args.at:
        length = args.len
        found = None
        for L in range(args.min, 64):
            raw, mask = mask_window(pe, va, L)
            sig = to_ida(raw, mask)
            hits = scan(pe, raw, mask)
            if len(hits) == 1:
                found = (sig, hits, L)
                break
            if len(hits) == 0:
                found = (sig, hits, L)
                break
        sig, hits, L = found
        status = 'УНИКАЛЬНА' if len(hits) == 1 else (
            f'{len(hits)} вхождений' if hits else 'НЕ НАЙДЕНА (битая маска)')
        print(f'VA 0x{va:08X}  len={L:2d}  {status}')
        print(f'   {sig}')
        if len(hits) > 1:
            print(f'   попадания: ' + ', '.join(f'0x{h:08X}' for h in hits[:6]))
        results.append(dict(va=va, name=args.name, len=L, sig=sig,
                            hits=len(hits), unique=len(hits) == 1))

    if args.json:
        json.dump(results, open(args.json, 'w'), indent=1)


if __name__ == '__main__':
    main()
