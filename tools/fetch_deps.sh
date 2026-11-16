#!/usr/bin/env bash
# Восстанавливает все вендорные исходники (Ghidra, хуки, CS 1.6 SDK).
# Клонирует только то, чего нет. Запускать из корня репо.
set -euo pipefail

cd "$(dirname "$0")"
ROOT="$(pwd)"

clone() {
  local url="$1" dest="$2" ref="${3:-}"
  if [ -d "$dest/.git" ]; then
    echo "[=] $dest уже есть"
    return
  fi
  rm -rf "$dest"
  echo "[+] $dest <- $url"
  if [ -n "$ref" ]; then
    git clone --depth 1 --branch "$ref" "$url" "$dest"
  else
    git clone --depth 1 "$url" "$dest"
  fi
  rm -rf "$dest/.git"
}

# --- Ghidra (исходники; бинарный релиз качается отдельно, см. tools/README.md)
if [ ! -d "$ROOT/tools/ghidra-src" ]; then
  echo "[+] tools/ghidra-src <- Ghidra_12.2_build (это долго, ~440 МБ)"
  git clone --depth 1 --branch Ghidra_12.2_build \
    https://github.com/NationalSecurityAgency/ghidra.git "$ROOT/tools/ghidra-src"
  rm -rf "$ROOT/tools/ghidra-src/.git"
fi

# --- Хуки
clone https://github.com/Sh0ckFR/Universal-Dear-ImGui-Hook.git \
      "$ROOT/tools/hooks/Universal-Dear-ImGui-Hook"
clone https://github.com/Rebzzel/kiero.git "$ROOT/tools/hooks/kiero"

# kiero использует submodules (minhook, imgui) — их тоже тянем
if [ -d "$ROOT/tools/hooks/kiero" ]; then
  [ -d "$ROOT/tools/hooks/kiero/minhook" ] && [ -z "$(ls -A "$ROOT/tools/hooks/kiero/minhook" 2>/dev/null)" ] && \
    clone https://github.com/TsudaKageyu/minhook.git "$ROOT/tools/hooks/kiero/minhook"
  [ -d "$ROOT/tools/hooks/kiero/examples/imgui/imgui" ] && [ -z "$(ls -A "$ROOT/tools/hooks/kiero/examples/imgui/imgui" 2>/dev/null)" ] && \
    clone https://github.com/ocornut/imgui.git "$ROOT/tools/hooks/kiero/examples/imgui/imgui"
fi

# --- CS 1.6 (легальные открытые исходники, не сама игра)
clone https://github.com/rehlds/ReHLDS.git "$ROOT/cs1.6-src/ReHLDS"
clone https://github.com/FWGS/cs16-client.git "$ROOT/cs1.6-src/cs16-client"

echo
echo "Готово. Размеры:"
du -sh "$ROOT"/tools/hooks/* "$ROOT"/cs1.6-src/* 2>/dev/null || true
