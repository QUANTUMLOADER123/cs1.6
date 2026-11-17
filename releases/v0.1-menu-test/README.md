# v0.1-menu-test — меню gamesense, порт 1:1

Тестовая сборка меню для CS 1.6: **пиксельный порт меню gamesense (skeet)**
с настоящими ассетами.

## Файлы

| файл | что |
| --- | --- |
| `menu_test_x86.dll` | 32-битная Windows DLL, хук `wglSwapBuffers` + ImGui-меню |
| `rage.png`, `antiaim.png`, `visuals.png`, `misc.png`, `settings.png`, `skins.png`, `players.png`, `lua.png` | 8 вкладок, 1:1, 731×617 |
| `SHA256.txt` | контрольная сумма DLL |

SHA256: `38caf668335555553cf346e11756ca8ab363a06a8194de5cf45a06cb57798c76`

Скачать DLL напрямую:
https://github.com/QUANTUMLOADER123/cs1.6/raw/arena/6f730c22-cs1-6/releases/v0.1-menu-test/menu_test_x86.dll

## Что значит 1:1

Меню снято с реального скриншота gamesense попиксельно — не «в стиле», а точно:

- **иконки — настоящий спрайт** `tab_icons.png` (384×48, 8 плиток 48×48) из
  gamesense-меню, вшит в DLL (`gui_icons_data.h`, генерируется `tools/embed_assets.py`);
- **шрифт — настоящий Tahoma Bold** (`assets/Tahoma-Bold.ttf`), вшит в DLL
  (`gui_font_data.h`), с кириллицей;
- **цвета**: фон `#0C0C0C`, дизеринг `#141414`, колонки `#171717` + рамка `#303030`,
  окно `#3C3C3C`/`#282828`, текст `#CDCDCD`, галка `#9CC728`→`#7AA506`,
  поля `#1F1F1F`→`#242424`, радуга 2px под рамкой;
- **геометрия**: окно 731×617, сайдбар 97px, колонки x 101..392 и 415..705,
  чекбокс 6×6 на x 123, текст на x 143, поля x 153..333, шаг строк 18px;
- **8 вкладок как в оригинале**: Rage, Anti-Aim, Visuals, Misc, Settings, Skins,
  Players, Lua.

Контроль качества: средняя разница с оригиналом по всем 731×617 пикселям —
**3.8 из 255**; 81 % пикселей отличаются не более чем на 1/255.

## Что внутри DLL

- PE32, i386 — **32-битная**, как сам CS 1.6
- MinHook на `wglSwapBuffers` (opengl32)
- ImGui 1.93, бэкенд OpenGL2 (legacy pipeline — совпадает с рендером 1.6)
- Оригинальный спрайт иконок загружается в GL-текстуру при старте
- Tahoma Bold из памяти — не зависит от системных шрифтов

## Управление

| клавиша | действие |
| --- | --- |
| `INSERT` | открыть / закрыть меню |
| `F12` | сохранить скриншот кадра в `menu_shot_N.png` |

## Установка

Нужен инжектор (Process Hacker, Xenos, свой). DLL 32-битная — инжектить
только в 32-битный процесс.

1. Запусти CS 1.6 с рендером OpenGL.
2. Инжектни `menu_test_x86.dll` в `hl.exe`.
3. `INSERT` — меню.

## Скриншоты (1:1, 731×617)

| Rage | Anti-Aim |
| --- | --- |
| ![Rage](rage.png) | ![Anti-Aim](antiaim.png) |

| Visuals | Misc |
| --- | --- |
| ![Visuals](visuals.png) | ![Misc](misc.png) |

| Settings | Skins |
| --- | --- |
| ![Settings](settings.png) | ![Skins](skins.png) |

| Players | Lua |
| --- | --- |
| ![Players](players.png) | ![Lua](lua.png) |

## Ограничения

- Это **только UI и хук**. Чекбоксы запоминают состояние, но к игре не
  подключены — функционал софта будет в следующих версиях (по плану: меню
  и инфраструктура → софт → релизный билд чита).
- В 1.6 движок меняет матрицы перед своим кадром. Если меню «уедет» — нужен
  `glPushAttrib(GL_ALL_ATTRIB_BITS)` + `glViewport(0,0,w,h)` перед
  `RenderDrawData()` (в этой сборке меню рисуется уже после сброса состояния).

## Как собрано

`zig cc -target x86-windows-gnu`, без MSVC и mingw: `cs1.6/menu/build_win.sh`.
Исходники: `cs1.6/menu/` (`include/gui_*.h`, `hook/dllmain.cpp`).
