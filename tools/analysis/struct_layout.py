#!/usr/bin/env python3
"""
struct_layout.py — считает раскладку C-структур (32-bit, MSVC, agp=4)
из заголовков pm_shared и сверяет их с дизассемблером client.dll.

Использование:
    python3 tools/analysis/struct_layout.py --header /home/user/cs1.6-ref/pm_shared/pm_defs.h \
                                            --struct playermove_t physent_t

Зачем: если оффсеты, посчитанные из исходников, совпадают с константами
в машинном коде — значит бинарник и исходники это один и тот же код,
и оффсеты можно использовать как проверенный контракт.
"""
import re
import sys
import argparse

# Базовые типы: имя -> (размер, выравнивание)
PRIMS = {
    'char': (1, 1), 'byte': (1, 1), 'unsigned char': (1, 1),
    'short': (2, 2), 'unsigned short': (2, 2), 'wchar_t': (2, 2),
    'int': (4, 4), 'unsigned int': (4, 4), 'long': (4, 4),
    'float': (4, 4), 'vec_t': (4, 4), 'qboolean': (4, 4),
    'double': (8, 8),
    # указатели и функции — 4 байта на x86
    'void *': (4, 4), 'char *': (4, 4), 'int *': (4, 4), 'float *': (4, 4),
}

VEC3 = ['vec3_t']


def strip_comments(text: str) -> str:
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)
    return text


def extract_struct(text: str, name: str):
    """Достаёт тело `typedef struct ... { ... } name;`"""
    pat = re.compile(r'typedef\s+struct[^;{]*\{', re.S)
    for m in pat.finditer(text):
        start = m.end()
        depth = 1
        i = start
        while i < len(text) and depth:
            if text[i] == '{':
                depth += 1
            elif text[i] == '}':
                depth -= 1
            i += 1
        tail = text[i:i + 200]
        if re.match(r'\s*' + re.escape(name) + r'\s*;', tail):
            return text[start:i - 1]
    return None


def parse_fields(body: str):
    """Разбирает поля: ('name', type, count_or_None)."""
    body = re.sub(r'#if.*?#endif', '', body, flags=re.S)
    out = []
    for raw in body.split(';'):
        decl = ' '.join(raw.split())
        if not decl or decl.startswith('typedef'):
            continue
        # Указатель на функцию: тип (*имя)(аргументы)
        m = re.match(r'^(.*?)\(\s*\*\s*([A-Za-z_]\w*)\s*\)\s*\(', decl)
        if m:
            out.append((m.group(2), 'void *', None))
            continue

        # Может быть несколько деклараторов через запятую:
        #   vec3_t forward, right, up;
        parts = _split_decl(decl)
        if not parts:
            continue
        first, rest = parts[0], parts[1:]

        m = re.match(r'^(.*?)([A-Za-z_]\w*)\s*((?:\[[^\]]*\]\s*)*)$', first.strip())
        if not m:
            continue
        ctype = m.group(1).strip()
        base_dims = re.findall(r'\[([^\]]*)\]', m.group(3) or '')

        # базовый тип мог содержать звёздочку: char *a, *b  → тип «char *»
        star = ctype.rstrip().endswith('*')
        ctype = ctype.rstrip().rstrip('*').strip() + (' *' if star else '')

        def mk(dims):
            if not dims:
                return None
            try:
                n = 1
                for d in dims:
                    n *= int(d)
                return n
            except ValueError:
                return 1

        out.append((m.group(2), ctype, mk(base_dims)))
        for d in rest:
            dm = re.match(r'^\s*\*?\s*([A-Za-z_]\w*)\s*((?:\[[^\]]*\]\s*)*)$', d)
            if dm:
                out.append((dm.group(1), ctype, mk(re.findall(r'\[([^\]]*)\]', dm.group(2) or ''))))
    return out


def _split_decl(decl):
    """Делит «vec3_t a, b[2], *c» на части по запятым верхнего уровня."""
    parts, depth, cur = [], 0, ''
    for ch in decl:
        if ch in '[(':
            depth += 1
        elif ch in '])':
            depth -= 1
        if ch == ',' and depth == 0:
            parts.append(cur)
            cur = ''
        else:
            cur += ch
    if cur.strip():
        parts.append(cur)
    return parts


def type_size(ctype, structs, seen):
    ctype = ctype.replace('struct ', '').replace('const ', '').strip()
    if ctype.endswith('*'):
        return 4, 4
    if ctype in PRIMS:
        return PRIMS[ctype]
    if ctype in VEC3:
        return 12, 4
    if ctype in ('model_t', 'hull_t', 'movevars_t') or ctype.endswith('_s'):
        return 4, 4  # forward-declared / берём как указатель
    if ctype in structs:
        if ctype in seen:
            return 4, 4
        size, align, _ = layout(ctype, structs, seen | {ctype})
        return size, align
    return 4, 4


def layout(name, structs, seen=frozenset()):
    body = structs[name]
    fields = parse_fields(body)
    off = 0
    maxalign = 1
    rows = []
    for fname, ctype, count in fields:
        size, align = type_size(ctype, structs, seen | {name})
        maxalign = max(maxalign, align)
        pad = (-off) % align
        off += pad
        total = size * (count or 1)
        rows.append((off, fname, ctype, count, total))
        off += total
    total = off + ((-off) % maxalign)
    return total, maxalign, rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--header', required=True, nargs='+')
    ap.add_argument('--struct', nargs='+', required=True)
    ap.add_argument('--check', nargs='*', default=[],
                    help='имя=0xOFFSET для сверки с бинарником')
    args = ap.parse_args()

    text = ''
    for h in args.header:
        text += strip_comments(open(h, encoding='utf-8', errors='replace').read()) + '\n'

    structs = {}
    # вариант 1: struct TAG { ... };  (без typedef)
    for m in re.finditer(r'\bstruct\s+(\w+)\s*\{', text):
        start = m.end()
        depth, i = 1, start
        while i < len(text) and depth:
            if text[i] == '{':
                depth += 1
            elif text[i] == '}':
                depth -= 1
            i += 1
        if m.group(1) not in structs:
            structs[m.group(1)] = text[start:i - 1]

    for m in re.finditer(r'typedef\s+struct[^;{]*\{', text, re.S):
        start = m.end()
        depth, i = 1, start
        while i < len(text) and depth:
            if text[i] == '{':
                depth += 1
            elif text[i] == '}':
                depth -= 1
            i += 1
        mt = re.match(r'\s*([A-Za-z_]\w*)\s*;', text[i:i + 120])
        if mt and mt.group(1) not in structs:
            structs[mt.group(1)] = text[start:i - 1]

    # обратный маппинг: если "typedef struct X_s {...};" без typedef-имени,
    # то X_t (из typedef struct X_s X_t;) должен ссылаться на X_s
    for m in re.finditer(r'typedef\s+struct\s+(\w+)\s+(\w+)\s*;', text):
        tag, alias = m.group(1), m.group(2)
        if tag in structs and alias not in structs:
            structs[alias] = structs[tag]

    for s in args.struct:
        if s not in structs:
            print(f'!! структура {s} не найдена')
            continue
        total, align, rows = layout(s, structs)
        print(f'=== {s}: size = {total} (0x{total:X}), align = {align}, полей {len(rows)}')
        for off, fname, ctype, count, total_sz in rows:
            arr = f'[{count}]' if count else ''
            print(f'  +0x{off:04X}  {ctype:22s} {fname}{arr:12s} ({total_sz} Б)')
        print()

    # сверка
    checks = {}
    for c in args.check:
        k, v = c.split('=')
        checks[k] = int(v, 0)
    if checks:
        print('=== сверка с бинарником')
        ok = bad = 0
        for k, want in checks.items():
            found = None
            for s in args.struct:
                if s not in structs:
                    continue
                _, _, rows = layout(s, structs)
                for off, fname, ctype, count, _ in rows:
                    if f'{s}.{fname}' == k or fname == k:
                        found = (off, s)
            if found is None:
                print(f'  ?? {k:32s} не найдено в заголовках')
                continue
            got, s = found
            mark = 'OK ' if got == want else 'BAD'
            if got == want:
                ok += 1
            else:
                bad += 1
            print(f'  {mark} {k:32s} заголовок=0x{got:04X}  бинарник=0x{want:04X}')
        print(f'  итого: {ok} совпало, {bad} расхождений')


if __name__ == '__main__':
    main()
