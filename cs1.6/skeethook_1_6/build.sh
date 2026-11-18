#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

if command -v zig >/dev/null 2>&1; then ZIG=zig
else ZIG=$(python3 -c 'import ziglang,os;print(os.path.join(os.path.dirname(ziglang.__file__),"zig"))'); fi

OUT=build
rm -rf "$OUT" && mkdir -p "$OUT"
T=x86-windows-gnu

CF="-target $T -O2 -Iext/minhook/include -Iext/minhook/src/hde -D_CRT_SECURE_NO_WARNINGS"
for f in ext/minhook/src/hook.c ext/minhook/src/buffer.c ext/minhook/src/trampoline.c ext/minhook/src/hde/hde32.c; do
  "$ZIG" cc $CF -c "$f" -o "$OUT/$(basename ${f%.c}).o"
done

CXF="-target $T -std=c++17 -O2 -fno-exceptions -fno-rtti -Iinclude -Iext/minhook/include -D_CRT_SECURE_NO_WARNINGS"
SRC="src/core/sh_scanner.cpp src/core/sh_math.cpp src/core/sh_config.cpp src/core/sh_hooks.cpp src/core/sh_dllmain.cpp
     src/sdk/sh_patterns.cpp src/render/sh_render.cpp src/render/sh_font.cpp src/render/sh_font_data.cpp
     src/features/sh_players.cpp src/features/sh_bones.cpp src/features/sh_antiaim.cpp
     src/features/sh_misc.cpp src/features/sh_esp.cpp src/features/sh_menu.cpp src/features/sh_usermsg.cpp src/features/sh_ballistics.cpp src/features/sh_autowall.cpp src/features/sh_movement.cpp src/features/sh_visuals.cpp src/features/sh_events.cpp src/features/sh_nospread.cpp src/features/sh_modelchanger.cpp src/features/sh_soundesp.cpp \
    src/features/sh_viewmodel.cpp src/features/sh_aspect.cpp src/features/sh_skybox_fast.cpp src/features/sh_matrix.cpp src/features/sh_chams.cpp src/features/sh_tracer3d.cpp src/features/sh_scope.cpp src/features/sh_nightmode.cpp src/features/sh_entityesp.cpp src/features/sh_demo.cpp src/features/sh_grenade.cpp src/features/sh_fakelag.cpp src/features/sh_rage.cpp src/features/sh_skybox.cpp src/features/sh_hitmarker.cpp src/features/sh_knife.cpp src/features/sh_worldfx.cpp src/features/sh_indicators.cpp src/features/sh_svc.cpp src/features/sh_surface.cpp src/features/sh_icons.cpp"
for f in $SRC; do
  "$ZIG" c++ $CXF -c "$f" -o "$OUT/$(basename ${f%.cpp}).o"
done

"$ZIG" c++ -target $T -shared -o "$OUT/skeethook_1_6.dll" "$OUT"/*.o \
  -lopengl32 -lgdi32 -luser32 -lpsapi -static-libgcc -static-libstdc++

echo "[+] $(ls -lh $OUT/skeethook_1_6.dll | awk '{print $5, $9}')"
