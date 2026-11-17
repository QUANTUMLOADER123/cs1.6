# Реверс CS 1.6 — карта фич: что реально, чем делается

Итоговый документ. Для каждой механики из типичного набора чита:
**работает ли в GoldSrc**, **каким механизмом**, **откуда взято**.

Легенда: ✅ работает · ⚠️ частично · ❌ невозможно в движке

---

## Ragebot

| фича | | механизм | источник |
|---|---|---|---|
| Aimbot | ✅ | `CL_CreateMove` → правка `usercmd.viewangles` | `client.dll 0x01958CC0` |
| Silent aim | ✅ | подмена углов **только** в `cmd`, не в камере | `15-antiaim.md` |
| Auto fire | ✅ | `cmd->buttons \|= IN_ATTACK` | — |
| **Remove recoil** | ✅ | детерминированная отдача, `KickBack` | `12-ballistics.md` п.5 |
| **Remove spread** | ⚠️ | ГПСЧ воспроизводим, но сервер разбрасывает сам | `12-ballistics.md` п.2-4 |
| **Penetration / autowall** | ✅ | повтор цикла `FireBullets3` | `12-ballistics.md` п.6 |
| **Double tap** | ⚠️ | не телепорт тиков, а быстрое переключение при `m_flNextAttack` | ниже, п.1 |
| **Hideshots** | ✅ | `pitch = 1620 / -1619` | `15-antiaim.md` п.2 |
| Auto stop | ✅ | разворот + forward при выстреле | `12-ballistics.md` п.10 |
| Min damage | ✅ | расчёт `damage *= rangeModifier^(dist/500)` | `12-ballistics.md` п.7 |
| Knife bot | ✅ | `KNIFE_STAB_DISTANCE 32`, `SWING_DISTANCE 48` | `weapons.h:1180` |

---

## Antiaim

| фича | | механизм |
|---|---|---|
| Pitch down/up | ✅ | `x = 88 / -88` |
| **Fake pitch (hideshot)** | ✅ | `x = 1620 / -1619` — ломает хитбокс |
| Pitch spin | ✅ | `cos(t*11*pi) * 90` |
| Yaw backwards | ✅ | `y += 180` |
| Yaw static/left/right | ✅ | `y = 90 / 270` |
| Yaw spin | ✅ | `y += accum` |
| Yaw jitter | ✅ | `rand()%100 < 10 → y += 180` |
| Yaw random | ✅ | `98% back + ±70` |
| **Fake sideways** | ✅ | `y -= 91`, через пакет `-= 180` |
| **Roll** | ⚠️ | `viewangles.z` идёт на сервер, влияет на движение; модель крутит сервер |
| **Desync** | ⚠️ | псевдо: разные yaw в разных пакетах. Настоящего CS:GO-десинка нет |
| **Fake lag** | ✅ | лимит choke в переменной `0x1E3AFCC` |
| Movement fix | ✅ | **обязателен**, иначе бег задом |

### Почему desync «псевдо»

В CS:GO хитбокс считается по анимации, и `desync` разводит
`m_angEyeAngles` и позу на `GetMaxDelta()`.

В GoldSrc (`entity_state_t` + `pev->origin`):

```c
player hull: mins = {-16,-16,-36}, maxs = {16,16,36}   // стоя
             mins = {-16,-16,-18}, maxs = {16,16,18}   // присед
```

**Это коробка.** Она не зависит от анимации. Значит рассинхрона
«модель ≠ хитбокс» в принципе нет — но `magic pitch` переворачивает
систему координат, и попасть в голову надо с другой стороны. Это даёт
практически тот же результат.

---

## Visuals

| фича | | механизм | cvar / API |
|---|---|---|---|
| ESP box / name / health | ✅ | `HUD_Redraw` + W2S | `pTriAPI->WorldToScreen` |
| **Skeleton по костям** | ✅ | `m_pbonetransform[128]` | `09-chams.md` |
| **Chams** | ✅ | `HUD_AddEntity` + `StudioDrawModel` | `rendermode/renderamt` |
| Glow | ✅ | `renderfx = 20` (`kRenderFxGlowShell`) | `09-chams.md` |
| **Weapon icons** | ✅ | `WeaponList` usermsg | `16-feature-map.md` п.2 |
| **Ammo** | ✅ | `CurWeapon` usermsg (`state, id, clip`) | `weapons.cpp:1385` |
| **Nightmode** | ✅ | `gl_monolights`, `gl_overbright`, `gl_fog` | найденные cvars |
| **Skybox changer** | ✅ | `sv_skyname` | `hw.dll` |
| **Flat/blocky текстуры** | ✅ | `gl_picmip`, `r_detailtextures` | `hw.dll` |
| **Bullet tracer** | ✅ | своя линия W2S + `R_ParticleLine` | `07-render.md` |
| **Impacts** | ✅ | `pEfxAPI->R_ParticleBox` | `08-events.md` |
| **Thirdperson** | ⚠️ | `pfnSetViewAngles`, полный override камеры без движка не выйдет | `HUD_Redraw` |
| Hitmarker | ✅ | свой рендер | — |
| Spectator list | ✅ | `demo_api->IsPlayingback` + entity scan | `13-hud-layout.md` |
| Sound ESP / hit sound | ✅ | `pfnPlaySoundByName` | — |
| Fog | ✅ | `pTriAPI->Fog` / `gl_fog` | `07-render.md` |

---

## Misc

| фича | | механизм | источник |
|---|---|---|---|
| **Bhop** | ✅ | `IN_JUMP` в кадре приземления | `12-ballistics.md` п.11 |
| **Autostrafe** | ✅ | `omega = 90 − 180/(wishspeed/30+1)` | `12-ballistics.md` п.9 |
| **Quick stop** | ✅ | разворот 180° + forward | п.10 |
| Edge jump | ✅ | `IN_JUMP` у края (трасс вниз) | — |
| Fast ladder | ✅ | `movetype == MOVETYPE_FLY` + forward | — |
| **Super toss** | ✅ | прыжок за кадр до броска | `14-grenades.md` п.5 |
| Auto pistol | ✅ | `p_pmove->next_primary_attack` | `11-clientdata.md` |
| Fast reload | ⚠️ | `IN_RELOAD` при `m_fInReload` | — |
| No fall damage | ❌ | урон считает сервер | — |
| No flash / no smoke | ⚠️ | скрыть спрайт через `HUD_AddEntity` | — |
| No scope | ✅ | `IN_ATTACK2` блок | — |
| Radar hack | ✅ | скан всех entities, без `visible` check | — |
| **Grenade predict** | ✅ | формула + траектория, оптимум 42° | `14-grenades.md` |
| **Demo parser** | ✅ | формат `.dem` | `13-hud-layout.md` |
| Config versioning | ✅ | magic + size | `sh_config.cpp` |

---

## 1. Double tap — почему не как в CS:GO

В CS:GO `double tap` = откат `tickbase` и повторная отправка команд.

В GoldSrc:
- `usercmd_t` содержит `msec` (длительность) и `random_seed`;
- сервер исполняет команды **последовательно** в `PM_Move`;
- `clientdata_t.m_flNextAttack` (`+0x7C`) — таймер атаки, **серверный**.

Что реально можно:

```c
// если таймер ещё тикает — отпустить и переключиться
if (next_attack > now)
{
    cmd->buttons &= ~IN_ATTACK;
    cmd->buttons |= IN_ATTACK2;      // смена оружия -> сброс анимации
}
```

Это даёт **ускоренную стрельбу** для оружия с самоотдачей (AWP, Deagle),
но не два выстрела в одном тике. `sh_rage_double_tap` в SDK делает именно
это и честно называется «switch fire».

---

## 2. `WeaponList` и `CurWeapon` — точные форматы

### `WeaponList` (`player.cpp:3415`)

```c
WRITE_STRING(weapon_name);       // строка с нулём
WRITE_BYTE(PrimaryAmmoID);
WRITE_BYTE(PrimaryAmmoMaxAmount);
WRITE_BYTE(SecondaryAmmoID);
WRITE_BYTE(SecondaryAmmoMaxAmount);
WRITE_BYTE(SlotID);
WRITE_BYTE(NumberInSlot);
WRITE_BYTE(WeaponID);
WRITE_BYTE(Flags);
```

Значит: **сначала строка, потом 8 байт**. Разбор — читать имя до нуля,
дальше байты по порядку. Это то, что делает `sh_cb_weapon_list`.

### `CurWeapon` (`weapons.cpp:1385`)

```c
WRITE_BYTE(state);   // 1 = есть цель, 0 = нет
WRITE_BYTE(m_iId);
WRITE_BYTE(m_iClip);
```

`state != 0` — оружие активно. При смене оружия `m_iClientClip`
сбрасывается и сообщение приходит заново.

Это даёт **точные патроны** без чтения памяти: `clip` приходит от сервера.

---

## 3. Чего нет и почему

| фича | причина |
|---|---|
| Настоящий double tap | нет tickbase, сервер последователен |
| CS:GO-десинк | хитбокс = коробка, не анимация |
| Fake lag > лимита | сервер исполняет всё, ограничение в клиенте |
| No fall damage | урон в `mp.dll` |
| Полный thirdperson | камеру держит движок |
| Замена моделей | `svc_*` прекеш, серверная |

---

## 4. Итоговая статистика проекта

| что | значение |
|---|---|
| документов реверса | **17** |
| строк документации | ~4100 |
| файлов SDK (`.h` + `.cpp`) | 52 |
| строк кода SDK | ~6600 |
| сигнатур (паттернов) | 8/8 уникальны + 1 новый (fakelag) |
| DLL | 350 KB |
| экспортов | 4 |
