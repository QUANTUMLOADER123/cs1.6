# Реверс CS 1.6 — предсказание движения (`pm_shared`)

Источник: `FWGS/cs16-client`, `pm_shared/pm_shared.cpp`. Это **тот же код**,
что крутится на сервере в `mp.dll`, поэтому предсказание клиента
совпадает с сервером до бита.

---

## 1. Порядок вызова за кадр

```
PM_PlayerMove()
  ├── PM_CheckParameters()
  ├── PM_ReduceTimers()
  ├── PM_PlayStepSound()
  ├── PM_CheckWaterJump()
  ├── PM_CategorizePosition()      // onground, waterlevel
  ├── PM_Friction()                // :1085
  ├── PM_CheckVelocity()           // кап 2000 по каждой оси
  ├── PM_AddGravity()              // :2162
  ├── PM_CheckFalling()            // :2583, ставит flFallVelocity
  ├── PM_Physics_Toss()            // для не-WALK
  ├── PM_WalkMove()                // :897  (на земле)
  │   ├── PM_Friction()            // ВТОРОЙ раз
  │   ├── PM_Accelerate()          // :856
  │   └── PM_StepSlideMove()
  ├── PM_AirMove()                 // :1315 (в воздухе)
  │   ├── PM_Friction()            // снова
  │   ├── PM_AirAccelerate()       // :1168
  │   └── PM_FlyMove() / PM_StepSlideMove()
  ├── PM_CatagorizePosition()
  └── PM_CheckFalling()
```

**Ключевое:** `PM_Friction` вызывается **дважды** за кадр (до и внутри
`PM_WalkMove`). Это видно только в исходниках — и это важно для
предсказания скорости.

---

## 2. `PM_Friction` (`:1085`)

```c
speed = |velocity|;
if (speed < 0.1f) return;

// только на земле
if (onground != -1)
{
    // трюк: если перед нами обрыв — усиленное трение
    start = origin + vel/speed * 16;  start.z += mins[2];
    stop  = start; stop.z -= 34;
    trace = PM_PlayerTrace(start, stop);
    friction = (trace.fraction == 1.0f)
             ? movevars->friction * movevars->edgefriction   // 4.0 * 2.0
             : movevars->friction;                            // 4.0

    friction *= pmove->friction;    // обычно 1.0

    // bleed off speed, но не меньше stopspeed
    control = (speed < movevars->stopspeed) ? movevars->stopspeed : speed;
    drop += friction * (control * frametime);
}

newspeed = max(0, speed - drop) / speed;
velocity *= newspeed;
```

### Числа по умолчанию
| параметр | значение |
|---|---:|
| `friction` | 4.0 |
| `edgefriction` | 2.0 |
| `stopspeed` | 75.0 |
| `accelerate` | 5.0 |
| `airaccelerate` | 10.0 |
| `maxspeed` | 320 (с оружием ниже) |
| `gravity` | 800 |

### Почему это важно для quick stop

`drop = friction * max(speed, 75) * frametime`

При `frametime = 0.01` и скорости 250:
```
drop = 4.0 * 250 * 0.01 = 10   → newspeed = 240
```

*Edgefriction* включается, когда игрок **на краю** — трение удваивается.
Это и есть причина, почему на краю платформы тормозят быстрее.

---

## 3. `PM_Accelerate` (`:856`) — земля

```c
currentspeed = DotProduct(velocity, wishdir);
addspeed = wishspeed - currentspeed;
if (addspeed <= 0) return;

accelspeed = accel * frametime * wishspeed * friction;   // 5.0 * 0.01 * 250 * 1 = 12.5
if (accelspeed > addspeed) accelspeed = addspeed;

velocity += accelspeed * wishdir;
```

**Здесь `wishspeed` НЕ капается** — на земле можно разогнаться до
`maxspeed`. Кап делает вызывающий код (`PM_WalkMove`):

```c
if (wishspeed > movevars->maxspeed)
    wishspeed = movevars->maxspeed;
```

---

## 4. `PM_AirAccelerate` (`:1168`) — воздух (сердце bhop/strafe)

```c
float wishspd = wishspeed;
if (wishspd > 30) wishspd = 30;          // ← КАП 30 В ВОЗДУХЕ

currentspeed = DotProduct(velocity, wishdir);
addspeed = wishspd - currentspeed;
if (addspeed <= 0) return;

accelspeed = accel * wishspeed * frametime * friction;
//            10.0 * wishspeed * 0.01 * 1.0
if (accelspeed > addspeed) accelspeed = addspeed;

velocity += accelspeed * wishdir;
```

### Разбор

`addspeed = 30 - projectionspeed`. Значит **прирост возможен только если
проекция скорости на wishdir меньше 30**. Если ты бежишь прямо вперёд на
300 — проекция 300, `addspeed < 0`, прироста нет.

**Отсюда вся математика strafe:** надо смотреть под углом, чтобы проекция
была мала, но сам вектор скорости был большим.

Максимальный прирост за кадр:
```
accelspeed = airaccelerate * wishspeed * frametime * friction
           = 10 * 450 * 0.01 * 1 = 45   (ограничен addspeed ≤ 30)
```

То есть **не более 30 юнитов/с за кадр** — это потолок.

### Оптимальный угол

Прирост скорости за кадр:
```
dv = accelspeed * cos(theta)     (theta — угол между wishdir и velocity)
```
но `accelspeed ≤ 30 - v*sin(phi)` где phi — отклонение.

Максимум при `theta = 0` и проекции = 0, то есть **wishdir перпендикулярен
скорости**. Это 90° — но тогда `addspeed = 30 - 0 = 30` и весь прирост
уходит в сторону, а не вперёд.

Реальный оптимум чуть меньше 90°: `omega = 90 - 180/(wishspeed/30 + 1)`.
При wishspeed 450 → **78.75°**. Именно это используется в autostrafe.

### Проверка расчётом

```
omega = 90 - 180/(450/30 + 1) = 90 - 180/16 = 90 - 11.25 = 78.75°
```

Значит желаемое направление должно быть на 78.75° в сторону от текущей
скорости. Каждый кадр — снова в сторону. Скорость растёт, угол остаётся.

---

## 5. Система координат — где чаще всего ошибаются

`AngleVectors` из `pm_shared.cpp`:

```c
void AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
    forward[0] = cp * cy;
    forward[1] = cp * sy;
    forward[2] = -sp;

    right[0] = -1 * sr * sp * cy + -1 * cr * -sy;
    right[1] = -1 * sr * sp * sy + -1 * cr * cy;
    right[2] = -1 * sr * cp;
}
```

При `roll = 0` (`sr = 0`, `cr = 1`):
```
forward = (cos yaw,  sin yaw, 0)
right   = (sin yaw, -cos yaw, 0)     ← ЗНАК МИНУС У Y
```

**Отсюда:** `sidemove > 0` даёт движение в направлении
`(sin yaw, -cos yaw)`, то есть при `yaw = 0` это **−y**, а не +y.

Формула сборки wishvel:
```c
wishvel[i] = forward[i] * forwardmove + right[i] * sidemove;
```

Поэтому при разложении целевого угла `target`:
```c
forward = cos(target - view.yaw);
side    = -sin(target - view.yaw);   // минус из-за right.y = -cos
```

Если забыть минус — autostrafe будет крутить игрока в противоположную
сторону (или вообще стоять на месте при беге вдоль оси Y). Эта ошибка
была в SDK и исправлена после численной проверки.

---

## 6. `PM_CheckVelocity` — кап скорости

```c
for (i = 0; i < 3; i++)
{
    if (velocity[i] > movevars->maxvelocity)  velocity[i] = maxvelocity;
    if (velocity[i] < -movevars->maxvelocity) velocity[i] = -maxvelocity;
}
```

`maxvelocity = 2000`. Кап **по каждой оси отдельно**, не по модулю.
Поэтому «разгон» выше 2000 невозможен даже с читами на клиенте.

---

## 7. `PM_StepSlideMove` — лестницы и ступеньки

```c
if (pmove->onground != -1) {
    // пробуем пройти вверх на stepheight (18)
    PM_StepUp();
}
```

`stepheight = 18`. Это причина, почему игрок «запрыгивает» на ящики
высотой до 18 юнитов без прыжка.

**Для fast ladder / edge jump:** прыжок в момент `onground == -1` даёт
`PM_AirMove`, и `PM_AirAccelerate` работает только там.

---

## 8. Итог для SDK

| фича | что использовать |
|---|---|
| autostrafe | `omega = 90 - 180/(w/30+1)`, знак по `d_right` |
| quick stop | `wishdir` противоположен скорости, `drop = 4*max(speed,75)*ft` |
| bhop | `IN_JUMP` удерживать, кап `1.2*maxspeed` (см. `01-movement.md`) |
| предсказание | `m_flFallVelocity` не писать — сервер не читает |
| edge jump | `onground == -1` и `oldbuttons & IN_JUMP` |
| fast ladder | движение по `forward` при `movetype == 5` |

Все константы (`friction 4.0`, `stopspeed 75`, `accelerate 5`,
`airaccelerate 10`, `maxspeed 320`, `stepheight 18`, `maxvelocity 2000`)
из `pm_shared` — не выдуманы.
