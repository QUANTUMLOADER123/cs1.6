#!/usr/bin/env bash
# Сборка 32-битной Windows DLL: тест-плагин меню (kiero/minhook + OpenGL2 backend).
#
# Почему zig, а не mingw: в песочнице нет apt, а zig cc умеет
# -target x86-windows-gnu и тянет свою libc из своего же кэша.
#
# Использование: ./build_win.sh
# Результат:    build/menu_test_x86.dll  (инжектить в 32-битный OpenGL процесс)
set -euo pipefail

cd "$(dirname "$0")"

# путь к zig из pip-пакета
if command -v zig >/dev/null 2>&1; then
    ZIG=zig
else
    ZIG="$(python3 -c 'import ziglang, os; print(os.path.join(os.path.dirname(ziglang.__file__), "zig"))')"
fi
echo "[*] zig: $ZIG"

OUT=build
mkdir -p "$OUT"

TARGET=x86-windows-gnu
CXXFLAGS="-target $TARGET -std=c++17 -O2 -fno-exceptions -fno-rtti
          -I. -Iinclude -Ibackends
          -Ithird_party/minhook/include
          -D_CRT_SECURE_NO_WARNINGS -DIMGUI_DISABLE_DEMO_WINDOWS"

SRC_IMGUI="imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp"
SRC_BACKENDS="backends/imgui_impl_opengl2.cpp backends/imgui_impl_win32.cpp"
SRC_MINHOOK="third_party/minhook/src/hook.c third_party/minhook/src/buffer.c
             third_party/minhook/src/trampoline.c third_party/minhook/src/hde/hde32.c"
SRC_MAIN="hook/dllmain.cpp"

echo "[*] компилирую ..."
# C-часть minhook
for f in $SRC_MINHOOK; do
    "$ZIG" cc -target $TARGET -O2 -Ithird_party/minhook/include -Ithird_party/minhook/src/hde \
        -D_CRT_SECURE_NO_WARNINGS -c "$f" -o "$OUT/$(basename "${f%.c}").o"
done

# C++ часть
for f in $SRC_IMGUI $SRC_BACKENDS $SRC_MAIN; do
    "$ZIG" c++ $CXXFLAGS -c "$f" -o "$OUT/$(basename "${f%.cpp}").o"
done

echo "[*] линкую ..."
"$ZIG" c++ -target $TARGET -shared -o "$OUT/menu_test_x86.dll" \
    "$OUT"/*.o \
    -lopengl32 -lgdi32 -luser32 -ldwmapi -limm32 -lshell32 -lole32 \
    -static-libgcc -static-libstdc++

echo "[+] готово: $OUT/menu_test_x86.dll"
ls -lh "$OUT/menu_test_x86.dll"

# ---------------------------------------------------------------------------
# Что нужно скачать один раз (build_win.sh сам не тянет):
#   git clone --depth 1 https://github.com/TsudaKageyu/minhook.git \
#       cs1.6/menu/third_party/minhook
# imgui и backends уже лежат в репозитории.
# ---------------------------------------------------------------------------
