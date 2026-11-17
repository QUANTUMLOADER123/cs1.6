# cs1.6 — песочница для реверса CS 1.6

Репо с инструментами и исходниками для разбора Counter-Strike 1.6.

## Что где

```
tools/
  download_ghidra.sh   — качает официальный релиз Ghidra 12.2 с GitHub
  fetch_deps.sh        — восстанавливает все вендорные исходники (хуки + CS 1.6 SDK)
  hooks/               — Universal Dear ImGui Hook + Kiero (не в git, см. fetch_deps.sh)
  ghidra-src/          — исходники Ghidra 12.2 (не в git)

cs1.6-src/
  ReHLDS/              — открытый серверный движок HL/CS 1.6
  cs16-client/         — реверс клиента CS 1.6 под Xash3D FWGS

cs1.6/                 — сюда кладётся своя легальная копия игры для реверса в Ghidra
```

## Быстрый старт

```bash
./tools/fetch_deps.sh          # ~50 МБ: хуки + cs16-client + ReHLDS
./tools/download_ghidra.sh     # ~570 МБ: Ghidra 12.2 (нужен JDK)
```

## Ghidra: статус

Бинарный релиз Ghidra лежит на `release-assets.githubusercontent.com`, а песочница пускает только на `github.com`, `codeload`, `api.github.com`, `npmjs`, `pypi`. Поэтому ZIP не скачивается — `download_ghidra.sh` запускать на машине с полным интернетом. Исходники Ghidra при этом клонируются нормально.

## CS 1.6: статус

Саму игру (коммерческий контент Valve) и сборки с `counter-strike.one` не качаю — хост вне allowlist песочницы, плюс это пиратская копия. Вместо этого в репо лежат открытые исходники:

- **ReHLDS** — серверный движок HLDS, переписанный с фиксами. C++/CMake, GPL.
- **cs16-client** — реверс клиента CS 1.6 (Xash3D FWGS). Есть `cl_dll` (клиентская DLL), `engine`, `dlls` (GameDLL), `common` (SDK-хедеры).

Для реверса конкретных бинарников (`hw.dll`, `client.dll`) нужна твоя легальная копия — кладёшь в `cs1.6/`, открываешь в Ghidra. Детали — `cs1.6/README.md`.

## Рендер и хуки

CS 1.6 (Win32) рендерит через OpenGL 1.x → хук на `wglSwapBuffers`. Оба хука это умеют:

- `Universal-Dear-ImGui-Hook` — один DLL под все API, для 1.6 нужен билд **Win32**, меню на INSERT.
- `kiero` — в `kiero.h` включить `KIERO_INCLUDE_OPENGL 1`, MinHook в комплекте.

Для ImGui-бэкенда в 1.6 использовать `imgui_impl_opengl2` (legacy pipeline), не `opengl3`.
