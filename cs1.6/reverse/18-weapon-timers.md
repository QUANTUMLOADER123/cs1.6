# Реверс CS 1.6 — таймеры оружия и «attack creep»

Источник: ReGameDLL `dlls/weapons.cpp`,
`CBasePlayerWeapon::GetNextAttackDelay` (~строка 1593 и определение).

---

## 1. Что это

Каждое оружие ставит `m_flNextPrimaryAttack` — время, раньше которого
стрелять нельзя. Но ставит не «сейчас + задержка», а через функцию:

```c
float CBasePlayerWeapon::GetNextAttackDelay(float delay)
{
    // (в некоторых сборках) учёт «наползания»
    float flTimeBetweenFires = gpGlobals->time - m_flLastFireTime;
    float flCreep = 0.0f;

    if (flTimeBetweenFires > 0)
        flCreep = flTimeBetweenFires - m_flPrevPrimaryAttack;

    float flNextAttack = UTIL_WeaponTimeBase() + delay - flCreep;

    m_flLastFireTime = gpGlobals->time;
    m_flPrevPrimaryAttack = flNextAttack - UTIL_WeaponTimeBase();

    return flNextAttack;
}
```

**`- flCreep`** — вот ключ. Если ты стрелял **реже**, чем позволяло
оружие (например, ждал между выстрелами), то `flCreep > 0`, и следующий
таймер ставится **раньше**. Оружие «догоняет» тебя.

Если стрелял **ровно в темп**, `flCreep ≈ 0`, и таймер обычный.

---

## 2. Почему это важно для auto-pistol и double tap

Наивная реализация:

```c
if (next_primary_attack <= now)
    cmd->buttons |= IN_ATTACK;
```

Это работает, но **отстаёт**: сервер ставит таймер с `- flCreep`, а клиент
об этом не знает и ждёт полный интервал. В результате реальный темп
стрельбы ниже возможного.

Правильно:

```c
// держать палец на кнопке каждый кадр, когда таймер прошёл
if (next_primary_attack <= now + pc->frametime * 0.5f)
    cmd->buttons |= IN_ATTACK;
```

Допуск `frametime * 0.5` компенсирует то, что сервер применит `- flCreep`
и выстрел всё равно пройдёт.

### Значения задержек (из оружия)

| оружие | `delay` (сек) |
|---|---:|
| AWP | 1.45 |
| AK-47 | 0.095 |
| M4A1 | 0.0875 |
| Deagle | 0.225 |
| Glock (burst) | 0.15 |
| Knife | 0.4 |
| Knife (fast) | 0.35 |

`0.095` у AK — это **10.5 выстрелов/сек**, что соответствует
`600 RPM / 60 = 10`.

---

## 3. `m_flNextAttack` на стороне игрока

Отдельно от оружия есть **таймер игрока**:

```c
m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;   // при броске гранаты
m_pPlayer->m_flNextAttack = 0.75f;                          // при смене оружия
```

Он блокирует **любую** атаку. Именно он уходит клиенту в
`clientdata_t.m_flNextAttack` (`+0x7C`) и попадает в `weapon_data_t`
(см. `11-clientdata.md`).

**Отсюда «switch fire» (он же псевдо-double-tap):**

```
выстрел -> m_flNextAttack оружия = now + delay
смена оружия -> m_flNextAttack игрока = now + 0.75   (хуже!)
```

Так что **смена оружия замедляет**, а не ускоряет. Настоящий быстрый
огонь — это только не терять кадры:

```c
if (weapon_id == WEAPON_AWP && next_attack > now)
{
    // AWP: между выстрелами 1.45 с — окно для перезарядки/движения
    // но НЕ для выстрела
}
```

---

## 4. Fast reload — что реально

Перезарядка (`Reload()`):

```c
if ((m_pPlayer->pev->button & IN_RELOAD) && iMaxClip() != WEAPON_NOCLIP
    && !m_fInReload && m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
{
    // начинаем перезарядку
}
```

Ключ: **`IN_RELOAD` требует отдельного нажатия**. Отсюда «fast reload»:

```c
if (g_cfg.misc.fast_reload && weapon_empty() && !reloading)
{
    cmd->buttons &= ~IN_ATTACK;
    cmd->buttons |= IN_RELOAD;
}
```

Работает только если магазин пуст — иначе `iMaxClip()` проверка не даст.

**Чего нельзя:** ускорить саму анимацию перезарядки. Она в
`mp.dll`, длительность зашита в оружии (`m_flNextPrimaryAttack`).

---

## 5. Что поправлено в SDK

`sh_misc.cpp`, `sh_auto_pistol`:

```c
if (g_pmove->next_primary_attack > g_pmove->time)
    return;

cmd->buttons |= sh_in_attack;
```

Стало:

```c
f32 now = g_pmove->time;
f32 margin = g_pmove->frametime * 0.5f;

if (g_pmove->next_primary_attack > now + margin)
    return;

cmd->buttons |= sh_in_attack;
```

Допуск в половину кадра даёт «дожатие» выстрела в том кадре, когда
таймер ещё формально не прошёл — с учётом `- flCreep` на сервере.

---

## 6. Итог: почему auto-fire «не попадает в темп»

Причина не в задержке сети, а в том, что **клиент не знает `flCreep`**.
Сервер считает:

```
next = now + delay - (time_since_last_fire - prev_delay)
```

Клиент видит только `next` из `clientdata`. Значит каждая попытка угадать
«когда жать» имеет погрешность в один кадр. Допуск в полкадра — оптимум:
меньше — теряешь выстрелы, больше — жмёшь раньше и выстрел не проходит.

Это одна из тех деталей, которые отличают «работает» от «работает
стабильно» — и её видно только из исходников `GetNextAttackDelay`.
