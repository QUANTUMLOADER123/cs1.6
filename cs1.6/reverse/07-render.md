# Реверс CS 1.6 — рендер и проекция

Источники: `common/triangleapi.h`, `engine/APIProxy.h`.

---

## 1. `triangleapi_t` — интерфейс отрисовки

Доступ: `gEngfuncs->pTriAPI` (оффсет `+0x148`, подтверждён по заголовку).

```c
typedef struct triangleapi_s
{
    int   version;

    void  (*RenderMode)(int mode);
    void  (*Begin)(int primitiveCode);
    void  (*End)(void);

    void  (*Color4f)(float r, float g, float b, float a);
    void  (*Color4ub)(u8 r, u8 g, u8 b, u8 a);
    void  (*TexCoord2f)(float u, float v);
    void  (*Vertex3fv)(float *worldPnt);
    void  (*Vertex3f)(float x, float y, float z);
    void  (*Brightness)(float brightness);
    void  (*CullFace)(TRICULLSTYLE style);
    int   (*SpriteTexture)(model_s *pSpriteModel, int frame);

    int   (*WorldToScreen)(float *world, float *screen);   // ⭐ 1 = за клипом
    void  (*Fog)(float color[3], float start, float end, int on);
    void  (*ScreenToWorld)(float *screen, float *world);   // ⭐ обратная
    void  (*GetMatrix)(const int pname, float *matrix);    // ⭐ матрицы
    int   (*BoxInPVS)(float *mins, float *maxs);           // ⭐ видимость bbox
    void  (*LightAtPoint)(float *pos, float *value);
    void  (*Color4fRendermode)(float r, float g, float b, float a, int rendermode);
    void  (*FogParams)(float density, int skybox);
} triangleapi_t;
```

---

## 2. `WorldToScreen` — не изобретать самому ⭐

Главный вывод документа. В SDK `skeethook_1_6` `sh_world_to_screen` была
написана вручную через `g_globals->v_forward/v_right/v_up` и
предположение FOV = 90. Это **работает, но врёт** в трёх случаях:

1. смена FOV (`SetFOV` при снайперке — 40°, а не 90°)
2. `zoom` в оптике
3. нестандартный `aspect ratio`

Движок умеет это правильно — надо вызывать `pTriAPI->WorldToScreen`:

```c
int w2s(vec3_t world, vec2_t *out)
{
    float screen[2];
    if (!g_tri_api || !g_tri_api->WorldToScreen)
        return 0;

    // возвращает 1 если точка за клиппингом
    if (g_tri_api->WorldToScreen((float *)&world, screen))
        return 0;

    out->x = screen[0];
    out->y = screen[1];
    return 1;
}
```

Возврат `1` = точка **позади** камеры (z-clipped), рисовать нельзя.
Это ровно то, что нужно ESP — иначе боксы «зеркалятся» за спиной.

---

## 3. `ScreenToWorld` — обратная проекция

Нужна для:

- **автоволл** — трассировка от прицела в мир без знания углов
- **поиск цели под курсором** (aim наводится мышью)
- **3D-меню** в мире

```c
void screen_to_world(float sx, float sy, float *world)
{
    float screen[2] = { sx, sy };
    g_tri_api->ScreenToWorld(screen, world);
}
```

---

## 4. `BoxInPVS` — проверка видимости bbox

Быстрая проверка «попадает ли коробка в поле зрения без трассировки».
Дешевле `WorldToScreen`×8 и точнее, чем `visible_check` через трассу.

```c
int in_pvs = g_tri_api->BoxInPVS(mins, maxs);
```

Порядок фильтрации в ESP для производительности:

```
1. distance < max          (числа)
2. BoxInPVS(mins, maxs)    (быстро, без трассы)
3. WorldToScreen(origin)   (проекция)
4. trace_line             (дорого, только для видимых)
```

---

## 5. `GetMatrix` — мировые матрицы

```c
float world_matrix[16];
g_tri_api->GetMatrix(kMatrixWorld, world_matrix);
```

Константы `pname` (из `ref_params_t`):

| константа | значение |
|---|---|
| `kMatrixWorld` | 0 |
| `kMatrixView` | 1 |
| `kMatrixProjection` | 2 |

Позволяет считать проекцию самому по полной матрице MVP — если нужен доступ
к каждому шагу (например, для точных чеймов через `gl_Position`).

---

## 6. Порядок отрисовки в кадре

Движок за кадр делает:

```
1. 3D-сцена   (мировые координаты, glFrustum)
2. tri_api    (2D поверх мира, ортопроекция)
3. VGUI       (меню, счёт)
4. HUD_Redraw → client.dll рисует HUD (орто 2D, 0..width/0..height)
```

Следствие: **если рисовать внутри `HUD_Redraw`, координаты уже экранные**
и `glVertex2f` работает напрямую — ортопроекция выставлена движком.
Ровно так сделано в `sh_render.cpp`.

Если рисовать в своём хуке GL `SwapBuffers` — надо самому выставлять
`glOrtho(0, w, h, 0, -1, 1)`, иначе координаты уедут.

---

## 7. Резолв GL в `hw.dll`

Ядро `hw.dll` наружу GL не отдаёт (экспортируются только 323 VGUI-класса).
Внутри — таблица GL-функций, набиваемая резолвером:

```asm
01D72F95  push  0x01E509F8        ; имя функции
01D72F9A  call  0x01DCF914        ; jmp ds:[GetProcAddress]
01D72FA2  mov   ds:0x02792074,eax ; сохранить указатель
```

Паттерн резолвера: `push str / call [IAT] / add esp,4 / mov ds:gvar,eax`.

Чит обычно берёт GL **напрямую через `opengl32.dll`** (как в SDK) — это
надёжнее, чем искать таблицу в `hw.dll`, потому что `GetProcAddress` вернёт
те же адреса.

---

## 8. Что поправлено в SDK

Из этого документа:

1. `WorldToScreen` → `pTriAPI->WorldToScreen` вместо ручной формулы
2. `BoxInPVS` добавлен как быстрый фильтр перед трассой
3. Учтён `SetFOV` — иначе проекция врёт при зуме

Ручная `sh_world_to_screen` оставлена как fallback, если `pTriAPI` не
доступен (например, в headless-тесте).

---

## 9. Следующее

- `08-events.md` — `PlaybackEvent`, `events/*.sc`, партиклы
- `09-chams.md` — `StudioDrawModel`, подмена `rendermode` на уровне хука
