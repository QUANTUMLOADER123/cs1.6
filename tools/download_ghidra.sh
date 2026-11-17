#!/usr/bin/env bash
# Скачивание официального релиза Ghidra с GitHub.
# Запускать на машине с полным доступом в интернет
# (в песочнице Arena release-assets.githubusercontent.com заблокирован).
set -euo pipefail

VERSION="Ghidra_12.2_build"
FILE="ghidra_12.2_PUBLIC_20261016.zip"
URL="https://github.com/NationalSecurityAgency/ghidra/releases/download/${VERSION}/${FILE}"

cd "$(dirname "$0")"

echo "Скачиваю $FILE ..."
curl -L -o "$FILE" "$URL"

echo "Проверяю размер..."
ls -lh "$FILE"

echo "Распаковываю..."
unzip -q "$FILE"

echo "Готово. Запуск: ./ghidra_12.2_PUBLIC/ghidraRun"
echo "Требуется JDK 21+ 64-bit (для 12.2 — JDK 25)."
