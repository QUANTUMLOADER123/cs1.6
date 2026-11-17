# Реверс CS 1.6 — сетевой протокол: 59 opcodes `svc_*`

Извлечено из `hw.dll`: строки имён обработчиков идут в памяти **в том же
порядке, что и таблица обработчиков протокола**. Это даёт точную нумерацию.

Адрес блока: `hw.dll` offset `0x13AFD0` (секция `.rdata`).

---

## 1. Полная таблица opcodes

| # | opcode | что приходит |
|---:|---|---|
| 0 | `svc_bad` | маркер ошибки |
| 1 | `svc_nop` | пустышка |
| 2 | `svc_disconnect` | разрыв |
| 3 | `svc_event` | игровое событие (не надёжное) |
| 4 | `svc_version` | версия протокола |
| 5 | `svc_setview` | какой клиент — точка обзора |
| 6 | **`svc_sound`** | звук (канал, сэмпл, позиция) |
| 7 | `svc_time` | серверное время |
| 8 | `svc_print` | сообщение в консоль |
| 9 | `svc_stufftext` | команда клиенту |
| 10 | `svc_setangle` | углы обзора |
| 11 | `svc_serverinfo` | параметры сервера |
| 12 | `svc_lightstyle` | стиль освещения |
| 13 | `svc_updateuserinfo` | инфо игрока |
| 14 | `svc_deltadescription` | описание дельты |
| 15 | **`svc_clientdata`** | `clientdata_t` локального игрока |
| 16 | `svc_stopsound` | стоп звука |
| 17 | `svc_pings` | пинги |
| 18 | `svc_particle` | частица |
| 19 | **`svc_damage`** | броня/здоровье/направление урона |
| 20 | `svc_spawnstatic` | статичный объект |
| 21 | `svc_event_reliable` | событие (надёжное) |
| 22 | `svc_spawnbaseline` | базовое состояние сущности |
| 23 | **`svc_temp_entity`** | временная сущность (взрыв, кровь) |
| 24 | `svc_setpause` | пауза |
| 25 | `svc_signonnum` | номер этапа входа |
| 26 | `svc_centerprint` | текст в центре |
| 27 | `svc_killedmonster` | убит монстр |
| 28 | `svc_foundsecret` | найден секрет |
| 29 | `svc_spawnstaticsound` | статичный звук |
| 30 | `svc_intermission` | конец раунда |
| 31 | `svc_finale` | финал |
| 32 | `svc_cdtrack` | музыка |
| 33 | `svc_restore` | восстановление |
| 34 | `svc_cutscene` | катсцена |
| 35 | `svc_weaponanim` | анимация оружия |
| 36 | `svc_decalname` | имя декали |
| 37 | `svc_roomtype` | тип комнаты (реверб) |
| 38 | `svc_addangle` | добавить угол |
| 39 | `svc_newusermsg` | регистрация usermsg |
| 40 | **`svc_packetentities`** | сущности (полное состояние) |
| 41 | **`svc_deltapacketentities`** | сущности (дельта) |
| 42 | `svc_choke` | choke-ответ |
| 43 | `svc_resourcelist` | список ресурсов |
| 44 | `svc_newmovevars` | параметры движения |
| 45 | `svc_resourcerequest` | запрос ресурса |
| 46 | `svc_customization` | кастомизация |
| 47 | `svc_crosshairangle` | угол прицела |
| 48 | `svc_soundfade` | затухание звука |
| 49 | `svc_filetxferfailed` | передача файла провалилась |
| 50 | `svc_hltv` | HLTV |
| 51 | `svc_director` | директор (HL) |
| 52 | `svc_voiceinit` | голос: инициализация |
| 53 | `svc_voicedata` | голос: данные |
| 54 | `svc_sendextrainfo` | доп. инфо |
| 55 | `svc_timescale` | масштаб времени |
| 56 | `svc_resourcelocation` | URL ресурса |
| 57 | `svc_sendcvarvalue` | значение cvar клиента |
| 58 | `svc_exec` | выполнить |

---

## 2. Ключевые для ESP и парсера

### `svc_packetentities` (40) / `svc_deltapacketentities` (41)

Полное и дельта-состояние сущностей. Формат:

```
byte  numents
for each entity:
    bit   entity present
    if present:
        delta-compressed entity_state_t (по delta_description)
```

Дельта-описание приходит в `svc_deltadescription` (14) — массив
`delta_description_t`, который говорит для каждого поля: биты числа,
позиция, флаги.

**Вот почему ESP в CS 1.6 «дёрганый»:** данные приходят только для
сущностей, которые сервер считает изменившимися. Статичные враги
не обновляются кадр в кадр — отсюда интерполяция (см. `06-networking.md`).

### `svc_clientdata` (15)

Присылает `clientdata_t` (476 байт, см. `11-clientdata.md`) —
**только для локального игрока**. Это источник `health`, `punchangle`,
`fov`, `deadflag`.

### `svc_damage` (19)

```
byte armor
byte health
int  (длина последующего блока)   ; только при health > 0
```

Отсюда можно узнать свой урон, не читая память.

### `svc_temp_entity` (23)

`byte type` + параметры. Тип — `TE_*`:

| TE | что |
|---|---|
| `TE_BLOOD` | кровь |
| `TE_GUNSHOT` | попадание в стену |
| `TE_EXPLOSION` | взрыв |
| `TE_SMOKE` | дым |
| `TE_SPARKS` | искры |
| `TE_GLOWSPRITE` | свечение |

**Это готовый источник для impact-эффектов** в ESP: не надо трассировать
самому, сервер уже сказал, где попали.

### `svc_weaponanim` (35)

`byte anim`, `byte body`. Позволяет видеть анимацию оружия врага —
основа «weapon activity ESP».

---

## 3. Реализация в SDK

`src/features/sh_demo.cpp` читает блоки `.dem`, а блок типа
`dem_net` — это поток этих opcodes. Порядок разбора:

```c
i32 svc = buf[pos++];

switch (svc)
{
case 15:  // svc_clientdata
    read_client_data(buf + pos, &cd);
    break;
case 40:  // svc_packetentities
case 41:  // svc_deltapacketentities
    read_entities(buf + pos);
    break;
case 23:  // svc_temp_entity
    read_temp_entity(buf + pos);
    break;
default:
    break;
}
```

Чего **не хватает** для полного парсера:

1. `delta_description_t` — приходит в `svc_deltadescription` (14),
   содержит для каждого поля его битовое смещение, число бит, флаги;
2. таблица полей `entity_state_t` в **том же порядке**, что в дельте
   (порядок определяется сервером при `svc_deltadescription`).

Значит корректный парсер сначала читает описание дельты, а потом
применяет его к потоку. Это и есть разница между «работающим» и
«случайным» парсером демок.

---

## 4. Что даёт на практике

| фича | источник |
|---|---|
| ESP врагов | 40/41 |
| Свой урон | 19 |
| Impact-эффекты | 23 |
| Анимация оружия врага | 35 |
| Свой health/armor | 15 |
| Разбор демок | всё вместе |
| Sound ESP | 6 (`svc_sound` — позиция источника) |

Последнее интересно: `svc_sound` приходит с **мировой позицией** и номером
звука. По номеру можно понять, что это выстрел врага, и нарисовать
индикатор направления — точнее, чем эвристика по `svc_temp_entity`.

---

## 5. Проверка полноты

Всего opcodes: **59** (0..58). Это соответствует GoldSrc-протоколу
(48 базовых + расширения HL/CS). `svc_exec` — последний, добавлен в
Steam-версии.

Таблица извлечена по смещениям строк, значит порядок **точный** —
менее надёжно было бы угадывать по `switch`-таблицам в дизассемблере.
