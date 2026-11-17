# Tools — Ghidra и хуки для реверса

## Что здесь

- `download_ghidra.sh` — скрипт загрузки официального релиза Ghidra с GitHub.
- `hooks/Universal-Dear-ImGui-Hook/` — универсальный Dear ImGui хук (DX9–DX12, OpenGL, Vulkan), склонирован с GitHub.
- `hooks/kiero/` — классический Kiero (Rebzzel/kiero), универсальный графический хук.
- `../cs1.6-src/ReHLDS/` и `../cs1.6-src/cs16-client/` — открытые исходники серверного движка и клиента CS 1.6.
- Бинарный ZIP Ghidra (~570 МБ) и распакованная Ghidra (~1+ ГБ) в репозиторий не коммитятся — см. `.gitignore`.

## Почему бинарник не скачался прямо в песочнице

Песочница Arena разрешает исходящий трафик только на:
`github.com`, `codeload.github.com`, `api.github.com`, `registry.npmjs.org`, `pypi.org`, `files.pythonhosted.org`.

Официальные релизы Ghidra лежат на `release-assets.githubusercontent.com` — этот хост в allowlist отсутствует, поэтому `curl`/`gh release download` падают с `SSL_ERROR_SYSCALL` / `EOF`.

Что удалось сделать внутри песочницы:
- `git clone --depth 1 --branch Ghidra_12.2_build https://github.com/NationalSecurityAgency/ghidra.git` — исходники Ghidra 12.2 успешно склонированы (проверено, ~440 МБ), затем удалены, чтобы не превышать лимит снапшота (~128 МБ).

## Как получить рабочий Ghidra локально

1. Установи JDK 21+ 64-bit (для 12.2 — JDK 25, см. README Ghidra).
2. Скачай релиз вручную:
   https://github.com/NationalSecurityAgency/ghidra/releases — файл `ghidra_12.2_PUBLIC_20261016.zip`
3. Или запусти скрипт на машине с полным интернетом:

```bash
chmod +x tools/download_ghidra.sh
./tools/download_ghidra.sh
```

4. Запуск: `./ghidra_12.2_PUBLIC/ghidraRun`

## Сборка из исходников (альтернатива)

```bash
git clone --depth 1 --branch Ghidra_12.2_build https://github.com/NationalSecurityAgency/ghidra.git
cd ghidra
./gradlew buildGhidra -x test
```

Требуется JDK + Gradle, собирается долго, но работает без доступа к release-assets.

## Хуки (уже скачаны)

Оба репозитория склонированы через `git clone` с `github.com` (хост разрешён в песочнице):

```bash
git clone --depth 1 https://github.com/Sh0ckFR/Universal-Dear-ImGui-Hook.git tools/hooks/Universal-Dear-ImGui-Hook
git clone --depth 1 https://github.com/Rebzzel/kiero.git tools/hooks/kiero
```

- **Universal-Dear-ImGui-Hook** (Sh0ckFR, ~505 звёзд): один DLL/SO для всех API, меню на `INSERT`, без инжектора через `dinput8.dll` / `LD_PRELOAD`. Сборка: VS2022 на Windows или `cmake --preset linux` на Linux. Подробнее — в `tools/hooks/Universal-Dear-ImGui-Hook/README.md`.
- **Kiero** (Rebzzel): классика, включает нужный API через `#define KIERO_INCLUDE_OPENGL 1` и т.д. в `kiero.h`. Проект помечен как unmaintained, автор рекомендует `kiero2`, но для CS 1.6 (OpenGL) его достаточно. Подробнее — в `tools/hooks/kiero/README.md`.
