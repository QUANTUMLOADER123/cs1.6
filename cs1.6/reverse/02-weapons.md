# Реверс CS 1.6 — оружие

Источники: `dlls/wpn_shared/wpn_*.cpp` (по файлу на ствол), `dlls/wpn_shared.h`
(константы), `dlls/weapontype.h` (enum оружия), `cl_dll/cs_wpn/cs_weapons.cpp`
(общая база `CBasePlayerWeapon`).

Проверено по бинарнику `mp.dll`: `CAK47`, `CBasePlayerWeapon` (40 вхождений —
RTTI-имена строк, `weapon_ak47` = экспорт #340 по адресу `0x100E2B10`).

---

## 1. Скорость движения с оружием

Каждый ствол задаёт **свою** максимальную скорость игрока. Это одна из самых
недооценённых механик: с AWP ты физически медленнее.

| оружие | скорость | | оружие | скорость |
|---|---:|---|---|---:|
| Scout | **260** | | M4A1 | 230 |
| C4 / Deagle / Glock / MP5 / P228 / USP / MAC10 / TMP / Five-SeveN | 250 | | M3 | 230 |
| P90 | 245 | | Galil / Famas / AUG | 240 |
| AUG / Famas / Galil / XM1014 | 240 | | SG552 | 235 |
| SG552 | 235 | | AK47 | **221** |
| M249 | 220 | | AWP / G3SG1 / SG550 | **210** |

В прицеле: AWP `150`, G3SG1/SG550 `150`, SG552 `200`, Scout `220`.

---

## 2. Урон и затухание

`flRangeModifier` — множитель урона на единицу расстояния. Урон падает
геометрически: `damage * pow(range_mod, distance)`.

| оружие | урон | range_mod | перезарядка | магазин |
|---|---:|---:|---:|---:|
| AWP | **115** | 0.99 | 2.5 | 10 |
| Scout | 75 | 0.98 | 2.0 | 10 |
| G3SG1 | 80 | 0.98 | 3.5 | 20 |
| SG550 | 70 | 0.98 | 3.35 | 30 |
| Deagle | 54 | **0.81** | 2.2 | 7 |
| AK47 | 36 | 0.98 | 2.45 | 30 |
| USP | 34 | 0.79 | 2.7 | 12 |
| M4A1 | 32 | 0.97 | 3.05 | 30 |
| AUG | 32 | 0.96 | 3.3 | 30 |
| M249 | 32 | 0.97 | 4.7 | 100 |
| P228 | 32 | 0.80 | 2.7 | 13 |
| Galil | 30 | 0.98 | 2.45 | 35 |
| FAMAS | 30 | 0.96 | 3.3 | 25 |
| UMP45 | 30 | 0.82 | 3.5 | 25 |
| MAC10 | 29 | 0.82 | 3.15 | 30 |
| MP5 | 26 | 0.84 | 2.63 | 30 |
| Glock | 25 | 0.75 | 2.2 | 20 |
| P90 | 21 | 0.885 | 3.4 | 50 |
| Five-SeveN | 20 | 0.885 | 2.7 | 20 |
| TMP | 20 | 0.85 | 2.12 | 30 |

Обрати внимание на **Deagle 0.81** — в отличие от AWP 0.99, он теряет урон
очень быстро. Поэтому deagle-one-deag работает только в упор.

---

## 3. Три класса модели разброса ⭐

Это главное, что нужно понимать про стрельбу в CS 1.6. Есть **три разных
формулы**, и оружие принадлежит одной из них.

### 3.1 Класс «винтовки» (AK, M4A1, AUG, Galil, FAMAS, M249, SG552)

Три состояния по скорости игрока + растущая неточность от выстрелов:

```c
// AK47 — PrimaryAttack()
if (!(flags & FL_ONGROUND))              // в воздухе
    Fire(0.04 + 0.4  * m_flAccuracy, 0.0955);
else if (velocity.Length2D() > 140)      // бежит
    Fire(0.04 + 0.07 * m_flAccuracy, 0.0955);
else                                     // стоит/медленно
    Fire(0.0275 * m_flAccuracy, 0.0955);

// накопление неточности — КУБИЧЕСКОЕ
m_flAccuracy = (m_iShotsFired³) / 200 + 0.35f;   // AK47
m_flAccuracy = (m_iShotsFired³) / 220 + 0.30f;   // M4A1
if (m_flAccuracy > 1.25f) m_flAccuracy = 1.25f;  // потолок
```

Кубический рост — вот почему AK «уезжает» к 5-6 выстрелу. Первый выстрел
точен: `0.0275 * 0.2 = 0.0055`.

### 3.2 Класс «пистолеты» (Deagle, Glock, USP, P228)

Инвертированная формула — `m_flAccuracy` это **точность**, а не разброс:

```c
// Deagle
Fire(0.25 * (1 - m_flAccuracy), 0.3);   // в воздухе
Fire(0.115 * (1 - m_flAccuracy), 0.3);  // бежит
Fire(0.13 * (1 - m_flAccuracy), 0.3);   // стоит

// точность падает нелинейно по выстрелам:
// 0.9 → 0.55 → 0.4 → ...
```

При `m_flAccuracy = 0.9` стоя: `0.13 * 0.1 = 0.013` — точнее AK.
При `0.55`: `0.13 * 0.45 = 0.0585` — уже хуже.

### 3.3 Класс «снайперки» (AWP) — фиксированные константы

Никакого накопления вообще. Только позиция:

```c
if (!(flags & FL_ONGROUND))            Fire(0.85);   // в прыжке — почти не попасть
else if (velocity.Length2D() > 140)    Fire(0.25);   // бежит
else if (velocity.Length2D() > 10)     Fire(0.1);    // идёт
else if (flags & FL_DUCKING)           Fire(0.0);    // в приседе — АБСОЛЮТНО точно
else                                   Fire(0.001);  // стоит
```

**`Fire(0.0)` в приседе — математически нулевой разброс.** Это механика
«сидячая AWP» ровно на уровне формул.

---

## 4. Как spread превращается в отклонение пули

```c
// CBaseEntity::FireBullets3 — cl_dll/cs_wpn/cs_weapons.cpp:518
x = SharedRandomFloat(seed,   -0.5, 0.5) + SharedRandomFloat(seed+1, -0.5, 0.5);
y = SharedRandomFloat(seed+2, -0.5, 0.5) + SharedRandomFloat(seed+3, -0.5, 0.5);

return Vector(x * flSpread, y * flSpread, 0.0f);
```

Ключевые детали:

- **сумма двух равномерных** → треугольное распределение (центр вероятнее края)
- **`SharedRandom`** — детерминированный по `random_seed`, поэтому клиент и
  сервер получают **одинаковое** отклонение (основа предсказания)
- результат — смещение в плоскости, перпендикулярной направлению взгляда

---

## 5. Отдача (KickBack)

```c
// порядок аргументов:
KickBack(up_base, lateral_base, up_modifier, lateral_modifier,
         up_max, lateral_max, direction_change);

// AK47 — три состояния:
KickBack(1.5, 0.45, 0.225, 0.05, 6.5, 2.5, 7);   // бежит
KickBack(2.0, 1.0,  0.5,   0.35, 9.0, 6.0, 5);   // в воздухе
KickBack(0.9, 0.35, 0.15,  0.025, 5.5, 1.5, 9);  // в приседе
```

Отдача в приседе **в 2.2 раза слабее** воздушной по вертикали
(`0.9` против `2.0`) и в **14 раз** по горизонтали (`0.35` против `1.0`).

Отдача применяется к `punchangle`, который потом **добавляется к углам
прицеливания** при выстреле:

```c
UTIL_MakeVectors(pev->v_angle + pev->punchangle);
```

Поэтому в `PLAYBACK_EVENT_FULL` уходят `punchangle.x*100` и `.y*100` — клиент
воспроизводит ту же отдачу визуально.

---

## 6. Дробовики — отдельная система

Не `FireBullets3`, а `FireBullets` с вектором конуса и фиксированным числом
дробин:

```c
// M3: 9 дробин
m_pPlayer->FireBullets(9, vecSrc, vecAiming, M3_CONE_VECTOR, 3000, BULLET_PLAYER_BUCKSHOT, 0);

// XM1014: 6 дробин
m_pPlayer->FireBullets(6, vecSrc, vecAiming, XM1014_CONE_VECTOR, 3048, BULLET_PLAYER_BUCKSHOT, 0);
```

Здесь `iTracerFreq = 0` — трассеров нет.

---

## 7. Структура класса

```
CBaseEntity
  └── CBasePlayerItem
        └── CBasePlayerWeapon        ← cl_dll/cs_wpn/cs_weapons.cpp
              ├── CAK47              ← dlls/wpn_shared/wpn_ak47.cpp
              ├── CM4A1
              ├── CAWP
              └── ... (29 стволов)
```

Виртуальные методы, которые переопределяет каждый ствол:
`Spawn`, `Precache`, `GetItemInfo`, `Deploy`, `Holster`, `PrimaryAttack`,
`SecondaryAttack`, `Reload`, `WeaponIdle`, `ItemPostFrame`.

`LINK_ENTITY_TO_CLASS(weapon_ak47, CAK47)` — макрос, регистрирующий класс под
именем, по которому его создаёт движок (`give weapon_ak47`).

### Найденный в бинарнике скелет

| VA в `mp.dll` | что |
|---|---|
| `0x100539F0` | `GiveFnptrsToDll` — движок отдаёт 158 указателей |
| `0x100266A0` | `GetEntityAPI` |
| `0x10026820` | `GetNewDLLFunctions` |
| `0x100E2B10` | `weapon_ak47` (entry point Spawn) |
| `0x100A41D0` | `CWeaponBox::BombThink` |
| `0x100A4440` | `CWeaponBox::Kill` |
| `0x1017FF38` | строка `"CAK47"` (RTTI имя класса) |

---

## 8. Чего не хватает в эталонных исходниках

Честная оценка: `cs16-client` содержит **клиентскую** часть и **заголовки**
серверной, но не серверную реализацию. Отсутствуют:

- `dlls/weapons.cpp` — серверный `FireBullets3` (в репозитории только
  клиентская заглушка, возвращающая смещение)
- `dlls/player.cpp` — серверная логика игрока
- `dlls/client.cpp`, `dlls/gamerules.cpp` — реализация правил

Поэтому `CBasePlayerWeapon::KickBack` в репозитории — **пустая функция**
(клиент не считает отдачу, он её получает). Серверную часть надо
восстанавливать по `mp.dll`.

Восстановленный контракт по бинарнику: `sizeof(enginefuncs_t) = 632`
(подтверждено `rep movs` с `ecx=0x9E` в `GiveFnptrsToDll`).
