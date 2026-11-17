# Реверс CS 1.6 — таблица слотов экспортов `client.dll`

Извлечено из `hw.dll`: загрузчик клиентской DLL вызывает `GetProcAddress`
для каждого имени и складывает результат в **статическую таблицу слотов**
в `.data`. Найдено 32 слота.

Блок строк: `hw.dll` offset `0x1E36AC4`..`0x1E36EE0` (VA).
Загрузчик: `hw.dll` `0x1D0B660`..`0x1D0B720` (там же цикл `call esi` =
`GetProcAddress` и `mov ds:слот, eax`).

---

## 1. Полная таблица

| слот | экспорт | что даёт |
|---|---|---|
| `0x02F2ED64` | `HUD_Init` | инициализация клиента |
| `0x02F2ED68` | `HUD_VidInit` | смена разрешения |
| `0x02F2ED6C` | `HUD_Redraw` | **кадр** |
| `0x02F2ED70` | `HUD_UpdateClientData` | `clientdata_t` |
| `0x02F2ED74` | `HUD_Reset` | сброс |
| `0x02F2ED78` | `HUD_PlayerMove` | **движение** |
| `0x02F2ED7C` | `HUD_PlayerMoveInit` | инициализация pmove |
| `0x02F2ED80` | `HUD_PlayerMoveTexture` | текстура под ногами |
| `0x02F2ED98` | `CL_CreateMove` | **ввод** |
| `0x02F2ED9C` | `CL_IsThirdPerson` | третий вид |
| `0x02F2EDA0` | `CL_CameraOffset` | смещение камеры |
| `0x02F2EDAC` | `V_CalcRefdef` | вьюмодель |
| `0x02F2EDB0` | `HUD_AddEntity` | **рендер сущности** |
| `0x02F2EDB4` | `HUD_CreateEntities` | создание сущностей |
| `0x02F2EDB8` | `HUD_DrawNormalTriangles` | отрисовка мира |
| `0x02F2EDBC` | `HUD_DrawTransparentTriangles` | прозрачное |
| `0x02F2EDC0` | `HUD_StudioEvent` | событие студии |
| `0x02F2EDC4` | `HUD_PostRunCmd` | после команды |
| `0x02F2EDC8` | `HUD_Shutdown` | выход |
| `0x02F2EDCC` | `HUD_TxferLocalOverrides` | локальные overrides |
| `0x02F2EDD0` | `HUD_ProcessPlayerState` | состояние игрока |
| `0x02F2EDD4` | **`HUD_TxferPredictionData`** | **`weapon_data_t[64]`** |
| `0x02F2EDD8` | `Demo_ReadBuffer` | чтение демки |
| `0x02F2EDDC` | `HUD_ConnectionlessPacket` | пакет без соединения |
| `0x02F2EDE0` | `HUD_GetHullBounds` | границы hull |
| `0x02F2EDE4` | `HUD_Frame` | кадр движка |
| `0x02F2EDE8` | `HUD_Key_Event` | клавиши |
| `0x02F2EDEC` | `HUD_TempEntUpdate` | временные сущности |
| `0x02F2EDF0` | `HUD_GetUserEntity` | сущность игрока |
| `0x02F2EDF4` | `HUD_DirectorMessage` | сообщения директора |
| `0x02F2EDF8` | **`HUD_GetStudioModelInterface`** | **кости, `studiohdr_t`** |
| `0x02F2EE08` | `HUD_GetPlayerTeam` | команда игрока |

Все слоты лежат в `.data` (BSS-часть, за файлом), поэтому читать надо
**по адресу в памяти** после загрузки, а не из файла.

---

## 2. Что это даёт SDK

### `HUD_TxferPredictionData` (`0x02F2EDD4`)

Указатель на функцию с прототипом:

```c
void HUD_TxferPredictionData(
    struct entity_state_s *ps,  const struct entity_state_s *pps,
    struct clientdata_s   *pcd, const struct clientdata_s   *ppcd,
    struct weapon_data_s  *wd,  const struct weapon_data_s  *pwd);
```

Хук даёт `wd[64]` — **предсказанные таймеры всех 64 оружий**
(см. `20-weapon-data.md`). Отсюда точный fast reload и double tap.

### `HUD_GetStudioModelInterface` (`0x02F2EDF8`)

```c
int HUD_GetStudioModelInterface(int version,
                                struct r_studio_interface_s **ppinterface,
                                struct engine_studio_api_s *pstudio);
```

Это **единственный** способ получить `studiohdr_t` и кости. Через него
становится возможен настоящий скелетон по костям (в `sh_bones.cpp`,
который был удалён как недоступный).

**Что нужно:** сохранить `engine_studio_api_t` из второго аргумента,
затем `pstudio->StudioGetBoneTransform(hdr, bone, transform)` даёт
матрицы костей.

### `HUD_AddEntity` (`0x02F2EDB0`)

Сейчас SDK находит `HUD_AddEntity` паттерном (`0x01945F10`). Через слот
можно получить его **динамически**, без паттерна:
`*(void**)0x02F2EDB0` — и готовый адрес. Это надёжнее паттерна,
потому что не зависит от кода функции.

### `CL_IsThirdPerson` / `CL_CameraOffset` (`0x02F2ED9C`, `0x02F2EDA0`)

Вот это — **настоящий thirdperson**! Движок сам спрашивает клиентскую
DLL: «ты в третьем виде?» и «какое смещение камеры?». Хук `CL_CameraOffset`
даёт то же, что делает читы через `view_ofs`, но корректно —
движок сам применит смещение и сделает трассировку, чтобы камера
не провалилась в стену.

Это лучше текущей реализации в `sh_visuals.c` (там просто сдвиг
`view_ofs`, без трассировки).

---

## 3. Как читать слоты

```c
typedef void *(*sh_slot_fn)(void);

static void *sh_client_export(u32 slot_va)
{
    return *(void **)slot_va;      // после загрузки client.dll
}
```

**Важно:** слот заполняется **после** того, как `hw.dll` загрузит
`client.dll` (в `HUD_Init`). Если читать раньше — там 0.

Проверка готовности:

```c
void *p = sh_client_export(0x02F2EDB0);
if (!p)
    return;     // client.dll ещё не загружен
```

---

## 4. Почему это лучше паттернов

| способ | плюсы | минусы |
|---|---|---|
| паттерн по коду | работает без таблицы | ломается при обновлении, надо уникальность |
| слот таблицы | **точный адрес**, не зависит от кода | адрес слота надо найти один раз (нашли) |

Слоты **одинаковы во всех сборках GoldSrc** — таблица заполняется
одним и тем же кодом загрузчика. Проверено: все 32 слота идут подряд
с шагом 4 байта от `0x02F2ED64` до `0x02F2EE08`.

---

## 5. Следующие шаги

1. Заменить паттерн `HUD_AddEntity` на чтение слота `0x02F2EDB0`.
2. Хукнуть `HUD_TxferPredictionData` → получить `weapon_data_t[64]`
   → точный `m_flNextReload` для fast reload.
3. Через `HUD_GetStudioModelInterface` получить `engine_studio_api_t`
   → вернуть `sh_bones.cpp` (настоящий скелетон).
4. Хукнуть `CL_CameraOffset` → корректный thirdperson с трассировкой.
