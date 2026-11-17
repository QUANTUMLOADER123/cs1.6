#!/usr/bin/env python3
"""
xref.py — поиск строки в PE и кросс-ссылок на неё из кода.

Стратегия реверса без дизассемблера:
  1. найти строку в образе → её VA
  2. найти в .text инструкции `push imm32` / `mov reg, imm32` с этим VA
  3. вокруг этих мест — та функция, что использует строку

    python3 tools/analysis/xref.py mp.dll CAK47
    python3 tools/analysis/xref.py mp.dll --list-fire
"""
import struct
import sys
import argparse


class PE:
    def __init__(self, path):
        self.data = path if isinstance(path, bytes) else open(path, 'rb').read()
        d = self.data
        pe = struct.unpack_from('<I', d, 0x3c)[0]
        nsec = struct.unpack_from('<H', d, pe + 6)[0]
        opt = pe + 24
        magic = struct.unpack_from('<H', d, opt)[0]
        self.plus = magic == 0x20b
        self.base = struct.unpack_from('<I', d, opt + (24 if self.plus else 28))[0]
        dd = opt + (112 if self.plus else 96)
        self.dirs = [struct.unpack_from('<II', d, dd + i * 8) for i in range(16)]
        so = opt + struct.unpack_from('<H', d, pe + 20)[0]
        self.secs = []
        for i in range(nsec):
            o = so + i * 40
            name = d[o:o + 8].rstrip(b'\0').decode('latin-1')
            vs, va, rs, ra = struct.unpack_from('<IIII', d, o + 8)
            chars = struct.unpack_from('<I', d, o + 36)[0]
            self.secs.append(dict(name=name, va=va, vsize=vs, raw=ra,
                                  rsize=rs, chars=chars))

    def rva2off(self, rva):
        for s in self.secs:
            span = max(s['vsize'], s['rsize'])
            if s['va'] <= rva < s['va'] + span:
                delta = rva - s['va']
                if delta >= s['rsize']:
                    return None
                return s['raw'] + delta
        return None

    def off2rva(self, off):
        for s in self.secs:
            if s['raw'] <= off < s['raw'] + s['rsize']:
                return s['va'] + (off - s['raw'])
        return None

    def va2off(self, va):
        return self.rva2off(va - self.base)

    def sec(self, name):
        for s in self.secs:
            if s['name'] == name:
                return s
        return None

    def find_str(self, text):
        """Все вхождения строки → список (VA, смещение)."""
        pat = text.encode() if isinstance(text, str) else text
        out, start = [], 0
        while True:
            i = self.data.find(pat, start)
            if i < 0:
                break
            rva = self.off2rva(i)
            out.append((self.base + rva if rva is not None else None, i))
            start = i + 1
            if len(out) > 50:
                break
        return out

    def code_xrefs(self, va):
        """Ищем push/mov imm32 = VA в исполняемых секциях."""
        pat = struct.pack('<I', va)
        out = []
        for s in self.secs:
            if not (s['chars'] & 0x20000000):   # IMAGE_SCN_MEM_EXECUTE
                continue
            start = s['raw']
            end = s['raw'] + s['rsize']
            i = start
            while True:
                i = self.data.find(pat, i, end)
                if i < 0:
                    break
                op = self.data[i - 1] if i > start else None
                kind = {0x68: 'push', 0xb8: 'mov eax', 0xb9: 'mov ecx',
                        0xba: 'mov edx', 0xbb: 'mov ebx', 0xbe: 'mov esi',
                        0xbf: 'mov edi', 0x35: 'xor eax'}.get(op, f'op{op:02x}')
                rva = self.off2rva(i - 1)
                out.append(dict(va_at=self.base + rva, kind=kind,
                                off=i, sec=s['name']))
                i += 1
        return out

    def data_xrefs(self, va):
        """Ищем указатель на VA в неисполняемых секциях (таблицы, vtable)."""
        pat = struct.pack('<I', va)
        out = []
        for s in self.secs:
            if s['chars'] & 0x20000000:
                continue
            i = s['raw']
            end = s['raw'] + s['rsize']
            while True:
                i = self.data.find(pat, i, end)
                if i < 0:
                    break
                rva = self.off2rva(i)
                out.append(dict(va_at=self.base + rva, sec=s['name']))
                i += 1
        return out

    def find_floats(self, value):
        """Где встречается float-константа."""
        pat = struct.pack('<f', value)
        out = []
        for s in self.secs:
            i = s['raw']
            end = s['raw'] + s['rsize']
            while True:
                i = self.data.find(pat, i, end)
                if i < 0:
                    break
                rva = self.off2rva(i)
                out.append(dict(va_at=self.base + rva, sec=s['name']))
                i += 4
        return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('binary')
    ap.add_argument('needle', nargs='?')
    ap.add_argument('--data-xref', action='store_true')
    ap.add_argument('--float', type=float, nargs='*', default=[])
    ap.add_argument('--limit', type=int, default=12)
    args = ap.parse_args()

    pe = PE(args.binary)
    print(f'{args.binary}: ImageBase=0x{pe.base:08X}')

    if args.float:
        for v in args.float:
            hits = pe.find_floats(v)
            print(f'\nfloat {v}: {len(hits)} вхождений')
            for h in hits[:args.limit]:
                print(f"  0x{h['va_at']:08X}  {h['sec']}")
        return

    if not args.needle:
        for s in pe.secs:
            print(f"  {s['name']:9s} VA=0x{pe.base+s['va']:08X} "
                  f"vsize=0x{s['vsize']:X} raw=0x{s['raw']:X}")
        return

    hits = pe.find_str(args.needle)
    print(f'строка "{args.needle}": {len(hits)} вхождений')
    for va, off in hits:
        print(f'  VA=0x{va:08X}  файл=0x{off:X}')
        xr = pe.code_xrefs(va)
        for x in xr[:args.limit]:
            print(f'      <- код 0x{x["va_at"]:08X}  ({x["kind"]})')
        if not xr:
            print('      <- ссылок из кода нет')
        if args.data_xref:
            for x in pe.data_xrefs(va)[:args.limit]:
                print(f'      <= данные 0x{x["va_at"]:08X} ({x["sec"]})')


if __name__ == '__main__':
    main()
