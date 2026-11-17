# Реверс CS 1.6 — события и эффекты

Источники: `common/event_api.h`, `common/r_efx.h`,
`common/particledef.h`, `common/event_flags.h`, `cl_dll/events/*.cpp`.

---

## 1. `event_api_t` — 24 функции

Доступ: `gEngfuncs->pEventAPI` (оффсет `+0x150`).

```c
typedef struct event_api_s
{
    int   version;

    void  (*EV_PlaySound)(int ent, float *origin, int channel, const char *sample,
                          float volume, float attenuation, int fFlags, int pitch);
    void  (*EV_StopSound)(int ent, int channel, const char *sample);
    int   (*EV_FindModelIndex)(const char *pmodel);
    int   (*EV_IsLocal)(int playernum);
    int   (*EV_LocalPlayerDucking)(void);
    void  (*EV_LocalPlayerViewheight)(float *);
    void  (*EV_LocalPlayerBounds)(int hull, float *mins, float *maxs);
    int   (*EV_IndexFromTrace)(pmtrace_t *pTrace);
    physent_s *(*EV_GetPhysent)(int idx);
    void  (*EV_SetUpPlayerPrediction)(int dopred, int bIncludeLocalClient);
    void  (*EV_PushPMStates)(void);
    void  (*EV_PopPMStates)(void);
    void  (*EV_SetSolidPlayers)(int playernum);
    void  (*EV_SetTraceHull)(int hull);
    void  (*EV_PlayerTrace)(float *start, float *end, int flags, int ignore, pmtrace_t *tr);
    void  (*EV_WeaponAnimation)(int sequence, int body);
    unsigned short (*EV_PrecacheEvent)(int type, const char *psz);
    void  (*EV_PlaybackEvent)(int flags, const edict_s *pInvoker, u16 eventindex,
                              float delay, float *origin, float *angles,
                              float fparam1, float fparam2, int iparam1,
                              int iparam2, int bparam1, int bparam2);
    const char *(*EV_TraceTexture)(int ground, const float *start, const float *end);
    void  (*EV_StopAllSounds)(int entnum, int entchannel);
    void  (*EV_KillEvents)(int entnum, const char *eventname);
} event_api_t;
```

### Что важно для чита

| функция | зачем |
|---|---|
| `EV_PlaybackEvent` | **вызвать событие вручную** — например, нарисовать свой трассер |
| `EV_PlayerTrace` | трассировка через движок, с учётом всех physents |
| `EV_PushPMStates` / `PopPMStates` | **безопасно** работать с `pmove` вне предсказания |
| `EV_SetTraceHull` | сменить hull (голова/присед) |
| `EV_IndexFromTrace` | получить индекс сущности по результату трассы |
| `EV_GetPhysent` | доступ к физической сущности по индексу |

`EV_PushPMStates`/`PopPMStates` — критично: если лезть в `playermove_t`
из `HUD_Redraw`, можно получить мусор (состояние заполняется только во время
предсказания). Правильно: `Push` → работать → `Pop`.

---

## 2. `efx_api_t` — эффекты

Доступ: `gEngfuncs->pEfxAPI` (оффсет `+0x14C`).

```c
struct efx_api_s
{
    particle_t *(*R_AllocParticle)(void (*callback)(particle_t *, float));
    void (*R_BlobExplosion)(float *org);
    void (*R_Blood)(float *org, float *dir, int pcolor, int speed);
    void (*R_BloodSprite)(float *org, int colorindex, int modelIndex,
                          int modelIndex2, float size);
    void (*R_BloodStream)(float *org, float *dir, int pcolor, int speed);
    void (*R_BreakModel)(float *pos, float *size, float *dir, float random,
                         float life, int count, int modelIndex, char flags);
    void (*R_Bubbles)(float *mins, float *maxs, float height, int modelIndex,
                      int count, float speed);
    void (*R_BubbleTrail)(float *start, float *end, float height,
                          int modelIndex, int count, float speed);
    void (*R_BulletImpactParticles)(float *pos);
    void (*R_EntityParticles)(cl_entity_t *ent);
    void (*R_Explosion)(float *pos, int model, float scale, float framerate, int flags);
    void (*R_FizzEffect)(cl_entity_t *pent, int modelIndex, int density);
    void (*R_FireField)(float *org, int radius, int modelIndex, int count,
                        int flags, float life);
    void (*R_FlickerParticles)(float *org);
    void (*R_FunnelSprite)(float *org, int modelIndex, int reverse);
    void (*R_Implosion)(float *end, float radius, int count, float life);
    void (*R_LavaSplash)(float *org);
    void (*R_LargeFunnel)(float *org, int reverse);
    void (*R_MultiGunshot)(float *org, float *dir, float *noise,
                           int count, int decalCount, int *decalIndices);
    void (*R_MuzzleFlash)(float *pos1, int type);
    void (*R_ParticleBox)(float *mins, float *maxs, u8 r, u8 g, u8 b, float life);
    void (*R_ParticleBurst)(float *pos, int size, int color, float life);
    void (*R_ParticleExplosion)(float *org);
    void (*R_ParticleExplosion2)(float *org, int colorStart, int colorLength);
    void (*R_ParticleLine)(float *start, float *end, u8 r, u8 g, u8 b, float life);
    void (*R_PlayerSprites)(int client, int modelIndex, int count, int size);
    void (*R_Projectile)(float *origin, float *velocity, int modelIndex,
                         int life, int owner, void (*hitcallback)(tempent_t *, pmtrace_t *));
    void (*R_RicochetSound)(float *pos);
    void (*R_RicochetSprite)(float *pos, model_s *pmodel, float duration, float scale);
    void (*R_RocketFlare)(float *pos);
    void (*R_RocketTrail)(float *start, float *end, int type);
    /* + R_RunParticleEffect, R_ShowLine, R_Sprite_Explode, R_Sprite_Smoke,
       R_SparkEffect, R_SparkShower, R_SparkStreaks, R_Spray, R_Sprite_Trail,
       R_StreakSplash, R_TracerEffect, R_UserTracerParticle, R_TempSprite,
       R_TempModel, R_DefaultSprite, R_TempBox, R_TempModel, ... */
};
```

### Что это даёт

| задача | функция |
|---|---|
| свой трассер от выстрела | `R_TracerEffect` / `R_TracerParticle` |
| подсветить врага свечением | `R_RocketFlare(origin)` — вспышка в точке |
| линия между двумя точками | `R_ParticleLine(start, end, r, g, b, life)` |
| коробка-«скин» для чеймов | `R_ParticleBox(mins, maxs, r, g, b, life)` |
| след за игроком | `R_RocketTrail(start, end, type)` |
| спавн партикла вручную | `R_AllocParticle(callback)` → полный контроль |

`R_ParticleBox` — самый недооценённый: рисует **заполненную светящуюся
коробку** по bbox. Это готовый «стенхак-силуэт» без правки рендера.

---

## 3. Структуры эффектов

```c
typedef struct particle_s
{
    struct particle_s *next;
    float   org[3];
    float   vel[3];
    float   deathcube;
    float   die;
    int     color;
    int     type;
    void    (*callback)(struct particle_s *particle, float frametime);
} particle_t;
```

`callback` вызывается каждый кадр — можно двигать партикл самому
(орбиты, водовороты вокруг игрока).

---

## 4. Флаги событий (`event_flags.h`)

```c
#define FEV_NOTHOST     0x0001   // не проигрывать локально
#define FEV_HOSTONLY    0x0002   // только на хосте
#define FEV_CLIENT      0x0004   // только на клиенте
```

`FEV_NOTHOST` — то, что ставят оружие в `cl_dll/cs_wpn/*` при
`CLIENT_WEAPONS`, чтобы не дублировать эффект, когда его уже проиграл сервер.

---

## 5. `events/*.sc` — скрипты событий

Файлы `cl_dll/events/*.cpp` — это **обработчики** для `.sc`-скриптов:

```cpp
void EV_FireAK47(event_args_t *args)
{
    // args->entindex, args->origin, args->angles, args->fparam1/2, args->iparam1/2
}
```

`event_args_t` — то, что приходит в обработчик:

```c
typedef struct event_args_s
{
    int     flags;
    int     entindex;
    float   velocity[3];
    float   origin[3];
    float   angles[3];
    int     ducking;
    float   fparam1;   // обычно spread.x
    float   fparam2;   // обычно spread.y
    int     iparam1;   // обычно punchangle.x * 100
    int     iparam2;   // обычно punchangle.y * 100
    int     bparam1;
    int     bparam2;
} event_args_t;
```

**Важное следствие**: в `iparam1/iparam2` летит
`punchangle.x * 100` и `punchangle.y * 100` — то есть **отдача видна в
событии выстрела на клиенте**, без чтения памяти игры.

---

## 6. Трассеры: где их рисовать

`R_TracerEffect(origin, dir)` — создаёт трассер от дула по направлению.
Чтобы перехватить и заменить своим:

1. хук на `EV_FireEvent` (функция движка, через которую идут все события)
2. либо перехват `gEngfuncs->pEventAPI->EV_PlaybackEvent`

Второй проще: подменить указатель в таблице на свою обёртку.

```c
static void (*orig_playback)(int flags, const void *invoker, u16 idx, float delay,
                             float *origin, float *angles, float f1, float f2,
                             int i1, int i2, int b1, int b2);

static void hook_playback(int flags, const void *invoker, u16 idx, float delay,
                          float *origin, float *angles, float f1, float f2,
                          int i1, int i2, int b1, int b2)
{
    if (g_cfg.misc.tracers && origin && angles)
        my_tracer(origin, angles);
    orig_playback(flags, invoker, idx, delay, origin, angles, f1, f2, i1, i2, b1, b2);
}
```

---

## 7. Чеймы: три пути

| путь | как | плюсы / минусы |
|---|---|---|
| **`HUD_AddEntity`** | правка `curstate.rendermode` | просто, но ломает модель |
| **`StudioDrawModel`** | хук рендера модели | чисто, нужен `StudioModelRenderer` |
| **`R_ParticleBox`** | эффект поверх bbox | не трогает модель вовсе |

В `skeethook_1_6` реализован первый (хук `HUD_AddEntity` уже стоит).
Второй — по `HUD_GetStudioModelInterface` (`0x0194A840`), он отдаёт
`StudioModelRenderer_t` с виртуальными методами.

---

## 8. Что добавлено в SDK

- `sh_triapi_t` и `sh_efx_api_t` — оба интерфейса доступны по оффсетам
- хук `HUD_AddEntity` → `rendermode` для chams/glow
- `event_args_t` для будущего перехвата выстрелов
- вывод: **трассеры и партиклы можно рисовать без знания оружия** —
  достаточно поймать `EV_PlaybackEvent`

---

## 9. Следующее

- `09-chams.md` — `StudioDrawModel`, `StudioModelRenderer_t`, скелет
- `10-anticheat.md` — что проверяет VAC и что светит клиентским читом
