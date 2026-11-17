# Реверс CS 1.6 — сеть и user messages

Источники: `common/net_api.h`, `common/entity_state.h`, `common/cl_entity.h`,
`engine/APIProxy.h`. Все 150 user messages с адресами — в
[`exports/usermsgs.txt`](exports/usermsgs.txt).

---

## 1. Три слоя сети

| слой | что передаётся | где живёт |
|---|---|---|
| **NetAPI** | запросы к мастер-серверу, пинги | `net_api_t` (в `gEngfuncs+0x158`) |
| **Snapshot / delta** | состояние сущностей | `entity_state_t` |
| **User messages** | игровые события | обработчики в `client.dll` |

Чита нужны второй и третий: первый — только список серверов.

---

## 2. `entity_state_t` — 340 байт

Полная раскладка посчитана из `common/entity_state.h` и **сверена с
бинарником** через код, читающий `cl_entity_t`:

```asm
0194D51E  mov ecx,[edi+0xb54]     ; angles.x   curstate+0x1C
0194D4F1  mov edx,[edi+0xb58]     ; angles.y
0194D505  mov eax,[edi+0xb5c]     ; angles.z
0196D539  mov edi,[edi+0x364]     ; weaponmodel curstate+0xB4
```

| оффсет | поле | тип |
|---|---|---|
| `+0x000` | `entityType` | int |
| `+0x004` | `number` | int |
| `+0x008` | `msg_time` | float |
| `+0x00C` | `messagenum` | int |
| `+0x010` | `origin` | vec3 |
| `+0x01C` | `angles` | vec3 |
| `+0x028` | `modelindex` | int |
| `+0x02C` | `sequence` | int |
| `+0x030` | `frame` | float |
| `+0x038` | `skin` | short |
| `+0x03A` | `solid` | short |
| `+0x03C` | `effects` | int |
| `+0x040` | `scale` | float |
| `+0x044` | `eflags` | **byte** |
| `+0x048` | `rendermode` | int |
| `+0x04C` | `renderamt` | int |
| `+0x050` | `rendercolor` | color24 |
| `+0x058` | `movetype` | int |
| `+0x070` | `velocity` | vec3 |
| `+0x07C` | `mins` | vec3 |
| `+0x088` | `maxs` | vec3 |
| `+0x0A4` | `team` | int |
| `+0x0AC` | `health` | **int** |
| `+0x0B0` | `spectator` | qboolean |
| `+0x0B4` | `weaponmodel` | int |
| `+0x0BC` | `basevelocity` | vec3 |
| `+0x0C8` | `usehull` | int |
| `+0x0CC` | `oldbuttons` | int |
| `+0x0D0` | `onground` | int |
| `+0x0DC` | `fov` | float |
| `+0x104..0x110` | `iuser1..4` | int |
| `+0x124..0x148` | `vuser1..4` | vec3 |

**Важно:** `armorvalue` в `entity_state_t` **нет**. Броня приходит только
через `ArmorType` в `client_data_t`, а не в снапшоте сущности.

---

## 3. `cl_entity_t` — 2996 байт

```c
struct cl_entity_s {
    int                 index;            // +0x000
    qboolean            player;           // +0x004
    entity_state_t      baseline;         // +0x008
    entity_state_t      prevstate;        // +0x15C
    entity_state_t      curstate;         // +0x2B0  ⭐ самая нужная
    int                 current_position; // +0x404
    position_history_t  ph[64];           // +0x408  (64 * 24 = 1536)
    mouth_t             mouth;            // +0xA08
    latchedvars_t       latched;          // +0xA14
    float               lastmove;         // +0xA70
    vec3_t              origin;           // +0xA74  интерполированная позиция
    vec3_t              angles;           // +0xA80
    vec3_t              attachment[4];    // +0xA8C
    int                 trivial_accept;   // +0xABC
    model_s            *model;            // +0xAC0
    efrag_s            *efrag;            // +0xAC4
    mnode_s            *topnode;          // +0xAC8
    float               syncbase;         // +0xACC
    int                 visframe;         // +0xAD0
    colorVec            cvFloorColor;     // +0xAD4
};
```

### Что откуда брать

| задача | поле |
|---|---|
| позиция для ESP | `origin` (`+0xA74`) — интерполированная |
| позиция из снапшота | `curstate.origin` (`+0x2C0`) |
| здоровье | `curstate.health` (`+0x35C`) |
| команда | `curstate.team` (`+0x354`) |
| модель в руках | `curstate.weaponmodel` (`+0x364`) |
| присед | `curstate.usehull` (`+0x378`) или `eflags & FL_DUCKING` |
| границы (bbox) | `curstate.mins/maxs` (`+0x32C`/`+0x338`) |
| история позиций | `ph[]` — 64 записи по 24 байта |
| признак игрока | `player` (`+0x004`) |

`ph[]` — **история для интерполяции**. Именно из неё считается «лаг-компенсация»
при стрельбе по движущейся цели: `ph[current_position]` — последняя позиция.

---

## 4. User messages — 150 штук

Механизм: клиент регистрирует обработчик по имени, сервер шлёт сообщение по
**индексу**, который выдал движок при регистрации. Индекс не константа — он
зависит от порядка регистрации.

### Как находится паттерн регистрации

```asm
01940710  push  01940260        ; handler
01940715  push  019E8F50        ; "CurWeapon"
0194071A  call  ds:[HookUserMsg]
```

То есть тройка `push handler / push string / call HookUserMsg`.
По этому паттерну вытащены все 150: имя, адрес регистрации, обработчик.

### Ключевые сообщения для чита

| имя | обработчик | что даёт |
|---|---|---|
| `CurWeapon` | `0x01940260` | текущее оружие, патроны в магазине, isActive |
| `WeaponList` | `0x01940280` | **полная таблица оружия**: id, слот, патроны |
| `AmmoX` | `0x019402A0` | запас патронов по типам |
| `Health` | `0x01944060` | здоровье |
| `Battery` | `0x01944170` | броня |
| `ArmorType` | `0x01944170` | **наличие шлема** ⭐ |
| `DeathMsg` | `0x019454D0` | убийство: killer, victim, weapon, headshot ⭐ |
| `SayText` | `0x01963AE0` | чат |
| `TextMsg` | `0x01969960` | системные сообщения |
| `TeamInfo` | `0x0194E680` | команда и модель игрока |
| `ScoreInfo` | `0x0194E620` | счёт игрока |
| `ScoreAttrib` | `0x0194AE80` | **жив/мёртв, есть ли C4** ⭐ |
| `Money` | `0x0194AF80` | деньги |
| `RoundTime` | `0x0194C086` | таймер раунда |
| `HideWeapon` | `0x01940370` | маска скрытия HUD |
| `Crosshair` | `0x019403D0` | параметры прицела |
| `Radar` | `0x0194AF00` | настройки радара |
| `ResetHUD` | `0x0194DF80` | новый раунд |
| `SetFOV` | `0x0194E150` | FOV — важно для world_to_screen |

### Форматы ключевых сообщений

Вытащены из строк, которые читает каждый обработчик:

```
DeathMsg:      byte killer, byte victim, byte isHeadshot, string weapon
SayText:       byte entityIndex, string text
TextMsg:       byte destination, string text
TeamInfo:      byte index, string team                  ("CT" / "TERRORIST")
ScoreAttrib:   byte index, byte attrib                  (0x1 = dead, 0x2 = C4)
RoundTime:     u16 seconds
Health:        byte health
Battery:       byte armor
ArmorType:     byte index, byte hasHelmet
SetFOV:        byte fov
HideWeapon:    byte mask
CurWeapon:     byte state, byte weaponId, byte clip
WeaponList:    string name, byte primaryAmmoId, byte primaryAmmoMax,
               byte secondaryAmmoId, byte secondaryAmmoMax,
               byte slot, byte slotPosition, byte weaponId, byte flags
```

---

## 5. Дельта-кодирование

Сервер не шлёт снапшот целиком. Он шлёт **дельту** относительно
`baseline`, и клиент восстанавливает состояние:

```c
// упрощённо
for each field:
    if (field changed vs baseline) {
        read value;
        write to curstate;
    } else {
        copy from baseline;
    }
```

Отсюда: `baseline` — состояние «по умолчанию» для модели, `prevstate` —
предыдущий кадр, `curstate` — текущий. Все три лежат в `cl_entity_t`.

Следствие для ESP: если сущность «не обновлялась» (`msg_time` старый), её
`curstate` может содержать протухшие данные. Проверять `msg_time` против
`g_globals->time`.

---

## 6. Интерполяция

Клиент рисует не `curstate`, а интерполированную позицию в `origin`
(`+0xA74`). Формула: смесь `prevstate.origin` и `curstate.origin` по
`cl_interp`.

`ph[]` (64 записи) хранит историю, чтобы при потере пакетов брать предыдущие
позиции.

**Для aimbot:** целиться надо в `curstate.origin` (то, где игрок *на сервере*),
а рисовать — в `origin`. Разница — это лаг-компенсация. В SDK
`sh_player_fill` берёт `ent->origin` (интерполированный, для ESP) — правильно;
для стрельбы нужно `curstate.origin`.

---

## 7. NetAPI

```c
typedef struct net_api_s {
    void   (*InitNetworking)(void);
    void   (*Status)(struct net_status_s *status);
    void   (*SendRequest)(int context, int request, int flags, double timeout,
                          struct netadr_s *remote, net_api_response_func_t cb);
    void   (*CancelRequest)(int context);
    void   (*CancelAllRequests)(void);
    char  *(*AdrToString)(struct netadr_s *a);
    int    (*CompareAdr)(struct netadr_s *a, struct netadr_s *b);
    int    (*StringToAdr)(char *s, struct netadr_s *a);
    const char *(*ValueForKey)(const char *s, const char *key);
    void   (*RemoveKey)(char *s, const char *key);
    void   (*SetValueForKey)(char *s, const char *key, const char *value, int maxsize);
} net_api_t;
```

Указатель: `gEngfuncs + 0x158` (`pNetAPI`). Нужен только для сервер-браузера.

---

## 8. Что это даёт SDK

Из этого документа в `skeethook_1_6` взято:

- `sh_entity_state_t` (0x154) и `sh_cl_entity_t` (0xBB4) — с `static_assert`
- поля читаются по именам, а не по оффсетам
- `ScoreAttrib` — единственный способ узнать «мёртв ли», не дожидаясь `health=0`
- `ArmorType` — единственный способ узнать шлем
- `SetFOV` — иначе `world_to_screen` врёт при смене FOV (снайперка)

---

## 9. Следующее

- `07-render.md` — `tri_api`, мировые матрицы, точный `world_to_screen`
- `08-events.md` — `events/*.sc` скрипты, `PlaybackEvent`
- сверка `ph[]` и лаг-компенсации по `curstate` vs `origin`
