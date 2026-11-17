# Реверс CS 1.6 — `clientdata_t` и предсказание

Самый важный слой для локального игрока: то, что сервер присылает лично
тебе каждый кадр и что клиент потом предсказывает.

---

## 1. Три таблицы, которые надо различать

| структура | размер | от кого | к чему |
|---|---:|---|---|
| `entity_state_t` | 340 | сервер | состояние **любой** сущности |
| `clientdata_t` | **476** | сервер | **локальный** игрок (только своё) |
| `weapon_data_t` | 60 | сервер | предсказание оружия |

Первые две легко перепутать. Разница:

- `entity_state_t` — для всех, включая врагов, содержит `team`, `weaponmodel`, `health`
- `clientdata_t` — только для себя, содержит `punchangle`, `fov`, `view_ofs`,
  `ammo_*`, `deadflag` — то, из чего считается вид и HUD

---

## 2. `clientdata_t` — 476 байт, проверено

Раскладка посчитана из `common/entity_state.h` и **сверена с кодом**.
`HUD_TxferLocalOverrides` (`0x01945FC0`):

```asm
01945FC8  mov edx,[eax]        ; from->origin.x
01945FCA  mov [ecx+0x10],edx   ; to->origin.x
01945FD9  mov edx,[eax+0x18C]  ; from->iuser1      ⭐
01945FDF  mov [ecx+0x104],edx  ; to->iuser1
01945FE5  mov edx,[eax+0x190]  ; from->iuser2
```

| оффсет | поле |
|---|---|
| `+0x000` | `origin` (vec3) |
| `+0x00C` | `velocity` (vec3) |
| `+0x018` | `viewmodel` (int) |
| `+0x01C` | **`punchangle`** (vec3) ⭐ отдача |
| `+0x028` | `flags` (int) |
| `+0x02C` | `waterlevel` |
| `+0x030` | `watertype` |
| `+0x034` | `view_ofs` (vec3) |
| `+0x040` | `health` (float) |
| `+0x044` | `bInDuck` |
| `+0x048` | `weapons` |
| `+0x04C` | `flTimeStepSound` |
| `+0x050` | `flDuckTime` |
| `+0x054` | `flSwimTime` |
| `+0x058` | `waterjumptime` |
| `+0x05C` | **`maxspeed`** ⭐ |
| `+0x060` | **`fov`** ⭐ |
| `+0x064` | `weaponanim` |
| `+0x068` | `m_iId` (id оружия) |
| `+0x06C` | `ammo_shells` |
| `+0x070` | `ammo_nails` |
| `+0x074` | `ammo_cells` |
| `+0x078` | `ammo_rockets` |
| `+0x07C` | `m_flNextAttack` |
| `+0x080` | `tfstate` |
| `+0x084` | `pushmsec` |
| `+0x088` | **`deadflag`** ⭐ |
| `+0x08C` | `physinfo[256]` |
| `+0x18C` | `iuser1..4` |
| `+0x19C` | `fuser1..4` |
| `+0x1AC` | `vuser1..4` |

### Ловушка, на которой легко ошибиться

`physinfo` — **256 байт**, а не 512. Из-за этого `iuser1` уезжает с
`+0x18C` на `+0x28C` — ошибка в 256 байт. Проверяется по коду выше:
какой бы ни была раскладка, `iuser1` обязан быть на `+0x18C`.
Это и подтвердило, что `physinfo = 256` (`pm_info.h:35`).

---

## 3. `weapon_data_t` — предсказание оружия

```c
typedef struct weapon_data_s
{
    int   m_iId;                    // +0x00
    int   m_iClip;                  // +0x04  патроны в магазине
    float m_flNextPrimaryAttack;    // +0x08
    float m_flNextSecondaryAttack;  // +0x0C
    float m_flTimeWeaponIdle;       // +0x10
    int   m_fInReload;              // +0x14
    int   m_fInSpecialReload;       // +0x18
    float m_flNextReload;           // +0x1C
    float m_flPumpTime;             // +0x20
    float m_fReloadTime;            // +0x24
    float m_fAimedDamage;           // +0x28
    float m_fNextAimBonus;          // +0x2C
    int   m_fInZoom;                // +0x30
    int   m_iWeaponState;           // +0x34
    int   iuser1;                   // +0x38
} weapon_data_t;                    // 60 байт
```

Массив `g_weapon_data[64]` — по одному на слот. Заполняется в
`HUD_TxferPredictionData`.

### Зачем это чита

| поле | применение |
|---|---|
| `m_iClip` | **точный** счётчик патронов (ESP-патроны) |
| `m_flNextPrimaryAttack` | когда можно стрелять — основа auto-pistol |
| `m_fInReload` | перезаряжается ли (для fast reload timing) |
| `m_fInZoom` | в зуме ли (для no-scope и FOV) |
| `m_iWeaponState` | режим оружия (авто/очередь/силуэт) |

Функция `sh_auto_pistol` в SDK сейчас использует
`g_pmove->next_primary_attack` — это **другой** указатель (из
`playermove_t`). Правильнее — `weapon_data_t.m_flNextPrimaryAttack`,
потому что он учитывает предсказание.

---

## 4. `HUD_TxferPredictionData` — сердце предсказания

```
0x019461D0
```

Копирует **5** структур:

```c
void HUD_TxferPredictionData(entity_state_t *ps, const entity_state_t *pps,
                             clientdata_t *pcd, const clientdata_t *ppcd,
                             weapon_data_t *wd, const weapon_data_t *pwd)
{
    // 1. ps  <- pps   (состояние сущности)
    // 2. pcd <- ppcd  (данные локального игрока)
    // 3. wd  <- pwd   (данные оружия)
}
```

Смысл: клиент сохраняет, что было предсказано, и сверяет с тем, что пришло
от сервера. Если расхождение — «prediction error», который движок гасит
сглаживанием (кроме телепорта, где `cl_predict_error` сбрасывается).

---

## 5. `HUD_ProcessPlayerState` — где ставятся флаги

```
0x01946010
```

```asm
01946010  push esi
01946014  mov esi,[esp+0xC]     ; src
01946018  push edi
0194601C  mov edi,[esp+0xC]     ; dst
01946020  mov eax,[esi+0x4]     ; src->number
01946023  mov ecx,[esi+0x110]   ; src->iuser4
01946029  mov ...
```

Именно здесь копируется `curstate` и выставляются флаги игрока
(dead, ducking, onground) на основе `entity_state_t`.

---

## 6. Порядок за кадр

```
сервер:   UpdateClientData   → clientdata_t + weapon_data_t
              ↓ сеть
клиент:   HUD_ProcessPlayerState  (0x01946010)
          HUD_TxferLocalOverrides (0x01945FC0)  ← локальные правки поверх серверных
          HUD_TxferPredictionData (0x019461D0)  ← сохранение для предсказания
              ↓
          CL_CreateMove   (0x01958CC0)  ← строит usercmd_t
              ↓
          HUD_PlayerMove  (0x01963050)  ← прогоняет физику локально
              ↓
          HUD_UpdateClientData (0x01944A80)
              ↓
          HUD_Redraw      (0x01944A60)
```

Это **полный конвейер кадра**. Любая правка `usercmd_t` попадает в него в
`CL_CreateMove` — до `HUD_PlayerMove`, поэтому физика уже учитывает
antiaim/bhop в этом же кадре.

---

## 7. Что исправлено в SDK по этому документу

1. `sh_client_data_t` в `sh_sdk.h` был **неверен**: `qbool fov` в начале,
   `u8 m_flNextAttack` в конце. Правильная раскладка — 476 байт, добавлена
   с `static_assert`.
2. `physinfo[256]`, а не 512 — иначе `iuser*` уезжает на 256 байт.
3. `sh_auto_pistol` стоит перевести на `weapon_data_t.m_flNextPrimaryAttack`.
4. Добавлен `sh_weapon_data_t` (60 байт) для патронов и таймингов.

---

## 8. Следующее

- `12-demo.md` — `demo_api`, формат `.dem`, разбор демок
- перенос `auto_pistol` на `weapon_data_t`
