# Реверс CS 1.6 — карта фич: что реально, чем делается

Итоговый документ. Для каждой механики из типичного набора чита:
**работает ли**, **каким механизмом**, **откуда взято**, **в каком файле SDK**.

Легенда: ✅ работает · ⚠️ работает с честной оговоркой · ❌ невозможно

Последняя сверка — по коду `skeethook_1_6` (52 файла). У каждой ✅ стоит
файл, где это лежит: если файла нет — значит не сделано.

---

## Ragebot

| фича | | механизм | файл |
|---|---|---|---|
| Aimbot | ✅ | `CL_CreateMove` → правка `cmd.viewangles` | `sh_rage.cpp` |
| Silent aim | ✅ | углы подменяются только в `cmd`, камера не трогается | `sh_rage.cpp` |
| Auto fire | ✅ | `cmd->buttons \|= 1` | `sh_rage.cpp` |
| Auto stop | ✅ | `forwardmove/sidemove = 0` + снятие `IN_JUMP` при выстреле | `sh_rage.cpp` |
| **Penetration** | ✅ | `sh_autowall_trace` — цикл по `12-ballistics.md` п.6 | `sh_autowall.cpp` |
| **Auto wall** | ✅ | то же + `sh_trace_line` через движок | `sh_autowall.cpp` |
| **Double tap** | ⚠️ | досыл `IN_ATTACK` в окне 1.5 `frametime` — выстрел на кадр раньше | `sh_rage.cpp` |
| **Remove recoil** | ✅ | вычитание `punchangle.x/y` из `viewangles` | `sh_rage.cpp` |
| **Remove spread** | ⚠️ | компенсация `punchangle.z`; серверный разброс не отменяется | `sh_rage.cpp` |
| Hitbox выбор | ✅ | `aimbot.hitbox` → `target.hitgroup` | `sh_rage.cpp` |
| Priority цели | ✅ | distance / fov / low health / angle+dist | `sh_rage.cpp` |
| Min damage | ✅ | фильтр по реальному урону из autowall | `sh_rage.cpp` |
| Knife bot | ✅ | `KNIFE_STAB_DISTANCE 32`, `SWING_DISTANCE 48` | `sh_knife.cpp` |
| Spread-трейсеры | ✅ | `sh_spread_compute` по таблице `sh_recoil_for` | `sh_ballistics.cpp` |

**Честная граница double tap:** в CS:GO это сброс tickbase (64 тика),
в GoldSrc тиков нет — есть частота отправки команд. Ускорить её выше
`cl_cmdrate` 100 нельзя (проверено: `cmp eax,0x64` @ `hw.dll+0x18ECD`).
Максимум — не терять кадры и досылать выстрел в окно готовности.

---

## Antiaim

| фича | | механизм | файл |
|---|---|---|---|
| Pitch down/up | ✅ | `x = 88 / -88` | `sh_antiaim.cpp` |
| **Fake pitch (magic)** | ✅ | `x = 1620 / -1619` — ломает хитбокс | `sh_antiaim.cpp` |
| Pitch spin | ✅ | `cos(t*11*pi) * 90` | `sh_antiaim.cpp` |
| Yaw backwards | ✅ | `y += 180` | `sh_antiaim.cpp` |
| Yaw static L/R | ✅ | `y = 90 / 270` | `sh_antiaim.cpp` |
| Yaw left/right | ✅ | `y += 90 / 270` | `sh_antiaim.cpp` |
| Yaw spin | ✅ | накопление `spin_speed` | `sh_antiaim.cpp` |
| Yaw jitter | ✅ | `±yaw_jitter` с флипом каждый кадр | `sh_antiaim.cpp` |
| Yaw random | ✅ | 98% назад + шум | `sh_antiaim.cpp` |
| Yaw fake sideways | ✅ | `−91°` с флипом | `sh_antiaim.cpp` |
| **Desync** | ⚠️ | `±desync_amount` (кап 90) с флипом — **псевдо**: хитбокс в GoldSrc это коробка ±16, поворот модели на него не влияет | `sh_antiaim.cpp` |
| **Roll** | ⚠️ | `viewangles.z` (работает в демках/HLTV, в игре камера не крутится) | `sh_antiaim.cpp` |
| **Fake lag** | ✅ | сдвиг `next_send_time` (0x2D5DA48) — пакет не уходит | `sh_fakelag.cpp` |
| Fake duck | ✅ | `IN_DUCK` (4) на земле | `sh_antiaim.cpp` |
| Micro move | ✅ | `forwardmove = micro_move_speed` | `sh_antiaim.cpp` |
| Movement fix | ✅ | перепроекция `forwardmove/sidemove` на старые углы | `sh_antiaim.cpp` |
| Check IN_ATTACK | ✅ | не ломать аим при стрельбе | `sh_antiaim.cpp` |
| Check C4/граната/нож | ✅ | пропуск | `sh_antiaim.cpp` |
| Check noclip/WALK | ✅ | пропуск | `sh_antiaim.cpp` |
| Hideshot (локально) | ✅ | снятие `EF_MUZZLEFLASH` (2) + `weaponanim = 0` | `sh_hooks.cpp` |

**Почему desync «псевдо»:** в CS:GO хитбоксы — капсулы по костям, и
поворот модели меняет то, куда попадает пуля. В GoldSrc сервер считает
попадание по коробке `±16 × ±16 × ±36` от `origin`, независимо от
`angles`. Поворот модели виден визуально (и в демках), но хитбокс не
двигается. Проверено по `12-ballistics.md` и `SV_HullForEntity`.

**Почему fake lag работает:** клиент отправляет команды не чаще
`cl_cmdrate`, а `next_send_time` — та самая граница. Сдвигая её вперёд,
мы физически не даём движку отправить пакет. Это подтверждено
дизассемблером (`hw.dll+0x18ECD` clamp 100 и паттерн
`83 F8 FF 75 0A A1 30 9B D5 02 A3`).

---

## Visuals

| фича | | механизм | файл |
|---|---|---|---|
| ESP box | ✅ | 2D-бокс по `world_to_screen` | `sh_esp.cpp` |
| ESP name/health/armor | ✅ | `cl_entity_t` + `clientdata` | `sh_esp.cpp` |
| ESP weapon + иконки | ✅ | `WeaponList` из `player.cpp:3415` | `sh_usermsg.cpp` |
| ESP ammo `clip/clip_max` | ✅ | `CurWeapon` `b,b,b` из `weapons.cpp:1385` | `sh_usermsg.cpp` |
| ESP distance | ✅ | `len3d(origin − local)` | `sh_esp.cpp` |
| ESP skeleton | ⚠️ | 2D-проекция по боксу; настоящих костей нет (`studiomodel` недоступен) | `sh_esp.cpp` |
| ESP head dot / snaplines | ✅ | проекция | `sh_esp.cpp` |
| Team check / visible only | ✅ | `team` + `sh_player_visible` | `sh_esp.cpp` |
| **Chams** | ✅ | `HUD_AddEntity`: `rendermode 0`, `renderamt 255` | `sh_hooks.cpp` |
| **Chams team colors** | ✅ | `rendercolor`: T (255,90,90), CT (90,140,255) | `sh_hooks.cpp` |
| **Glow** | ✅ | `renderfx 20` + `glow_amount` 20..255 | `sh_hooks.cpp` |
| **Tracers** | ✅ | линия from→to, затухание по alpha, 2 с | `sh_visuals.cpp` |
| **Impacts** | ✅ | крестик 2→6 px за 1 с + звук `bullet_hit1.wav` | `sh_visuals.cpp` |
| **Thirdperson** | ✅ | сдвиг `view_ofs` назад по forward | `sh_visuals.cpp` |
| **Nightmode World** | ✅ | `gl_monolights`, `gl_fog 1`, `gl_lightholes 0` | `sh_visuals.cpp` |
| **Nightmode Sky** | ✅ | `gl_overbright` | `sh_visuals.cpp` |
| **Nightmode Lighting** | ✅ | `gl_flashblend` (порог 0.5) | `sh_visuals.cpp` |
| **Skybox changer** | ✅ | `sv_skyname`, 12 вариантов, save/restore | `sh_skybox.cpp` |
| Flat textures | ✅ | `gl_picmip`, `r_detailtextures` | `sh_worldfx.cpp` |
| Hitmarker | ✅ | usermsg `Health` (падение HP) + `Damage` | `sh_hitmarker.cpp` |
| Hit sound | ✅ | `pfnPlaySoundByName` по реальному попаданию | `sh_worldfx.cpp` |
| **Radar hack** | ✅ | свой радар 110×110, масштаб 2000 юнитов | `sh_esp.cpp` |
| **Spectator list** | ✅ | список мёртвых игроков | `sh_esp.cpp` |
| **Grenade path** | ✅ | 96 точек, `flVel = (90−pitch)*6`, g=800 | `sh_grenade.cpp` |
| Fire indicator | ✅ | `weaponanim` 1/2/3 меняется → выстрел врага | `sh_usermsg.cpp` |
| No flash / no smoke | ✅ | фильтр спрайтов `renderfx` 19/20 | `sh_hooks.cpp` |
| Indicators | ✅ | список активных фич + choke + shots + fall | `sh_indicators.cpp` |

---

## Misc / Movement

| фича | | механизм | файл |
|---|---|---|---|
| **Bhop** | ✅ | снятие `IN_JUMP` по `oldbuttons` для повтора | `sh_movement.cpp` |
| **Autostrafe** | ✅ | `omega = 90 − 180/(wishspeed/30+1)`, знак по `d_right` | `sh_movement.cpp` |
| **Quick stop** | ✅ | wishdir назад через `forward/side`, камера не трогается | `sh_movement.cpp` |
| **Super toss** | ✅ | прыжок при падении + добавка `forwardmove` | `sh_movement.cpp` |
| Auto pistol | ✅ | `IN_ATTACK` с допуском 0.5 `frametime` | `sh_misc.cpp` |
| Fast reload | ✅ | `IN_RELOAD` при пустом магазине | `sh_misc.cpp` |
| Fast ladder | ✅ | движение по `forward` при `movetype == 5` | `sh_misc.cpp` |
| Edge jump | ✅ | прыжок при `onground == −1` | `sh_misc.cpp` |
| No scope | ✅ | снятие `IN_ATTACK2` | `sh_misc.cpp` |
| No recoil | ✅ | `punchangle` в `viewangles` | `sh_rage.cpp` |
| No spread | ⚠️ | компенсация крена; серверный разброс остаётся | `sh_rage.cpp` |
| Fall damage warn | ⚠️ | `(v−500)*100/600` — предупреждение, не отмена урона | `sh_movement.cpp` |
| No fall damage | ❌ | урон считает сервер, `pmove->flFallVelocity` только читается | `01-movement.md` |

---

## Что невозможно и почему

| фича | почему |
|---|---|
| Настоящий desync | хитбокс — коробка, поворот модели его не двигает |
| Double tap как в CS:GO | нет tickbase, `cl_cmdrate` кап 100 |
| No fall damage | урон на сервере из `-pev->velocity.z` |
| Отмена серверного разброса | ГПСЧ на сервере, клиент только видит результат |
| Скелетон по костям | `studiomodel` в `cl_enginefuncs` — `void*`, нет `SetupBones` |
| Отмена `svc_weaponanim` для врагов | пакет шлёт сервер |

---

## Проверка «на пустышки»

Каждая фича проверена скриптом: поле конфига должно встречаться в
`src/`, иначе это мёртвый чекбокс. Найдено и исправлено 25 таких полей:

`no_smoke`, `no_flash`, `radar_hack`, `spectator_list`, `no_fall_damage`,
`super_toss`, `auto_stop`, `team_check`, `show_local`, `desync`,
`desync_amount`, `fake_duck`, `micro_move`, `micro_move_speed`,
`yaw_jitter`, `no_spread`, `no_recoil`, `impact`, `bullet_impact_sound`,
`col_health`, `col_armor`, `priority`, `hitbox`, `world_color`,
`sky_color`, `light_color`, `grenade_predict`, `hideshot`.

Плюс найдены функции без вызовов (написаны, но никогда не звались):
`sh_autowall_damage`, `sh_spread_compute`, `sh_rage_double_tap`,
`sh_visuals_impact_add`, `sh_fakelag_run` (писал счётчик, не сдвигал
`next_send_time`), `sh_hitmarker_on_hit` (звался при выстреле, а не при
попадании).

И удалён мёртвый код: `sh_aimbot.cpp` (201 строка, вытеснен `sh_rage`),
`sh_bones.cpp` (недоступны кости), дубли `sh_bhop`/`sh_auto_strafe`/
`sh_no_recoil` в `sh_misc.cpp`.
