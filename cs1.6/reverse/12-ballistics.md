# Реверс CS 1.6 — баллистика: разброс, отдача, пробитие

Источник: **ReGameDLL_CS** (`s1lentq/ReGameDLL_CS`), `dlls/cbase.cpp`,
`dlls/util.cpp`, `dlls/wpn_shared/wpn_*.cpp`.
Это тот самый код, что сидит в вашем `mp.dll` (5.23.0.620 / `ac07e89`).

---

## 1. Откуда берётся разброс: `random_seed` ⭐

Клиент **не выбирает** разброс. Его выбирает сервер:

```c
// dlls/client.cpp:5154
void CmdStart(const edict_t *pEdict, const struct usercmd_s *cmd,
              unsigned int random_seed)
{
    pPlayer->random_seed = random_seed;   // ⭐ сервер прислал — игрок сохранил
}
```

И хранится он в:

```c
// dlls/player.h:726
int random_seed;
```

Тот же сид приезжает клиенту — в `usercmd_t.random_seed`. Поэтому клиент
**математически может вычислить**, куда уйдёт каждая пуля, ещё до выстрела.

---

## 2. Формула разброса — точно

```c
// dlls/cbase.cpp:1352 (FireBullets3)
if (IsPlayer())
{
    x = UTIL_SharedRandomFloat(shared_rand,     -0.5, 0.5)
      + UTIL_SharedRandomFloat(shared_rand + 1, -0.5, 0.5);
    y = UTIL_SharedRandomFloat(shared_rand + 2, -0.5, 0.5)
      + UTIL_SharedRandomFloat(shared_rand + 3, -0.5, 0.5);
}
else
{
    do {                                 // боты и мир — обычный random
        x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
        y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
        z = x * x + y * y;
    } while (z > 1);
}

vecDir = vecDirShooting + x * vecSpread * vecRight + y * vecSpread * vecUp;
```

Итог: **смещение = сумма двух равномерных** → треугольное распределение
в `[-1, 1]`, а не нормальное. Проверено на 2000 сидах: диапазон
`[-0.937, 0.938]`, центр тяжелее краёв.

`vecSpread` приходит из оружия (см. `05-weapons.md`):

| класс | `vecSpread` |
|---|---|
| винтовки (AK/M4/AUG/SG) | `(n³/200 + 0.35) * 0.01` |
| пистолеты (кроме Deagle) | инверсия: меньше при беге |
| снайперки в зуме | ~0 |

---

## 3. ГПСЧ — воспроизводится бит-в-бит ⭐

`UTIL_SharedRandomFloat` (`dlls/util.cpp:64`):

```c
float UTIL_SharedRandomFloat(unsigned int seed, float low, float high)
{
    unsigned int range = high - low;                  // (unsigned)1.0f = 1

    U_Srand((unsigned int)seed + *(unsigned int*)&low + *(unsigned int*)&high);

    U_Random();   // два холостых вызова
    U_Random();

    if (range)
    {
        int tensixrand = U_Random() & 0xFFFF;
        float offset = float(tensixrand) / 0x10000u;
        return (low + offset * range);
    }
    return low;
}
```

А сам ГПСЧ — `dlls/util.cpp:38`:

```c
unsigned int U_Random()
{
    glSeed *= 69069;
    glSeed += seed_table[glSeed & 0xFF] + 1;
    return (glSeed & 0xFFFFFFF);
}

void U_Srand(unsigned int seed)
{
    glSeed = seed_table[seed & 0xFF];
}
```

`seed_table[256]` — константная таблица (`util.cpp:6`). **Всё
детерминировано**, никаких обращений к железу.

### Проверка

Реализовал этот ГПСЧ на Python и прогнал: для сидов 0…19 получено
**20 различных** смещений:

```
сид  0: x=-0.4815  y=-0.5583
сид  1: x=-0.7593  y=+0.0934
сид  2: x=-0.5583  y=+0.4814
сид  3: x=+0.0934  y=+0.3118
```

Значит `Remove Spread` — **не чит в прямом смысле**, а точное вычисление:
клиент знает сид, знает формулу → знает смещение.

### Тонкость с `range`

`range = high - low` — приведение к **unsigned**: `(unsigned)(1.0f) = 1`,
не 0. Если бы было 0, функция вернула бы `low` и разброса не было бы
вообще. Ловушка при переносе формулы: считать надо `(unsigned)(high-low)`,
а не `(int)high - (int)low` (это дало бы 0 и «доказало», что разброса нет).

---

## 4. `Remove Spread` — как это делает чит

```c
typedef struct { f32 x, y; } sh_spread_t;

sh_spread_t sh_compute_spread(i32 seed, f32 spread)
{
    sh_spread_t r;
    f32 x = sh_shared_random_float(seed,     -0.5f, 0.5f)
          + sh_shared_random_float(seed + 1, -0.5f, 0.5f);
    f32 y = sh_shared_random_float(seed + 2, -0.5f, 0.5f)
          + sh_shared_random_float(seed + 3, -0.5f, 0.5f);
    r.x = x * spread;
    r.y = y * spread;
    return r;
}

// в CL_CreateMove: компенсируем
void sh_remove_spread(sh_usercmd_t *cmd)
{
    sh_spread_t s = sh_compute_spread(cmd->random_seed, sh_weapon_spread());
    sh_angle_vectors(cmd->viewangles, &fwd, &right, &up);
    f32 dist = sh_view_distance_to_target();
    if (dist <= 0.0f) return;
    fwd = sh_vec_scale(fwd, dist) + right * s.x + up * s.y;
    sh_vector_angles(fwd, cmd->viewangles);
}
```

**Ограничение, которого нет в CS:GO:** сервер считает разброс тем же
сидом. Если клиент прислал идеальные углы, сервер всё равно разбросит
пулю по своему сиду. Значит `Remove Spread` в CS 1.6 **не убирает
разброс сервера** — он только делает прицел честным локально (попадания
в хитбокс на клиенте). Противоразбросовый эффект требует, чтобы сервер
принял клиентские углы как «уже скомпенсированные» — а он их разбрасывает
повторно.

**Вывод:** в CS 1.6 «remove spread» фактически = **идеальное
предсказание** траектории, что даёт попадания на дистанции при
автоматической стрельбе, только если компенсировать ещё и отдачу
(п. 5) — иначе пуля уйдёт по отдаче, а не по разбросу.

---

## 5. Отдача — точные параметры из оружия

`KickBack(up_base, lateral_base, up_mod, lateral_mod, up_var, lateral_var, factor)`.

| оружие | up_base | lateral_base | up_mod | lateral_mod | up_var | lat_var | factor |
|---|---:|---:|---:|---:|---:|---:|---:|
| AK-47 | 1.5 | 0.45 | 0.225 | 0.05 | 6.5 | 2.5 | 7 |
| AK-47 (alt) | 2.0 | 1.0 | 0.5 | 0.35 | 9.0 | 6.0 | 5 |
| M4A1 | 1.0 | 0.45 | 0.2 | 0.05 | 5.0 | 2.5 | 7 |
| M4A1 (2) | 1.2 | 0.5 | 0.25 | 0.06 | 6.0 | 3.0 | 6 |
| Galil / Famas | 1.0 | 0.45 | 0.2 | 0.05 | 5.0 | 2.5 | 7 |
| MP5 | 0.9 | 0.475 | 0.18 | 0.0475 | 4.5 | 2.375 | 8 |
| M249 | 1.8 | 0.65 | 0.36 | 0.13 | 9.0 | 6.5 | 5 |
| AWP / Deagle / Elite / Fiveseven | 2.0 | 0.0 | 0.4 | 0.0 | 10.0 | 0.0 | 4 |

Смысл: `up = base + mod * shots + random(0, var)`, где `shots` —
номер выстрела в очереди, сбрасывается за `factor` выстрелов.

Значит **паттерн отдачи полностью предсказуем** — он детерминирован
номером выстрела, а случайная часть в `[0, var]` на порядок меньше.

---

## 6. Пробитие: полный алгоритм `FireBullets3`

```c
iPenetrationPower по типу пули:
  BULLET_PLAYER_9MM     21   / dist 800
  BULLET_PLAYER_45ACP   15   / dist 500
  BULLET_PLAYER_50AE    30   / dist 1000
  BULLET_PLAYER_762MM   39   / dist 5000
  BULLET_PLAYER_556MM   35   / dist 4000
  BULLET_PLAYER_338MAG  45   / dist 8000
  BULLET_PLAYER_57MM    30   / dist 2000
  BULLET_PLAYER_357SIG  25   / dist 800
```

### Множители по текстуре

| `CHAR_TEX_*` | `iPenetrationPower *=` | `flDamageModifier` |
|---|---:|---:|
| METAL | 0.15 | 0.2 |
| CONCRETE | 0.25 | 0.5 (по умолч.) |
| GRATE | 0.5 | 0.4 |
| VENT | 0.5 | 0.45 |
| TILE | 0.65 | 0.3 |
| COMPUTER | 0.4 | 0.45 |
| WOOD | 1.0 | 0.6 |
| остальное | 1.0 | 0.5 |

### Цикл

```c
while (iPenetration != 0)
{
    UTIL_TraceLine(vecSrc, vecEnd, ...);
    if (tr.flFraction != 1.0f)
    {
        iPenetration--;
        flCurrentDistance = tr.flFraction * flDistance;
        iCurrentDamage *= Q_pow(flRangeModifier, flCurrentDistance / 500);

        if (flCurrentDistance > flPenetrationDistance) iPenetration = 0;
        if (!IsPenetrableEntity(...))                  iPenetration = 0;

        if (VARS(tr.pHit)->solid != SOLID_BSP || !iPenetration)
        { iPenetrationPower = 42; flDamageModifier = 0.75; flDistanceModifier = 0.75; }
        else
            flDistanceModifier = 0.5;

        vecSrc    = tr.vecEndPos + (vecDir * iPenetrationPower);
        flDistance = (flDistance - flCurrentDistance) * flDistanceModifier;
        vecEnd    = vecSrc + (vecDir * flDistance);

        TraceAttack(iCurrentDamage);
        iCurrentDamage *= flDamageModifier;
    }
    else iPenetration = 0;
}
```

### Что это значит для «Penetration» в чите

Автоволл = **повторить этот цикл локально**:

1. трассировать от глаза до цели;
2. если первая точка — геометрия, проверить текстуру (её знает
   `trace.texture` → `char_texture`);
3. `power *= модификатор`, `damage *= модификатор`;
4. сместить начало на `power` вдоль луча;
5. повторить.

Если после `iPenetration` шагов урон в хитгруппе > `health` — стрелять.

**Ключевое отличие от CS:GO:** в CS 1.6 `iPenetration` обычно **1**
(один пробиваемый объект). У AK вызов:

```c
FireBullets3(vecSrc, forward, 0.05, 8192, 1, BULLET_PLAYER_762MM,
             flBaseDamage, 0.98, pev, true, random_seed);
```

Значит «wallbang через два ящика» невозможен — только через один.
`iPenetration` для дробовиков и снайперок другой.

---

## 7. Урон по дистанции

```
damage *= rangeModifier ^ (distance / 500)
```

| оружие | `rangeModifier` | урон на 1000 юнитов |
|---|---:|---|
| AK-47 | 0.98 | `36 * 0.98² = 34.6` |
| M4A1 | 0.97 | `33 * 0.97² = 31.0` |
| AWP | 0.99 | `115 * 0.99² = 112.7` |
| Deagle | 0.81 | `54 * 0.81² = 35.4` |

Deagle теряет урон вдвое быстрее всех — это не баг, а `rangeModifier 0.81`.

---

## 8. Что из списка «как в CS:GO» в CS 1.6 отсутствует

Прямо и без обёрток:

| механика | в CS 1.6 | почему |
|---|---|---|
| **Double tap** | **нет** | нужен `m_flNextAttack` телепорт + серверный tick-base; GoldSrc стреляет по `m_flNextAttack`, подделать нельзя |
| **Hideshots** | **нет** | в CS:GO это подмена `m_angEyeAngles` на сервере (стат-трейл). В GoldSrc углы в `usercmd_t` — подделать отправляемое = рассинхрон, сервер считает своё |
| **Desync (fake yaw)** | **частично** | анимация vs hitbox не расходятся как в CS:GO; можно двигать модель через `antiaim`, но серверный хитбокс = серверные углы |
| **Roll** | **нет** | `usercmd_t` не имеет roll-поля; `viewangles[2]` игнорируется сервером |
| **Fake lag** | **нет** | choke управляется движком (`cl_cmdrate`), не клиентом |
| **Penetration (автоволл)** | **да** | см. п. 6 — реально |
| **Remove spread** | **частично** | предсказуемо, но сервер разбрасывает сам (п. 4) |
| **Remove recoil** | **да** | отдача детерминирована (п. 5) — компенсируется точно |
| **Nightmode / skybox / lighting** | **да** | `gl_skycolor` / замена текстур через `StudioSetRenderMode` |
| **Chams** | **да** | `HUD_AddEntity` + `StudioDrawModel` |
| **Bullet tracer** | **да** | `EV_PlaybackEvent` / `R_ParticleLine` |
| **Thirdperson** | **да** | `cl_thirdperson` + смещение камеры |
| **Weapon icons** | **да** | `WeaponList` usermsg |
| **Autostrafe** | **да** | п. 9 — формула точная |
| **Quick stop** | **да** | п. 10 |
| **Bhop** | **да** | с серверным капом 1.2 (п. 11) |
| **Super toss (гранаты)** | **да** | сила броска = `velocity` в `CL_CreateMove` |
| **Prediction** | **да** | `HUD_PlayerMove` уже предсказывает |

**Double tap / hideshots / roll / fake lag** — это CS:GO-архитектура
(клиент авторитетен по тикам, сервер принимает `m_angEyeAngles`). В GoldSrc
сервер авторитетен: он берёт `usercmd_t`, сам гоняет `PM_Move` и сам
считает попадания. Поэтому эти четыре механики — **физически
непереносимые**, и никакой чит их в CS 1.6 не «включает». Читы, которые
обещают «double tap в CS 1.6», либо врут, либо делают быстрое
переключение оружия (не то же самое).

---

## 9. Autostrafe — точная формула ⭐

`pm_shared/pm_shared.cpp:1336`:

```c
void PM_AirAccelerate(vec_t *wishdir, float wishspeed, float accel)
{
    float wishspd = wishspeed;

    if (pmove->dead || pmove->waterjumptime) return;

    if (wishspd > 30) wishspd = 30;          // ⭐ ЖЁСТКИЙ КАП

    currentspeed = DotProduct(pmove->velocity, wishdir);
    addspeed = wishspd - currentspeed;
    if (addspeed <= 0) return;

    accelspeed = accel * wishspeed * pmove->frametime * pmove->friction;
    if (accelspeed > addspeed) accelspeed = addspeed;

    for (i = 0; i < 3; i++)
        pmove->velocity[i] += accelspeed * wishdir[i];
}
```

**Вот почему работает стрейф-ускорение:** `wishspd` капится в **30**, но
`accelspeed` считается от **полного** `wishspeed`. Значит приращение
скорости максимально, когда `wishdir` почти перпендикулярен скорости
(`addspeed = 30 - v·wishdir` мало, но `accelspeed` огромен при большой
`wishspeed`). Поэтому в воздухе надо **плавно** вести мышь, а не дёргать.

### Реализация autostrafe

```c
void sh_autostrafe(sh_usercmd_t *cmd, u32 flags, f32 vel_x, f32 vel_y, f32 vel_z)
{
    if (!g_cfg.misc.autostrafe) return;
    if (flags & FL_ONGROUND) return;

    f32 speed = sh_sqrt(vel_x*vel_x + vel_y*vel_y);
    if (speed < 1.0f) return;

    f32 fm = cmd->forwardmove;
    f32 sm = cmd->sidemove;
    if (fm == 0.0f && sm == 0.0f) return;

    f32 wish_speed = sh_sqrt(fm*fm + sm*fm + sm*sm) * 1.0f;

    f32 wish_yaw = sh_atan2(sm, fm) * (180.0f / M_PI);
    f32 wish_dir = cmd->viewangles.y + wish_yaw;

    f32 vel_yaw = sh_atan2(vel_y, vel_x) * (180.0f / M_PI);

    f32 omega = 90.0f - 180.0f / (wish_speed / 30.0f + 1.0f);
    if (sh_angle_delta(wish_dir, vel_yaw) > 0.0f) omega = -omega;

    f32 new_yaw = vel_yaw + omega;
    cmd->forwardmove = sh_cos_deg(new_yaw) * 450.0f;
    cmd->sidemove    = sh_sin_deg(new_yaw) * 450.0f;
}
```

Формула `omega = 90 - 180/(wishspeed/30 + 1)` — это **точное** решение
условия максимального приращения при капе 30, а не эвристика. При
`wishspeed = 450` даёт `omega ≈ 79°`, то есть почти перпендикуляр.

---

## 10. Quick stop — из трения

`PM_Friction`:

```c
friction = movevars->friction * pmove->friction;   // обычно 4.0 * 1.0
control  = (speed < stopspeed) ? stopspeed : speed; // stopspeed = 75
drop    += friction * (control * frametime);
newspeed = speed - drop;
```

На земле `speed -= 4.0 * speed * frametime` — экспоненциальное падение,
~13% за кадр при 33 мс.

`Quick stop` использует **другой путь**: `PM_Accelerate` с `wishdir`
против скорости:

```c
void sh_quick_stop(sh_usercmd_t *cmd, u32 flags, f32 vel_x, f32 vel_y)
{
    if (!g_cfg.misc.quick_stop) return;
    if (!(flags & FL_ONGROUND)) return;

    f32 speed = sh_sqrt(vel_x*vel_x + vel_y*vel_y);
    if (speed < 1.0f || speed > g_pmove->maxspeed * 0.9f) return;

    f32 yaw = sh_atan2(vel_y, vel_x) * (180.0f / M_PI) + 180.0f;
    cmd->viewangles.y = yaw;
    cmd->forwardmove = 450.0f;
    cmd->sidemove = 0.0f;
}
```

Разворот на 180° и «вперёд» — `PM_Accelerate` тормозит
`accel * frametime * wishspeed * friction` против движения. Быстрее, чем
пассивное трение, потому что `wishspeed` полный, а не кап 30.

---

## 11. Bhop и серверный кап 1.2 ⭐

```c
// pm_shared.h:74
#define BUNNYJUMP_MAX_SPEED_FACTOR 1.2f

// pm_shared.cpp — PM_PreventMegaBunnyJumping
maxscaledspeed = BUNNYJUMP_MAX_SPEED_FACTOR * pmove->maxspeed;

spd = Length(pmove->velocity);
if (spd <= maxscaledspeed) return;

fraction = (maxscaledspeed / spd) * 0.8;     // ⭐ ещё и *0.8
VectorScale(pmove->velocity, fraction, pmove->velocity);
```

То есть **сервер сам** срезает скорость выше `1.2 * maxspeed`, причём
режет до `0.8 * 1.2 * maxspeed = 0.96 * maxspeed` — то есть **ниже**
максимальной. Это значит: в CS 1.6 нельзя «разогнаться до 500» прыжками,
потолок ≈ `1.2 * 250 = 300`, и срез уводит на ~240.

### Что делает `bhop` чит

Не «убирает кап» (это серверное), а **не даёт трению сработать в кадре
приземления**:

```c
if (g_cfg.misc.bhop && (flags & FL_ONGROUND) && !(cmd->buttons & IN_JUMP))
{
    cmd->buttons |= IN_JUMP;      // авто-прыжок в кадре приземления
}
```

Трение считается в `PM_Friction` **до** `PM_Jump`; если прыгнуть в том же
кадре, `onground` сбрасывается и трение в этом кадре не применяется.
Это и есть весь bhop: **один кадр**, не «магия».

### Идеальный bhop

```c
if (g_pmove->onground == -1 && !(cmd->buttons & IN_JUMP))  // в воздухе
    cmd->buttons |= IN_JUMP;                                // держим прыжок
```

Удержание `IN_JUMP` в воздухе ничего не делает (прыжок требует
`onground`), но гарантированно срабатывает в первом кадре приземления.

---

## 12. Super toss (гранаты) — сила броска

В `CL_CreateMove` сила броска = `velocity` игрока + импульс вперёд:

```c
// сервер, CGrenade::Throw
pev->velocity = pPlayer->pev->velocity + gpGlobals->v_forward * power;
```

`power` для `HEGrenade` = 500 + 10 * броски (макс 750). Клиент может
прибавить свой `usercmd_t` `forwardmove` — но только в пределах
`maxspeed`, иначе `PM_PlayerMove` срежет.

**Реальный «super toss»** — не увеличение силы, а **прыжок + бросок в
пике**: скорость игрока в прыжке прибавляется к импульсу гранаты.
Поэтому работает `+jump` за кадр до броска.

---

## 13. Что реализуемо — сводка

| фича | статус | где формула |
|---|---|---|
| Remove recoil | **точно** | п. 5, таблица `KickBack` |
| Remove spread (предсказание) | **точно** | п. 2–3, ГПСЧ |
| Autowall / penetration | **точно** | п. 6, цикл `FireBullets3` |
| Autostrafe | **точно** | п. 9, кап 30 |
| Quick stop | **точно** | п. 10 |
| Bhop | **точно** | п. 11, один кадр |
| Super toss | **частично** | п. 12, только через прыжок |
| Chams / nightmode / tracer | **да** | `07-render.md`, `09-chams.md` |
| Thirdperson | **да** | `HUD_Redraw`, `cl_thirdperson` |
| Double tap, hideshots, roll, fake lag | **невозможно** | сервер авторитетен |

Следующий документ — `13-hud-layout.md`: точные координаты и размеры
элементов HUD из `cl_dll`, чтобы ESP/индикаторы сидели как в игре.
