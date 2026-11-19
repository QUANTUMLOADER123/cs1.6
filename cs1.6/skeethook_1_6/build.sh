#!/usr/bin/env bash
# Сборка skeethook 1.6 (база: gold-sdk).
#
# Собираем через zig cc/c++ под x86 (32-бит!): CS 1.6 - 32-битный
# процесс, под x86_64 структуры выходят на 16 байт больше и все
# смещения уезжают (проверено: cl_entity_t 3016 вместо 3000).
set -u

cd "$(dirname "$0")" || exit 1

if command -v zig >/dev/null 2>&1; then
    ZIG=zig
else
    ZIG=$(python3 -c 'import ziglang,os;print(os.path.join(os.path.dirname(ziglang.__file__),"zig"))')
fi

OUT=build
T=x86-windows-gnu

CF="-target $T -O2 -w -Iinclude/minhook/include -Iinclude/minhook/src/hde"
XF="-target $T -std=c++20 -O2 -w -Iinclude -Isrc -Iinclude/minhook/include -Iinclude/imgui"

rm -rf "$OUT"
mkdir -p "$OUT"

n=0
fail=0

compile() {
    local f="$1"
    local o="$OUT/$(echo "$f" | tr '/' '_' | sed 's/\.\(cpp\|c\)$/.o/')"
    case "$f" in
        *.c) "$ZIG" cc $CF -c "$f" -o "$o" 2>"$OUT/err.txt" ;;
        *)   "$ZIG" c++ $XF -c "$f" -o "$o" 2>"$OUT/err.txt" ;;
    esac
    if [ -f "$o" ]; then
        n=$((n + 1))
    else
        fail=$((fail + 1))
        echo "  FAIL: $f"
        head -4 "$OUT/err.txt"
    fi
}

# C: MinHook
for f in include/minhook/src/buffer.c include/minhook/src/hook.c \
         include/minhook/src/trampoline.c include/minhook/src/hde/hde32.c; do
    compile "$f"
done

# C++: ImGui + бэкенд
for f in include/imgui/imgui.cpp include/imgui/imgui_draw.cpp \
         include/imgui/imgui_tables.cpp include/imgui/imgui_widgets.cpp \
         include/imgui/backend/imgui_impl_opengl.cpp; do
    compile "$f"
done

# C++: проект
for f in $(find src -name '*.cpp' | sort); do
    compile "$f"
done

echo "[*] объектов: $n, ошибок: $fail"

if [ "$fail" -ne 0 ]; then
    echo "[-] сборка остановлена"
    exit 1
fi

# -static-libgcc -static-libstdc++: библиотеки gcc/стдлиба вшиваются в DLL.
# Без них DLL тянет api-ms-win-crt-*.dll и на машине без
# "Visual C++ Redistributable" просто не загрузится.
"$ZIG" c++ -target $T -shared -o "$OUT/skeethook_1_6.dll" "$OUT"/*.o \
    -lopengl32 -lgdi32 -luser32 -lpsapi \
    -static-libgcc -static-libstdc++ 2>"$OUT/link.txt"

if [ -f "$OUT/skeethook_1_6.dll" ]; then
    echo "[+] $(du -h "$OUT/skeethook_1_6.dll" | cut -f1) $OUT/skeethook_1_6.dll"
    sha256sum "$OUT/skeethook_1_6.dll" | cut -d' ' -f1
else
    echo "[-] линковка не удалась"
    head -10 "$OUT/link.txt"
    exit 1
fi
