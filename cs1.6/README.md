# CS 1.6 — файлы игры

## Статус

Файлы CS 1.6 с `counter-strike.one` в песочницу скачать **не удалось** по двум причинам:

1. **Сетевые ограничения песочницы.** Разрешённые хосты: `github.com`, `codeload.github.com`, `api.github.com`, `registry.npmjs.org`, `pypi.org`, `files.pythonhosted.org`. `counter-strike.one` не входит в список — прямое скачивание недоступно (проверено: `curl` падает с `SSL_ERROR_SYSCALL`).

2. **Лицензия.** CS 1.6 — коммерческая игра Valve. Я не могу скачивать и распространять пиратские копии. Легальный способ — Steam (Counter-Strike 1.6).

## Как добавить файлы вручную

1. Купи/скачай CS 1.6 в Steam или возьми из своей легальной копии.
2. Скопируй папку игры в `cs1.6/` (например, `cs1.6/cstrike/`, `cs1.6/valve/` и т.д.).
3. Большие бинарники не коммитить — они уже в `.gitignore` (`*.zip`, `*.exe`, `*.dll`, `*.so`, `*.pak`, `*.bsp` и т.д.). Для реверса в Ghidra достаточно открыть `cstrike/bin/` или `valve/bin/` `.dll`/`.so` как бинарники.

## Для реверса

После добавления файлов:
- Открой Ghidra → File → Import File → выбери `cstrike/bin/engine.dll` (Windows) или `cstrike/bin/engine.so` (Linux).
- Или через headless: `ghidraRun -import <path> -postScript Decompile`
