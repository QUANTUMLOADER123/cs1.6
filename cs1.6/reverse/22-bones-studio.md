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
сущности, что рисуется в данный момент.

**Первая попытка была провалена.** Мы читали кости в `HUD_AddEntity`
(он захукан и знает номер сущности), но там кости **ещё не посчитаны**.
`HUD_AddEntity` вызывается *до* того, как движок отрисует модель; в
результате приходили кости **предыдущей** нарисованной сущности —
скелетон «отставал» на одного игрока и висел в неверных точках.

Правильное место — **внутри `StudioDrawPlayer`**, после вызова
оригинала. Разбор — в секции 3.1.

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

### 3.1. Хук `StudioDrawPlayer` — единственно верное место

`r_studio_interface_s` (`common/r_studioint.h:126`):

```c
typedef struct r_studio_interface_s
{
    int version;
    int (*StudioDrawModel)(int flags);
    int (*StudioDrawPlayer)(int flags, struct entity_state_s *pplayer);
} r_studio_interface_t;
```

Движок рисует игрока через `StudioDrawPlayer`. Внутри
`CGameStudioModelRenderer::_StudioDrawPlayer`
(`cl_dll/GameStudioModelRenderer.cpp:939`) стоит:

```c
StudioSetupBones();
```

А `StudioSetupBones` (`StudioModelRenderer.cpp:721`) конкатенирует
мировую матрицу `m_protationmatrix` с локальной костью:

```c
ConcatTransforms((*m_protationmatrix), bonematrix, (*m_pbonetransform)[i]);
```

Именно `m_pbonetransform` и есть то, что отдаёт
`StudioGetBoneTransform` (он присваивается в конструкторе,
`StudioModelRenderer.cpp:62`).

**Значит после возврата из `StudioDrawPlayer(flags, pplayer)` кости
`pplayer->number` гарантированно посчитаны и мировые.**

Хукаем не адрес функции через MinHook, а сам указатель в интерфейсе —
подменяем поле `iface->studio_draw_player`. Это надёжнее, потому что
не зависит от того, скомпилирован ли клиент с фиксированным адресом:

```c
typedef int (*sh_bones_draw_orig_fn)(int flags, entity_state_t *pplayer);

static sh_bones_draw_orig_fn g_draw_orig;

static int sh_bones_draw_player(int flags, entity_state_t *pplayer)
{
    int r = g_draw_orig(flags, pplayer);

    if (pplayer && pplayer->number > 0)
        sh_bones_capture(pplayer->number, sh_bones_current_header());

    return r;
}

void sh_bones_hook_iface(r_studio_interface_t *iface)
{
    if (!iface || !iface->StudioDrawPlayer || g_draw_orig)
        return;

    MH_CreateHook(iface->StudioDrawPlayer, sh_bones_draw_player,
                  (void **)&g_draw_orig);
    MH_EnableHook(iface->StudioDrawPlayer);
    iface->StudioDrawPlayer = sh_bones_draw_player;
}
```

Читать кости:

```c
void ***raw = (void ***)g_studio->StudioGetBoneTransform();
float **rows = (float **)raw[bone_index];
vec3_t p = { rows[0][3], rows[1][3], rows[2][3] };
```

`float ****` в C не типизируется напрямую (компилятор ругается на
`assigning to float from float[4]`), поэтому идём через `void ***`.

### 3.2. `StudioSetupModel` вызывать НЕ нужно

Раньше в этом документе было написано, что перед чтением костей нужен
`StudioSetupModel(0, &bodypart, &submodel)`. **Это неверно** при хуке
`StudioDrawPlayer`: `_StudioDrawPlayer` сам вызывает `StudioSetupModel`
для каждой части тела, а `StudioSetupBones` работает уже после. Вызов
своего `StudioSetupModel` сбросил бы состояние на чужую модель.

### 3.3. Заголовок модели (`studiohdr_t`)

Чтобы перевести номер кости в имя, нужен заголовок текущей модели.
`HUD_GetStudioModelInterface` его не отдаёт (`StudioSetHeader` только
принимает). Берём так:

```c
engine_studio_api_t *api;
cl_entity_t *ent = api->GetCurrentEntity();
void *hdr = api->Mod_Extradata(ent->model);
```

`Mod_Extradata` лежит по смещению **0x10**, `GetCurrentEntity` — по
**0x18** в `engine_studio_api_t`.

Позиция кости:
```c
vec3_t p = { bones[i][0][3], bones[i][1][3], bones[i][2][3] };
```

**Трансляция — в четвёртом столбце** (`[k][3]`), потому что матрица
имеет вид `[3][4]`: 3×3 поворот + смещение.

## 4. Кости ищем ПО ИМЕНИ, а не по индексу

Первая версия `sh_bones.h` содержала выдуманную таблицу индексов
(0 pelvis, 1 spine, ... 16 r_foot). Это было ошибкой.

`mstudiobone_t` (`engine/studio.h:114`):

```c
typedef struct
{
    char  name[32];        // имя кости
    int   parent;          // родительская кость
    int   flags;
    int   bonecontroller[6];
    float value[6];        // значения DoF по умолчанию
    float scale[6];        // масштаб для delta DoF
} mstudiobone_t;           // 112 байт (0x70)
```

**Имена костей в моделях игроков CS одинаковы:**
`Bip01 Head`, `Bip01 Neck`, `Bip01 Spine3`, `Bip01 Pelvis`,
`Bip01 L UpperArm`, `Bip01 L Forearm`, `Bip01 L Hand`,
`Bip01 R UpperArm`, ... `Bip01 R Foot`.

А вот **порядковые номера различаются** между моделями (`vip`,
`ct_urban`, `terror`, `gign` и т.д.) — порядок задаётся при экспорте
из 3ds Max и ничем не регламентирован. Поэтому индекс — ловушка.

Заголовок отдаёт список так:

```c
mstudiobone_t *bones = (mstudiobone_t *)((byte *)hdr + hdr->boneindex);
for (int i = 0; i < hdr->numbones; ++i)
    if (stricmp(bones[i].name, "Bip01 Head") == 0) { ... }
```

Проверять надо все 32 байта с приведением к верхнему регистру:
`stricmp` останавливается на первом `\0`, а имя может быть короче.

### Наши 20 костей (`sh_bone_id`)

| id | имя в модели | id | имя в модели |
|---:|---|---:|---|
| 0 | `Bip01 Head` | 10 | `Bip01 R Clavicle` |
| 1 | `Bip01 Neck` | 11 | `Bip01 R UpperArm` |
| 2 | `Bip01 Spine3` | 12 | `Bip01 R Forearm` |
| 3 | `Bip01 Spine2` | 13 | `Bip01 R Hand` |
| 4 | `Bip01 Spine1` | 14 | `Bip01 L Thigh` |
| 5 | `Bip01 Pelvis` | 15 | `Bip01 L Calf` |
| 6 | `Bip01 L Clavicle` | 16 | `Bip01 L Foot` |
| 7 | `Bip01 L UpperArm` | 17 | `Bip01 R Thigh` |
| 8 | `Bip01 L Forearm` | 18 | `Bip01 R Calf` |
| 9 | `Bip01 L Hand` | 19 | `Bip01 R Foot` |

Замечено в `cl_dll/GameStudioModelRenderer.h:26` — там есть свой
`enum BoneIndex { BONE_HEAD, BONE_PELVIS, BONE_SPINE1, BONE_SPINE2,
BONE_SPINE3 }`, но это **не индексы в модели**, а индексы в массиве
`m_rgBonePos[5]`, который клиент заполняет сам для позиционирования
камеры наблюдения. К реальному скелету отношения не имеет.

### Fallback

Если по имени нашлось меньше 8 костей (модифицированная модель,
пропатченный клиент), берётся таблица стандартного скелета:

```c
static const int def[sh_bone_count] = {
    9, 8, 7, 6, 5, 4, 14, 15, 16, 17,
    38, 39, 40, 41, 50, 51, 52, 53, 54, 55
};
```

---

## 5. Почему это работает, а паттерн — нет

Паттерн ищет код, который может измениться. Слот
`HUD_GetStudioModelInterface` — **фиксированный адрес** в `.data`,
заполняется одним и тем же загрузчиком во всех сборках GoldSrc.

Плюс: `StudioGetBoneTransform` — официальный API движка. Он не
ломается при обновлении, потому что версия интерфейса
(`STUDIO_INTERFACE_VERSION`) проверяется самим движком.

---

## 6. Что сделано

- `engine_studio_api_t` добавлен в `sh_sdk.h` как
  `sh_studio_api_t` (21 поле, смещения сверены: `StudioGetBoneTransform`
  @ 0x40, `StudioSetupModel` @ 0x50).
- `r_studio_interface_s` добавлен как `sh_studio_iface_t`.
- `mstudiobone_t` → `sh_studio_bone_t` (112 байт, проверено
  `_Static_assert` через typedef-массив).
- `studiohdr_t` → `sh_studio_header_t`.
- Захукан `HUD_GetStudioModelInterface`: сохраняет `pstudio` и
  подменяет `iface->StudioDrawPlayer`.
- `sh_bones_capture` читает мировые позиции 20 костей через
  `StudioGetBoneTransform` после вызова оригинала.
- ESP рисует 19 связей скелета по реальным костям, с отсечкой
  устаревших данных (0.25 с) и fallback на 2D-бокс.

Кости теперь ложатся точно по модели, включая анимацию: бег, прыжок,
приседание, смерть.
