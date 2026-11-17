# Реверс CS 1.6 — кости через `HUD_GetStudioModelInterface`

Источник: `FWGS/cs16-client`, `common/r_studioint.h` и
`engine/APIProxy.h:66`.

---

## 1. Что это даёт

`HUD_GetStudioModelInterface` — **единственный** способ получить
доступ к студийным моделям (и, значит, к костям) из клиентской DLL.

Прототип:

```c
int HUD_GetStudioModelInterface(
    int version,                          // STUDIO_INTERFACE_VERSION = 1
    struct r_studio_interface_s **ppinterface,
    struct engine_studio_api_s *pstudio);
```

Движок зовёт его **один раз** после загрузки `client.dll`.

**Слот в `hw.dll`:** `0x02F2EDF8` (см. `21-clientdll-slots.md`).

---

## 2. Что внутри `pstudio`

`engine_studio_api_s` (`r_studioint.h:18`) — таблица функций движка.
Ключевые для костей:

```c
// Get bone, light, alias, and rotation matrices
float ****(*StudioGetBoneTransform)(void);
float ****(*StudioGetLightTransform)(void);
float ***(*StudioGetAliasTransform)(void);
float ***(*StudioGetRotationMatrix)(void);
```

`StudioGetBoneTransform` возвращает указатель на массив
`float bone[128][3][4]` — уже **мировые** матрицы для
**текущей** сущности, которую движок сейчас рисует.

**Отсюда важное ограничение:** функция даёт кости только для той
сущности, что рисуется в данный момент. Поэтому вызов надо делать
внутри `HUD_AddEntity` (когда движок как раз передал эту сущность)
или в `HUD_DrawNormalTriangles`.

---

## 3. Как получить кости

```c
static engine_studio_api_t *g_studio;
static r_studio_interface_t *g_studio_iface;

int __cdecl HUD_GetStudioModelInterface(int version,
                                        r_studio_interface_t **ppinterface,
                                        engine_studio_api_t *pstudio)
{
    int r = g_orig(version, ppinterface, pstudio);

    g_studio = pstudio;
    g_studio_iface = *ppinterface;

    return r;
}
```

Затем в `HUD_AddEntity`:

```c
float (**bones)[4] = (float (**)[4])g_studio->StudioGetBoneTransform();
// bones[bone_index][0..2][0..3] — мировые позиции
```

Позиция кости:
```c
vec3_t p = { bones[i][0][3], bones[i][1][3], bones[i][2][3] };
```

**Трансляция — в четвёртом столбце** (`[k][3]`), потому что матрица
имеет вид `[3][4]`: 3×3 поворот + смещение.

### `StudioSetupModel` — правильный порядок

```c
void (*StudioSetupModel)(int bodypart, void **ppbodypart, void **ppsubmodel);
```

Перед чтением костей надо вызвать для нужной части тела:

```c
void *bodypart, *submodel;
g_studio->StudioSetupModel(0, &bodypart, &submodel);
```

Без этого `StudioGetBoneTransform` вернёт данные от предыдущей модели.

---

## 4. Индексы костей (стандартный скелет игрока CS)

| индекс | кость |
|---:|---|
| 0 | pelvis |
| 1 | spine |
| 2 | spine1 |
| 3 | neck |
| 4 | head |
| 5 | l_upperarm |
| 6 | l_forearm |
| 7 | l_hand |
| 8 | r_upperarm |
| 9 | r_forearm |
| 10 | r_hand |
| 11 | l_thigh |
| 12 | l_calf |
| 13 | l_foot |
| 14 | r_thigh |
| 15 | r_calf |
| 16 | r_foot |

Это ровно те 17 костей, что были в `sh_bones.h` (удалён как
«недоступный» — теперь доступен).

---

## 5. Почему это работает, а паттерн — нет

Паттерн ищет код, который может измениться. Слот
`HUD_GetStudioModelInterface` — **фиксированный адрес** в `.data`,
заполняется одним и тем же загрузчиком во всех сборках GoldSrc.

Плюс: `StudioGetBoneTransform` — официальный API движка. Он не
ломается при обновлении, потому что версия интерфейса
(`STUDIO_INTERFACE_VERSION`) проверяется самим движком.

---

## 6. Что уже сделано

- Найден слот `0x02F2EDF8` (см. `21-clientdll-slots.md`).
- `HUD_TxferPredictionData` уже захукан (даёт `weapon_data_t[64]`).
- `sh_bones.cpp` был удалён, потому что `studiomodel` в
  `cl_enginefuncs_t` — просто `void*` без методов.

**Что осталось:**

1. Добавить `engine_studio_api_t` в `sh_sdk.h` (сейчас там нет).
2. Захукать `HUD_GetStudioModelInterface`, сохранить `g_studio`.
3. В `HUD_AddEntity` (уже захукан) читать кости для сущности.
4. Вернуть `sh_bones.cpp` с реальными позициями.
5. В ESP рисовать скелет по костям, а не по 2D-проекции бокса.

Разница будет видна сразу: сейчас скелетон — это
пропорциональные линии внутри бокса, они не совпадают с моделью.
С костями каждая линия ляжет точно по руке/ноге, включая анимацию.
