# Реверс CS 1.6 — правила игры, серверная часть, боты

## 0. Происхождение бинарников (установлено точно)

Вытащено из строк PDB и версий внутри файлов:

| файл | PDB / версия | вывод |
|---|---|---|
| `client.dll` | `C:\buildslave\goldsrc_win32\build\GoldSrc\cstrike\cl_dll\Release\client.pdb` | официальная сборка Valve GoldSrc (Steam) |
| `hw.dll` | `...\GoldSrc\engine\GL_Release_STEAM\hw.pdb` | официальный движок Valve, GL, Steam |
| `mp.dll` | `ReGameDLL version: 5.23.0.620-dev`, `ReGameDLL_CS/commit/ac07e89` | **ReGameDLL_CS**, открытый проект |

Это важнейший факт для всего реверса:

- **клиент и движок** — ванильные, значит `FWGS/cs16-client` (реверс клиента)
  описывает их корректно. Проверено: оффсеты структур и `sizeof` совпали
  один в один (`playermove_t`, `physent_t`, `cl_enginefunc_t`, `enginefuncs_t`).
- **игровая логика** — ReGameDLL, у него **есть открытые исходники**. Значит
  правила, оружие, боты восстанавливаются не догадками, а по коду
  (с поправкой на версию 5.23).

---

## 1. Иерархия классов сервера (RTTI из `mp.dll`)

Из бинарника вытащено **651 RTTI-имя классов**. Ключевые ветки:

```
CBaseEntity
 ├── CBasePlayerItem
 │    └── CBasePlayerWeapon
 │         ├── CAK47 / CCSAK47      (две ветки — старая и CS-специфичная)
 │         ├── CM4A1, CAWP, CDEAGLE, CGLOCK18, ...
 │         └── CCSPlayerWeapon
 ├── CBasePlayer
 │    └── CCSPlayer                 (+ CCSPlayerItem, CCSPlayerAmmo)
 ├── CWeaponBox  └── CCSWeaponBox
 ├── CBasePlayerAmmo
 ├── CGameRules └── CHalfLifeMultiplay
 ├── CStripWeapons └── CCSStripWeapons
 ├── CGameText, CGameScore, CGameEnd, CGameTeamSet, CGameTeamMaster
 ├── CGamePlayerEquip, CGamePlayerHurt, CGamePlayerZone, CGamePlayerTeam
 └── CGameCounter, CGameCounterSet, CRuleEntity / CRulePointEntity / CRuleBrushEntity

CBot      (+ CAPI_Bot)     — старый бот
CCSBot    (+ CAPI_CSBot)   — CS-бот
CBotManager └── CCSBotManager
CCStrikeGameMgrHelper
CVoiceGameMgr
CCSGameCounter(Set), CCSGamePlayerZone, CCSFuncWeaponCheck, CCSWeaponCycler
```

Плюс целая система **хуков** (`IReGameApi`, `CReGameHookchains`,
`HookChain`, `HookChainRegistry`, `HookChainClass`...) — ReGameDLL позволяет
подменять почти любой метод `CBasePlayer` / `CBasePlayerWeapon` / `CGameRules` /
`CWeaponBox` без патча памяти. Это отдельный, «легальный» слой расширения.

---

## 2. Точки входа `mp.dll` (372 экспорта)

| VA | ординал | экспорт | роль |
|---|---:|---|---|
| `0x100539F0` | 1 | `GiveFnptrsToDll` | получает 158 указателей движка (632 Б) |
| `0x100266A0` | 166 | `GetEntityAPI` | таблица `DLL_FUNCTIONS` |
| `0x10026820` | 167 | `GetNewDLLFunctions` | `NEW_DLL_FUNCTIONS` |
| `0x100E2B10` | 340 | `weapon_ak47` | точка спавна оружия |
| `0x1008EDF0` | 256 | `func_weaponcheck` | |
| `0x10081030` | 310 | `player_weaponstrip` | |
| `0x10053060` | 188 | `cycler_weapon` | |

Сущности `weapon_*` (29 штук, ординалы 340-368) — это `LINK_ENTITY_TO_CLASS`,
имена, по которым движок создаёт оружие.

---

## 3. Порядок работы сервера за кадр

`DLL_FUNCTIONS` (200 байт, 50 указателей) — то, что движок зовёт:

```
ServerActivate          — карта загружена
ClientPutInServer       — игрок подключился
ClientCommand           — игрок ввёл команду
PlayerPreThink          — до физики
PlayerPostThink         — после физики
StartFrame              — начало кадра сервера
ClientKill / ClientDisconnect
Think / Touch / Use     — для сущностей (dispatch через edict)
AddToFullPack           — сборка пакета для клиента ⭐
UpdateClientData        — что отдать клиенту (здоровье/броня/патроны) ⭐
```

`UpdateClientData` — это то, из чего потом `HUD_UpdateClientData` заполняет
`client_data_t`. То есть один контракт на два конца провода.

---

## 4. `AddToFullPack` ⭐

Самая интересная функция для чита: здесь сервер **решает**, что вообще
попадёт в сетевой пакет клиенту.

```c
int AddToFullPack(struct entity_state_s *state, int e, edict_t *ent,
                  edict_t *host, int hostflags, int player,
                  unsigned char *pSet);
```

Именно здесь уходит в сеть: `origin`, `angles`, `health`, `armorvalue`,
`weaponmodel`, `sequence`, `gaitsequence`, `renderamt`, `rendermode`.

Следствие: если сервер не получил сущность — ESP её не нарисует **в принципе**.
Против этого работает `sv_visiblemaxplayers` и «интерполяция пустоты».

На клиенте обратная сторона — `HUD_TxferPredictionData` и буфер
`cl_entity_t`, где хранятся `prevstate`/`curstate` для интерполяции.

---

## 5. Правила: состояние игры

`CGameRules` (`CHalfLifeMultiplay` в этой сборке) держит:

- `m_flRoundTime`, `m_iRoundState` — таймер и фаза раунда
- `m_iNumCT`, `m_iNumTerrorist` — численность
- `m_iNumCTWins`, `m_iNumTerroristWins` — счёт
- `m_bBombPlanted`, `m_flBombDropTime`, `m_flBombDefuseTime`
- `m_iNumTerroristAlive`, `m_iNumCTAlive`
- `m_bRoundTerminating`

Состояния `m_iRoundState`: `GR_STATE_PREROUND`, `GR_STATE_ROUND`,
`GR_STATE_TEAM_WIN`, `GR_STATE_GAME_OVER`.

Для чита это таймер раунда и счёт — можно рисовать без парсинга HUD.

---

## 6. Режимы

| режим | как влияет |
|---|---|
| `mp_freezetime` | блокировка ввода в начале раунда |
| `mp_roundtime` | длительность раунда (мин) |
| `mp_buytime` | окно закупки |
| `mp_friendlyfire` | урон по своим (иначе `Hurt` не сработает) |
| `mp_autoteambalance` | |
| `sv_gravity` (в `movevars_t.gravity`) | 800 по умолчанию |
| `sv_maxspeed` (`movevars_t.maxspeed`) | 320 |
| `sv_accelerate` / `sv_friction` | 10 / 4 |

Важно: **это поля `movevars_t`, а не квары напрямую**. Читать значения надёжнее
через `playermove_t->movevars`, а не через `Cvar_VariableValue` — сервер может
переопределить их для конкретного игрока.

---

## 7. Деньги

`CHudMoney` на клиенте и `CCSPlayer::m_iAccount` на сервере.
Состояние передаётся user message `Money` (id ищется через `RegUserMsg`).

Границы: максимум `16000`, старт `800` (или `mp_startmoney`).
Награды за убийство/раунд — в `CCSGameRules::` таблицах.

---

## 8. Боты (`CCSBot`)

В этой сборке ReGameDLL боты — полноценные: `CCSBotManager`,
`CCSBot::m_iState`, задача (`CCSBot::m_task`), зоны (`m_bombsite`),
`CBotMeme` (тактическая память: `BotMeme`, `BotFollowMeme`, `BotHelpMeme`...).

Для чита это значит: боты физически ходят теми же `CCSPlayer`, значит
подчиняются тем же правилам мувмента — `PM_AirAccelerate` и т.д.

---

## 9. Ограничения серверной стороны

Честно: ReGameDLL 5.23 отличается от ванильного `mp.dll` Valve.
Адреса функций **другие**, поэтому:

- клиентские сигнатуры (по `client.dll`/`hw.dll`) — стабильны, они из
  ванильной сборки;
- серверные адреса — привязаны к этому конкретному `mp.dll`; для чисто
  клиентского проекта (ESP/aim/antiaim) сервер не нужен вовсе.

Поэтому весь SDK ниже строится **только на клиентской стороне** —
там адреса и структуры проверены бинарно и совпадают с исходниками.
