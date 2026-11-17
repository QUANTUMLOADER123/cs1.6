# Реверс CS 1.6 — система движения (player movement)

Источник: `pm_shared/pm_shared.cpp` (3248 строк) из эталонных исходников
`FWGS/cs16-client` (реверс-инжиниринг клиента CS 1.6 в исходники).

`pm_shared` — это **общий код предсказания** между клиентом и сервером. Один и
тот же код исполняется на сервере (авторитетно) и в `client.dll` (для предсказания
без задержки). Поэтому это буквально то, что определяет физику игры.

---

## 1. Карта функций

Порядок вызовов за один тик — `PM_Move()`:

| # | функция | строка | назначение |
|---|---|---|---|
| 1 | `PM_CheckVelocity` | 557 | clamp скорости по `maxvelocity`, NaN-защита |
| 2 | `PM_CheckWaterJump` | 2507 | проверка прыжка из воды |
| 3 | `PM_CategorizePosition` | 1445 | определить `onground`, `waterlevel` |
| 4 | `PM_CheckStuck` | 1530 | разлипание из геометрии |
| 5 | `PM_AddGravity` | 2162 | `velocity.z -= gravity * dt` |
| 6 | **`PM_WalkMove`** | 897 | наземное движение (выбор air/water/ground) |
| 7 | `PM_FlyMove` | 678 | **главный коллайдер**: свип-тесты со скольжением |
| 8 | `PM_CheckFalling` | 2582 | урон от падения |

Служебные:

- `PM_Friction` (1085) — трение о землю
- `PM_Accelerate` (856) — разгон на земле
- `PM_AirAccelerate` (1168) — разгон в воздухе
- `PM_AirMove` (1315) — движение в воздухе
- `PM_WaterMove` (1208) — движение в воде
- `PM_Jump` (2363), `PM_PreventMegaBunnyJumping` (2336)
- `PM_Duck` (1883), `PM_UnDuck` (1830) — присед
- `PM_LadderMove` (1988), `PM_Ladder` (2105) — лестницы

---

## 2. Параметры движения (`movevars_t`)

Структура целиком — `pm_shared/pm_movevars.h`. Значения задаются сервером через
квары `sv_*`, но **имена полей другие**:

```c
typedef struct movevars_s
{
    float gravity;          // sv_gravity        (по умолч. 800)
    float stopspeed;        // sv_stopspeed      (75)
    float maxspeed;         // sv_maxspeed       (320)
    float spectatormaxspeed;// 500
    float accelerate;       // sv_accelerate     (10)
    float airaccelerate;    // sv_airaccelerate  (10)
    float wateraccelerate;  // sv_wateraccelerate(10)
    float friction;         // sv_friction       (4)
    float edgefriction;     // 2.0 — множитель у края обрыва
    float waterfriction;    // sv_waterfriction  (1)
    float entgravity;       // 1.0
    float bounce;           // 1.0 — упругость
    float stepsize;         // sv_stepsize       (18)
    float maxvelocity;      // sv_maxvelocity    (2000)
    float zmax;             // 4096
    float waveHeight;       // 0
    qboolean footsteps;     // 1
    char skyName[32];
    float rollangle, rollspeed;
    float skycolor_r/g/b;
    float skyvec_x/y/z;
} movevars_t;   // всего 26 полей
```

---

## 3. Физика: точные формулы

### 3.1 Трение (земля) — `PM_Friction`

```c
speed = Length(vel);
if (speed < 0.1) return;                     // мёртвая зона

if (onground != -1) {
    // проба на край обрыва: трассируем на 34 вниз со смещением 16 по направлению движения
    trace = PM_PlayerTrace(start, stop, PM_NORMAL, -1);

    friction = (trace.fraction == 1.0f)      // ничего не нашли — мы у края!
        ? movevars->friction * movevars->edgefriction   // 4 * 2 = 8
        : movevars->friction;                           // 4

    friction *= pmove->friction;             // множитель игрока (обычно 1)

    control = (speed < movevars->stopspeed) ? movevars->stopspeed : speed;
    drop += friction * (control * frametime);
}
```

Ключевое: **`edgefriction` удваивает трение у края обрыва** — поэтому на краю
платформы скорость падает быстрее. Это влияет на bhop-маршруты.

### 3.2 Разгон на земле — `PM_Accelerate`

```c
addspeed = wishspeed - DotProduct(velocity, wishdir);
if (addspeed <= 0) return;                   // уже быстрее wishspeed — не разгоняем

accelspeed = accel * frametime * wishspeed * pmove->friction;
if (accelspeed > addspeed) accelspeed = addspeed;

velocity += wishdir * accelspeed;            // по всем 3 осям
```

Классическая формула Quake. Разгон только **до** `wishspeed` по направлению.

### 3.3 Разгон в воздухе — `PM_AirAccelerate` ⭐

```c
wishspd = wishspeed;
if (wishspd > 30) wishspd = 30;              // ⭐ CAP = 30

currentspeed = DotProduct(velocity, wishdir);
addspeed = wishspd - currentspeed;
if (addspeed <= 0) return;

accelspeed = accel * wishspeed * frametime * pmove->friction;   // ← wishspeed, НЕ wishspd!
if (accelspeed > addspeed) accelspeed = addspeed;

velocity += wishdir * accelspeed;
```

**Вот источник air-strafe.** Разница `wishspd` (ограничено 30) и `wishspeed`
(полное, например 320) — это и есть эксплойт:

- проверка `addspeed` идёт против **30**
- а величина `accelspeed` считается от **полной** `wishspeed`

Поэтому при плавном повороте мыши в воздухе каждый тик добавляется до 30
единиц скорости вбок, и скорость растёт без предела (пока не ограничит
`maxvelocity`).

### 3.4 Прыжок и bhop — `PM_Jump` + `PM_PreventMegaBunnyJumping`

```c
maxscaledspeed = 1.2f * pmove->maxspeed;     // BUNNYJUMP_MAX_SPEED_FACTOR = 1.2
if (maxscaledspeed <= 0) return;

spd = Length(velocity);
if (spd <= maxscaledspeed) return;           // до 384 (при maxspeed 320) — не трогаем

fraction = (maxscaledspeed / spd) * 0.8f;    // ⭐ 0.8 — «штраф» за mega-bunny
VectorScale(velocity, fraction, velocity);   // режем скорость
```

Итого: при прыжке со скоростью выше `1.2 * maxspeed` скорость умножается на
`(1.2*maxspeed / spd) * 0.8`. То есть лишняя скорость **срезается на 20%**,
а не обнуляется. Это то, что делает возможным «mega bunny hop» со скоростью
чуть выше предела.

Прыжок в воде: `velocity[2] = 100` (вода) / `80` (слизь) / `50` (лава).

### 3.5 Гравитация — `PM_AddGravity`

```c
ent_gravity = movevars->gravity;
if (pmove->gravity) ent_gravity *= pmove->gravity;   // per-entity множитель
velocity[2] -= ent_gravity * frametime;
```

Плюс `PM_FixupGravityVelocity` — при полёте вверх по склону компенсирует.

---

## 4. Константы (`pm_shared.h`)

```c
#define MAX_CLIMB_SPEED             200     // макс. скорость на лестнице
#define PM_PLAYER_MAX_SAFE_FALL_SPEED 580   // ~20 футов, выше — урон
#define BUNNYJUMP_MAX_SPEED_FACTOR  1.2f    // ← источник bhop-механики
#define PM_DEAD_VIEWHEIGHT          (-8)    // вид от мёртвого
```

---

## 5. Коллайдер `PM_FlyMove` (678)

Свип-тест: до 4 итераций. На каждой:

1. `PM_PlayerTrace` из текущей позиции в целевую.
2. Если `fraction < 1` — упёрлись: двигаемся на `fraction`, затем
   `PM_ClipVelocity` (скольжение вдоль плоскости).
3. `allFraction == 1` — прошли насквозь, выходим.
4. Иначе — убираем из плана нормальную компоненту и повторяем.

```c
// PM_ClipVelocity (595)
backoff = DotProduct(in, normal) * overbounce;   // overbounce = 1.0
for (i = 0; i < 3; i++) {
    change = normal[i] * backoff;
    out[i] = in[i] - change;
}
```

Это и есть surf/ramp-slide: скорость проецируется на плоскость, `overbounce`
позволяет подпрыгивать от рамп.

---

## 6. Что это даёт для чита

| функция | что можно сделать |
|---|---|
| `PM_AirAccelerate` | bhop/air-strafe: знать cap 30 и множитель `wishspeed` |
| `PM_PreventMegaBunnyJumping` | предсказать срез скорости при прыжке |
| `PM_Friction` | понять, почему на краю платформы скорость падает (edgefriction=2) |
| `PM_Jump` | автопрыжок: `IN_JUMP` каждый тик, когда `onground != -1` |
| `PM_Duck` / `PM_UnDuck` | duck-jump, fast-crouch |
| `PM_LadderMove` | fast-ladder (скорость 200) |

---

## 7. Следующий шаг

- Сверить эти формулы с **дизассемблером `client.dll`** из репозитория —
  убедиться, что билд Steam совпадает с открытыми исходниками (адреса
  `HUD_PlayerMove`, `HUD_PlayerMoveInit` уже найдены в
  `cs1.6/analysis/client/index.json`).
- Затем: оружие (разброс, отдача), сеть (кодирование usercmd), рендер.


---

## Урон от падения (player.h:61-77, player.cpp:5394-5451)

```c
MAX_PLAYER_FATAL_FALL_SPEED     = 1100.0f
MAX_PLAYER_SAFE_FALL_SPEED      =  500.0f
DAMAGE_FOR_FALL_SPEED           = 100.0f / (1100.0f - 500.0f)   // 0.16666
PLAYER_FALL_PUNCH_THRESHHOLD    =  250.0f
```

Урон считается от **вертикальной скорости при приземлении**
(`m_flFallVelocity = -pev->velocity.z`, ставится в `player.cpp:4997`):

```
damage = (fall_velocity - 500.0f) * 0.16666
```

| скорость при приземлении | урон |
|---:|---:|
| 250 | 0 (только звук шагов) |
| 500 | 0 |
| 600 | 16 |
| 800 | 50 |
| 1100 | 100 — смерть |

В `player.cpp:5394`:

```c
if ((pev->flags & FL_ONGROUND) && pev->health > 0
    && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
{
    if (pev->watertype != CONTENT_WATER)
        if (m_flFallVelocity > MAX_PLAYER_SAFE_FALL_SPEED)
        {
            float flFallDamage = g_pGameRules->FlPlayerFallDamage(this);
            ...
            TakeDamage(pev, pev, flFallDamage, DMG_FALL);
        }
}
```

**Для SDK:** урон считает сервер, поэтому `no_fall_damage` на клиенте
невозможен в принципе. Проверено отдельно: `pmove->flFallVelocity`
в `pm_shared.cpp:2863` **только читается** для звука приземления
(`PM_CheckFalling`, 2584) и отката камеры (`punchangle[2] = v * 0.013`).
Запись в него из клиентского хука `HUD_PlayerMove` **ни на что не влияет** —
серверная сторона в `player.cpp:4997` берёт `-pev->velocity.z` из своих
данных.

Поэтому в SDK чекбокс переименован в **`fall damage warn`**: он считает
`(fall_velocity - 500) * 0.16666` и показывает `fall -N hp` на экране,
пока игрок падает. Это честно и полезно; обещать нулевой урон нельзя.

Это же объясняет, почему bhop «съедает» урон: при прыжке с зажатым
`IN_JUMP` вертикальная скорость в момент касания близка к нулю, и
`m_flFallVelocity` до 500 просто не доходит.
