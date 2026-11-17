# Реверс CS 1.6 — `weapon_data_t`: предсказанные таймеры оружия

Источник: `FWGS/cs16-client`, `common/weaponinfo.h:24`.

---

## 1. Что это

Массив из **64** структур (по одной на каждое оружие), который движок
передаёт клиенту через предсказание. В отличие от `clientdata_t`
(там только текущее оружие), здесь — **все оружия сразу**, и в них
предсказанные таймеры.

```c
typedef struct weapon_data_s
{
    int   m_iId;                    // +0x00  id оружия
    int   m_iClip;                  // +0x04  патроны
    float m_flNextPrimaryAttack;    // +0x08  когда можно стрелять
    float m_flNextSecondaryAttack;  // +0x0C  когда можно alt-fire
    float m_flTimeWeaponIdle;       // +0x10  простой
    int   m_fInReload;              // +0x14  1 = перезаряжается
    int   m_fInSpecialReload;       // +0x18  для дробовика/glock burst
    float m_flNextReload;           // +0x1C  следующий шаг перезарядки
    float m_flPumpTime;             // +0x20  помпа (дробовик)
    float m_fReloadTime;            // +0x24  длительность перезарядки
    float m_fAimedDamage;           // +0x28  для sniper zoom
    float m_fNextAimBonus;          // +0x2C
    int   m_fInZoom;                // +0x30  в прицеле
    int   m_iWeaponState;           // +0x34
    int   iuser1..4;                // +0x38..0x44
    float fuser1..4;                // +0x48..0x54
} weapon_data_t;                    // sizeof = 88 (0x58)
```

`88 * 64 = 5632` байт — таблица целиком.

Проверка размера **на уровне компилятора:**
```c
typedef char sh_weapon_data_size_check[(sizeof(sh_weapon_data_t) == 88) ? 1 : -1];
```

---

## 2. Чем это лучше `clientdata_t`

| | `clientdata_t` | `weapon_data_t` |
|---|---|---|
| оружий | только текущее | все 64 |
| `next_primary_attack` | через `weapon_data`-указатель | прямо в структуре |
| `in_reload` | нет | есть |
| `reload_time` | нет | есть |
| `pump_time` | нет | есть |

**Вывод:** для точного auto-pistol, fast reload и double tap нужен
именно `weapon_data_t`, а не `clientdata_t`.

---

## 3. Что даёт каждая фича

### Fast reload

```c
sh_weapon_data_t *w = &g_weapon_data[weapon_id];

if (w->clip == 0 && !w->in_reload && w->next_primary_attack <= now)
    cmd->buttons |= IN_RELOAD;
```

`m_fInReload` позволяет **не жать лишний раз**: если уже перезаряжаемся,
повторное нажатие ничего не даст (проверка в `weapons.cpp:1102`:
`!m_fInReload`).

`m_flNextReload` — момент, когда перезарядка **завершится**. Отсюда можно
предсказать, когда оружие будет готово, и не терять кадры.

### Double tap (честный предел)

```c
f32 ready = w->next_primary_attack - now;

if (ready < 0.0f)
    cmd->buttons |= IN_ATTACK;       // готов стрелять
else if (ready < frametime * 1.5f)
    cmd->buttons |= IN_ATTACK;       // досыл в окно
```

`next_primary_attack` в `weapon_data_t` — это **уже предсказанное**
значение, то есть серверное с учётом `- flCreep` из
`GetNextAttackDelay` (см. `18-weapon-timers.md`). Поэтому допуск в
полкадра работает.

### Pump (дробовик)

`m_flPumpTime` — когда закончится анимация помпы у M3/XM1014.
Между выстрелами дробовик требует помпу; если `pump_time > now`, второй
выстрел не пройдёт, сколько ни жми.

### Zoom (снайперка)

`m_fInZoom` — 1 в прицеле. Отсюда `no_scope` можно делать умнее:
не снимать `IN_ATTACK2` когда зум нужен (например, для AWP
quick-scope).

---

## 4. Как достать массив

В `cl_enginefuncs_t` нет прямого указателя на `weapon_data_t`.
Он приходит в `HUD_TxferPredictionData`:

```c
void HUD_TxferPredictionData(struct entity_state_s *ps,
                            const struct entity_state_s *pps,
                            struct clientdata_s *pcd,
                            const struct clientdata_s *ppcd,
                            struct weapon_data_s *wd,      // ← вот он
                            const struct weapon_data_s *pwd);
```

В `cl_dll.h:74` объявление есть, значит движок зовёт эту функцию
каждый кадр. Чтобы получить данные:

1. Найти `HUD_TxferPredictionData` в `client.dll` (одна из 8 сигнатур);
2. Хукнуть её через MinHook;
3. В хуке скопировать `wd[64]` в свой буфер.

**Что уже сделано:** структура добавлена в `include/sh_sdk.h` с
`sizeof == 88` (проверка компилятором). Хук и биндинг — следующий шаг;
если `TxferPredictionData` не найдётся паттерном, fallback остаётся на
`g_pmove->next_primary_attack` (одно оружие, но работает).

---

## 5. Почему 88, а не 84

Без последних четырёх `float fuser` было бы 84 с выравниванием 4.
Но компилятор MSVC выравнивает структуру по 4, а 22 поля × 4 байта = 88.
Проверено: попытка объявить `sizeof == 84` даёт ошибку компиляции.

Это важно: если бы структура была описана на 4 байта короче,
`wd[1]` указывал бы внутрь `wd[0]`, и все таймеры «поехали» бы —
классическая ошибка при реверсе.

---

## 6. Итоговая таблица смещений

| поле | смещение | тип |
|---|---:|---|
| `m_iId` | `0x00` | i32 |
| `m_iClip` | `0x04` | i32 |
| `m_flNextPrimaryAttack` | `0x08` | f32 |
| `m_flNextSecondaryAttack` | `0x0C` | f32 |
| `m_flTimeWeaponIdle` | `0x10` | f32 |
| `m_fInReload` | `0x14` | i32 |
| `m_fInSpecialReload` | `0x18` | i32 |
| `m_flNextReload` | `0x1C` | f32 |
| `m_flPumpTime` | `0x20` | f32 |
| `m_fReloadTime` | `0x24` | f32 |
| `m_fAimedDamage` | `0x28` | f32 |
| `m_fNextAimBonus` | `0x2C` | f32 |
| `m_fInZoom` | `0x30` | i32 |
| `m_iWeaponState` | `0x34` | i32 |
| `iuser1..4` | `0x38..0x44` | i32 ×4 |
| `fuser1..4` | `0x48..0x54` | f32 ×4 |
| **sizeof** | **`0x58` = 88** | |
