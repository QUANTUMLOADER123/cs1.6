# CS 1.6 — исходники и файлы игры

## Что лежит здесь / рядом

| Путь | Что это |
| --- | --- |
| `cs1.6-src/ReHLDS/` | ReHLDS — открытый ребилд серверного движка Half-Life/CS 1.6 (C++, CMake). Полные исходники. |
| `cs1.6-src/cs16-client/` | FWGS/cs16-client — реверс-инжиниринг клиента CS 1.6 под Xash3D FWGS (C++, CMake). Полные исходники, включая `cl_dll`, `engine`, `dlls` (GameDLL), `common` (SDK-хедеры). |
| `cs1.6/` (эта папка) | Сюда кладутся **бинарные файлы игры** (`.dll`, `.so`, `.bsp`, `.pak`) для реверса в Ghidra. В git не попадают — см. `.gitignore`. |

Оба репозитория — легальные открытые исходники (ReHLDS/GPL, cs16-client/GPL). Сама игра (карты, модели, звуки, `hw.dll`) — коммерческий контент Valve, его здесь нет.

## Почему не с counter-strike.one

1. **Сеть песочницы.** Разрешены только `github.com`, `codeload.github.com`, `api.github.com`, `registry.npmjs.org`, `pypi.org`, `files.pythonhosted.org`. `counter-strike.one` вне списка — `curl` падает с `SSL_ERROR_SYSCALL`.
2. **Лицензия.** CS 1.6 — коммерческая игра Valve. Пиратские сборки не качаю и не распространяю. Легально — Steam (Counter-Strike 1.6, ~$10).

## Как добавить свои файлы игры

1. Возьми легальную копию из Steam (`steamapps/common/Half-Life/cstrike` + `valve`) или свою установленную 1.6.
2. Скопируй в `cs1.6/`: `cs1.6/cstrike/`, `cs1.6/valve/`, при желании `cs1.6/hl.exe`.
3. Бинарники для реверса:
   - Windows: `hw.dll` (клиентский движок, рендер OpenGL), `sw.dll`, `cstrike/cl_dll/client.dll`, `cstrike/dlls/mp.dll`
   - Linux: `libhw.so`, `cstrike/cl_dll/client.so`, `cstrike/dlls/cs.so`
4. Всё это уже покрыто `.gitignore` — в репо не утечёт.

## Реверс в Ghidra

```bash
# headless-импорт и декомпиляция
/opt/ghidra/support/analyzeHeadless /tmp/ghidra_proj cs16 \
  -import cs1.6/cstrike/cl_dll/client.dll \
  -processor x86:LE:32:default \
  -postScript DecompileFunctions.java
```

Для 32-битных `hw.dll`/`client.dll` процессор — `x86:LE:32:default` (у 1.6 всё 32-битное, x86). По интерфейсам клиента удобно сверяться с `cs1.6-src/cs16-client/cl_dll/` — там те же классы (`CHud`, `CBaseEntity`, `CStudioModelRenderer`), но с читаемыми именами.

## Рендер CS 1.6 и ImGui-хук

CS 1.6 на Windows по умолчанию рендерит через OpenGL 1.x (`hw.dll` → `wglSwapBuffers`). Поэтому из скачанных хуков рабочий вариант — OpenGL-ветка:

- `tools/hooks/Universal-Dear-ImGui-Hook/` — сам определяет API (OpenGL 3.0+ / WGL), собирается одним DLL под Win32. Для CS 1.6 нужен **32-битный** билд (`Universal-ImGui-Hook-msvc-Win32.dll`).
- `tools/hooks/kiero/` — в `kiero.h` ставится `#define KIERO_INCLUDE_OPENGL 1`, хук на `wglSwapBuffers`, MinHook уже в комплекте.

Подводный камень: 1.6 — 32-битный процесс, и OpenGL-контекст там legacy (fixed pipeline), так что ImGui-бэкенд `imgui_impl_opengl2` подходит лучше, чем `opengl3`.
