# 23. Переопределения клиентского состояния: `HUD_TxferLocalOverrides`

Документ разбирает три функции, через которые клиентская DLL
переписывает сетевое состояние игроков: `HUD_TxferLocalOverrides`,
`HUD_ProcessPlayerState` и `HUD_TxferPredictionData`. Все три лежат в
`cl_dll/entity.cpp` (FWGS/cs16-client), все три про `iuser1..4`,
`fuser1..4` и `vuser1..4`.

Главный вывод, который ломает интуицию: **`iuser1..4` — это не
пользовательские поля для чита**. Это три разных набора семантики в
зависимости от того, чьё состояние переносится: локального игрока,
наблюдаемого игрока или любого игрока на сервере. И один и тот же
`iuser4` означает три разные вещи в трёх функциях. Из-за этого первая
версия нашего хука обнуляла `iuser3`/`iuser4` и ломала игру: игрок
переставал приседать и стрелять.

---

## 1. `HUD_TxferLocalOverrides` — 9 строк, ноль логики

`cl_dll/entity.cpp:85`:

```c
void DLLEXPORT HUD_TxferLocalOverrides(struct entity_state_s *state,
                                       const struct clientdata_s *client)
{
    VectorCopy(client->origin, state->origin);

    // Spectator
    state->iuser1 = client->iuser1;
    state->iuser2 = client->iuser2;

    // Duck prevention
    state->iuser3 = client->iuser3;

    // Fire prevention
    state->iuser4 = client->iuser4;
}
```

Функция вызывается движком, чтобы влить клиентские данные в
`entity_state_t` локального игрока перед предсказанием. Комментарии в
исходнике расставляют всё по местам:

| поле | смысл в этой функции | источник |
|---|---|---|
| `iuser1` | режим наблюдения (`observer mode`) | `clientdata_t` |
| `iuser2` | первая цель наблюдения | `clientdata_t` |
| `iuser3` | запрет приседания (`duck prevention`) | `clientdata_t` |
| `iuser4` | запрет выстрела (`fire prevention`) | `clientdata_t` |

Что делает движок с этими значениями:

- `iuser3 != 0` — `PM_Duck` не опускает прицел/не меняет `bInDuck`,
  то есть клиент физически не может присесть. Используется, когда
  сервер запрещает присед (например, на некоторых картах или в
  режимах с `mp_ducking 0`).
- `iuser4 != 0` — `IN_ATTACK` не доходит до оружия, выстрел не
  предсказывается. Используется для блокировки стрельбы на сервере
  (в зоне респавна, до начала раунда).

**Практический вывод:** эти два поля — готовый механизм «сервер
разрешил/запретил действие». Наш хук, который их обнулял, отбирал у
сервера право запрета, но заодно и право разрешения — движок начинал
считать, что запрета нет, и рассинхронизировался с сервером. Правильное
поведение: **не трогать их вообще**, а если нужно своё — писать
`0`/`1` осознанно, зная, что это за флаг.

Наш код (`src/core/sh_hooks.cpp`, `sh_txfer_override_hook`) сейчас:

```c
void __cdecl sh_txfer_override_hook(sh_entity_state_t *state,
                                    const sh_client_data_t *client)
{
    if (!g_txfer_override_orig)
        return;

    g_txfer_override_orig(state, client);

    if (!state || !client)
        return;

    vec3_t saved = state->origin;
    ...
}
```

То есть оригинал вызывается первым (он и копирует `iuser1..4`), а мы
поверх правим только `origin` — для `fake_duck`. Флаги не трогаются.
Это правильно.

---

## 2. `HUD_ProcessPlayerState` — здесь `iuser4` уже не то же самое

`cl_dll/entity.cpp:157`:

```c
if (dst->number == player->index)   // локальный игрок
{
    g_iTeamNumber = g_PlayerExtraInfo[dst->number].teamnumber;

    dst->iuser1 = g_iUser1 = src->iuser1;
    dst->iuser2 = g_iUser2 = src->iuser2;
    dst->iuser3 = g_iUser3 = src->iuser3;
}
dst->fuser2 = src->fuser2;

if (src->number > 0 && src->number < MAX_PLAYERS)
{
    iOnTrain[src->number] = src->iuser4;
}
```

Здесь `iuser4` — это **`iOnTrain`**. Не «запрет выстрела», а признак
«игрок стоит на движущейся платформе/поезде». И это видно в
`HUD_AddEntity` (`entity.cpp:46`):

```c
if (ent->player && iOnTrain[ent->index])
{
    VectorCopy(ent->curstate.origin, ent->origin);
    VectorCopy(ent->curstate.angles, ent->angles);
}
```

То есть если `iuser4` был выставлен в `HUD_ProcessPlayerState`,
`HUD_AddEntity` **перезаписывает интерполированную позицию жёсткой
позицией из сетевого состояния**. Это нужно, потому что на движущейся
платформе клиентская интерполяция даёт дрожание — модель «плывёт»
относительно платформы.

**Важный побочный эффект для ESP:** позиция такого игрока берётся не из
интерполяции, а из `curstate`. Если читать `ent->origin` без учёта
`iOnTrain`, бокс будет дёргаться на поездах (`func_train` на картах
вроде `de_train`). Наш `sh_players_track` читает `ent->origin`, а
движок уже подменил его — значит всё корректно, но знать про это надо.

Три `iuser` для локального игрока дублируются в глобалы
`g_iUser1/2/3` — их использует `HUD_AddEntity` для фильтрации
наблюдаемых сущностей (`if (g_iUser1)` в `entity.cpp:70`). Это не наши
`g_cfg`, это переменные клиента.

---

## 3. `HUD_TxferPredictionData` — перенос предсказанного состояния

`cl_dll/entity.cpp:176`. Комментарий в исходнике объясняет задачу
лучше любого пересказа:

> Because we can predict an arbitrary number of frames before the server
> responds with an update, we need to be able to copy client side
> prediction data in from the state that the server ack'd receiving,
> which can be anywhere along the predicted frame path (i.e., we could
> predict 20 frames into the future and the server ack's up through 10 of
> those frames, so we need to copy persistent client-side only state from
> the 10th predicted frame to the slot the server update is occupying).

Смысл: клиент предсказывает на 20 кадров вперёд, сервер подтвердил
только 10. Состояние 10-го кадра надо перетащить в слот, куда сервер
пишет обновление, иначе предсказание «прыгнет».

Что переносится:

```c
ps->oldbuttons    = pps->oldbuttons;
ps->flFallVelocity = pps->flFallVelocity;
ps->iStepLeft     = pps->iStepLeft;
ps->playerclass   = pps->playerclass;
ps->iuser4        = pps->iuser4;      // ! снова iOnTrain
```

Дальше идёт `clientdata_t` и там — развилка по режиму наблюдения:

```c
if (gEngfuncs.IsSpectateOnly())
{
    pcd->iuser1 = g_iUser1;   // observer mode
    pcd->iuser2 = g_iUser2;   // first target
    pcd->iuser3 = g_iUser3;   // second target
}
else
{
    pcd->iuser1 = ppcd->iuser1;
    pcd->iuser2 = ppcd->iuser2;
    pcd->iuser3 = ppcd->iuser3;
}
pcd->iuser4 = ppcd->iuser4;
```

**Здесь `iuser4` в `clientdata_t` — это опять «запрет выстрела»**, как в
разделе 1. А в `entity_state_t` (`ps->iuser4`) на две строки выше — это
`iOnTrain`. Два разных поля с одинаковым именем в одной функции.

### Наш хук

`sh_txfer_pred_hook` вызывает оригинал и затем копирует
`weapon_data_t[64]` в `g_weapon_data`:

```c
typedef void(__cdecl *txfer_pred_fn)(void *ps, const void *pps, void *pcd,
                                     const void *ppcd, sh_weapon_data_t *wd,
                                     const sh_weapon_data_t *pwd);
```

Это единственный официальный способ увидеть полное состояние всех 64
оружий: `clip`, `next_primary_attack`, `in_reload`, `idle`, `pump`,
`aimed_damage`, `iuser1..4`, `fuser1..4`. Через него работает
`fast_reload` (`sh_misc.cpp`).

---

## 4. Сводная таблица: что значит каждое поле и где

| поле | `TxferLocalOverrides` | `ProcessPlayerState` | `TxferPredictionData` |
|---|---|---|---|
| `iuser1` (entity_state) | режим наблюдения | режим наблюдения → `g_iUser1` | — |
| `iuser2` (entity_state) | первая цель | первая цель → `g_iUser2` | — |
| `iuser3` (entity_state) | запрет приседания | третья цель → `g_iUser3` | — |
| `iuser4` (entity_state) | запрет выстрела | **`iOnTrain`** | **`iOnTrain`** |
| `iuser1` (clientdata) | — | — | режим наблюдения |
| `iuser2` (clientdata) | — | — | первая цель |
| `iuser3` (clientdata) | — | — | вторая цель |
| `iuser4` (clientdata) | — | — | запрет выстрела |

Три разные роли у `iuser3`: в `entity_state` при локальном игроке это
«запрет приседания», в `clientdata` при наблюдении — «вторая цель», в
`ProcessPlayerState` — `g_iUser3`. Именно поэтому нельзя писать «`iuser3`
всегда значит X».

### `fuser`-поля

- `fuser2` — переносится везде без изменений. Используется клиентом для
  времени последнего выстрела при расчёте `m_flNextAttack`.
- `fuser3` — то же, для вторичной атаки.
- `vuser2/vuser3/vuser4` — векторы клиентского предсказания (позиции
  эффектов, привязанных к оружию).

---

## 5. Что это даёт SDK

1. **`fake_duck`** реализован правильно: правим `origin` в
   `TxferLocalOverrides`, не трогая `iuser3`. Движок сам решит, можно ли
   присесть; мы лишь сдвигаем позицию камеры/модели.
2. **`iOnTrain`** объясняет дёрганье боксов на поездах и позволяет
   отдельно обрабатывать такие сущности в ESP (не интерполировать).
3. **`weapon_data_t[64]`** из `TxferPredictionData` — единственный
   источник правды о патронах в магазине и таймерах перезарядки. Все
   фичи «no-recoil», «fast-reload», «auto-pistol» строятся на нём.
4. **Режим наблюдения** (`iuser1`) в `clientdata` живёт в `g_iUser1` —
   если надо узнать, что мы спектатор и кого смотрим, читаем его, а не
   ковыряем сетевые пакеты.

---

## 6. Ошибка, которую мы допустили и исправили

Первая версия `sh_txfer_override_hook` обнуляла `iuser3` и `iuser4`,
исходя из догадки «это мусор, который мешает». Результат: персонаж
перестал приседать и стрелять, потому что обнуление **снимало
разрешение**, которое сервер выдавал через эти поля. Причём на серверах
с `mp_ducking 1` это незаметно, а на картах с запретом приседа — ломает
игру полностью.

После чтения `entity.cpp` хук переписан: оригинал вызывается, флаги не
трогаются, правится только `origin` при включённом `fake_duck`.

Мораль: **любое поле `i*user*` надо сначала прочитать в исходнике**, а не
угадывать по имени. Одно и то же имя в `entity_state_t` и `clientdata_t`
означает разное, а в третьей функции — третье.
