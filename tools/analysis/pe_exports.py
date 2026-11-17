#!/usr/bin/env python3
"""
pe_exports.py — дампит таблицу экспорта PE-файла: имена, ординалы, RVA, VA.

Для реверса это скелет: экспорт = точки входа, через которые движок
и игровые DLL общаются друг с другом.

    python3 tools/analysis/pe_exports.py client.dll hw.dll mp.dll -o cs1.6/reverse/exports
"""
import struct
import sys
import os
import json
import argparse


def parse_pe(data):
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('не PE-файл')
    machine, nsec = struct.unpack_from('<HH', data, pe + 4)
    opt = pe + 24
    magic = struct.unpack_from('<H', data, opt)[0]
    pe32plus = magic == 0x20b
    base = struct.unpack_from('<I', data, opt + (24 if pe32plus else 28))[0]
    dd = opt + (112 if pe32plus else 96)
    dirs = [struct.unpack_from('<II', data, dd + i * 8) for i in range(16)]
    secs = []
    so = opt + struct.unpack_from('<H', data, pe + 20)[0]
    for i in range(nsec):
        o = so + i * 40
        name = data[o:o + 8].rstrip(b'\0').decode('latin-1')
        vs, va, rs, ra = struct.unpack_from('<IIII', data, o + 8)
        chars = struct.unpack_from('<I', data, o + 36)[0]
        secs.append(dict(name=name, va=va, vsize=vs, raw=ra, rsize=rs, chars=chars))
    return dict(base=base, secs=secs, dirs=dirs, machine=machine, pe32plus=pe32plus)


def rva2off(pe, rva):
    for s in pe['secs']:
        span = max(s['vsize'], s['rsize'])
        if s['va'] <= rva < s['va'] + span:
            delta = rva - s['va']
            if delta >= s['rsize']:
                return None
            return s['raw'] + delta
    return None


def cstr(data, off):
    if off is None:
        return ''
    end = data.index(b'\0', off)
    return data[off:end].decode('latin-1')


def exports(data, pe):
    erva, esize = pe['dirs'][0]
    if erva == 0:
        return [],
    eo = rva2off(pe, erva)
    (chars, ts, maj, mnr, name_rva, base_ord, nfun, nnam,
     afun, anam, aord) = struct.unpack_from('<IIHHIIIIIII', data, eo)
    out = []
    no = rva2off(pe, anam)
    ao = rva2off(pe, aord)
    fo = rva2off(pe, afun)
    for i in range(nnam):
        nr = struct.unpack_from('<I', data, no + i * 4)[0]
        name = cstr(data, rva2off(pe, nr))
        oi = struct.unpack_from('<H', data, ao + i * 2)[0]
        fr = struct.unpack_from('<I', data, fo + oi * 4)[0]
        is_fwd = erva <= fr < erva + esize
        out.append(dict(name=name, ordinal=base_ord + oi, rva=fr,
                        va=pe['base'] + fr,
                        forward=cstr(data, rva2off(pe, fr)) if is_fwd else None))
    out.sort(key=lambda x: x['ordinal'])
    return out


def imports(data, pe):
    """Импорты: из какой DLL берутся функции (что нужно движку)."""
    if not pe['dirs'][1][0]:
        return []
    res = []
    io = rva2off(pe, pe['dirs'][1][0])
    i = 0
    while True:
        oft, ts, fc, nrva, ft = struct.unpack_from('<IIIII', data, io + i * 20)
        if nrva == 0:
            break
        dll = cstr(data, rva2off(pe, nrva))
        names = []
        t = oft or ft
        j = 0
        while True:
            ent = struct.unpack_from('<I', data, rva2off(pe, t) + j * 4)[0]
            if ent == 0:
                break
            if not (ent & 0x80000000):
                nm = cstr(data, rva2off(pe, ent) + 2)
                names.append(nm)
            j += 1
        res.append(dict(dll=dll, count=len(names), funcs=names))
        i += 1
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('files', nargs='+')
    ap.add_argument('-o', '--outdir')
    ap.add_argument('--imports', action='store_true')
    args = ap.parse_args()

    if args.outdir:
        os.makedirs(args.outdir, exist_ok=True)
    md = []

    for path in args.files:
        data = open(path, 'rb').read()
        pe = parse_pe(data)
        exp = exports(data, pe)
        name = os.path.basename(path)
        print(f'=== {name}: {len(exp)} экспортов, ImageBase=0x{pe["base"]:08X}')
        for e in exp:
            print(f'  {e["ordinal"]:4d} 0x{e["va"]:08X}  {e["name"]}'
                  + (f'  -> {e["forward"]}' if e['forward'] else ''))
        if args.outdir:
            stem = os.path.splitext(name)[0]
            with open(os.path.join(args.outdir, f'{stem}_exports.txt'), 'w') as f:
                f.write(f'# {name}  ImageBase=0x{pe["base"]:08X}  секций {len(pe["secs"])}\n')
                for s in pe['secs']:
                    f.write(f'#   {s["name"]:8s} VA=0x{s["va"]:08X} vsize=0x{s["vsize"]:X} '
                            f'raw=0x{s["raw"]:X} rsize=0x{s["rsize"]:X}\n')
                f.write('#\n# ord   VA          имя\n')
                for e in exp:
                    f.write(f'{e["ordinal"]:4d} 0x{e["va"]:08X}  {e["name"]}\n')
            with open(os.path.join(args.outdir, f'{stem}_exports.json'), 'w') as f:
                json.dump(dict(binary=name, image_base=pe['base'],
                               sections=pe['secs'], exports=exp), f, indent=1)

        if args.imports:
            im = imports(data, pe)
            print(f'    импорты: {len(im)} DLL')
            for d in im:
                print(f'      {d["dll"]:24s} {d["count"]} функций')
            if args.outdir:
                stem = os.path.splitext(name)[0]
                with open(os.path.join(args.outdir, f'{stem}_imports.txt'), 'w') as f:
                    for d in im:
                        f.write(f'\n== {d["dll"]} ({d["count"]})\n')
                        for fn in d['funcs']:
                            f.write(f'   {fn}\n')
        md.append((name, pe, exp))
        print()

    if args.outdir:
        idx = os.path.join(args.outdir, 'README.md')
        with open(idx, 'w') as f:
            f.write('# Таблицы экспорта бинарников CS 1.6\n\n')
            f.write('Скелет игры: каждая строка — точка входа, объявленная наружу.\n\n')
            for name, pe, exp in md:
                f.write(f'## {name}\n\nImageBase `0x{pe["base"]:08X}`, экспортов **{len(exp)}**\n\n')
        print(f'записано в {args.outdir}')


if __name__ == '__main__':
    main()
