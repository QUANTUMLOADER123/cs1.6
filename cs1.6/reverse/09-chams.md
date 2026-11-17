# Реверс CS 1.6 — модель игрока, кости, чеймы

Источники: `cl_dll/include/studio/StudioModelRenderer.h`,
`cl_dll/StudioModelRenderer.cpp`, `cl_dll/GameStudioModelRenderer.cpp`,
`engine/studio.h`.

---

## 1. Как рисуется игрок

Движок, обходя мир, для каждой видимой сущности зовёт клиент:

```
HUD_AddEntity(type, cl_entity_t *ent)      // "добавь в кадр"
   ↓
HUD_DrawNormalTriangles()                  // 3D-проход
   ↓
StudioDrawPlayer(flags, entity_state_t*)   // модель игрока
   ↓
StudioSetupBones()                         // расчёт 128 костей
   ↓
StudioRenderModel()                        // GL
```

Ключевое: **`HUD_AddEntity` — самая ранняя точка**, где можно что-то
изменить до рендера. Именно там меняют `rendermode` для чеймов.

---

## 2. `CStudioModelRenderer` — 30+ виртуальных методов

Интерфейс выдаёт `HUD_GetStudioModelInterface` (`0x0194A840`):

```cpp
class CStudioModelRenderer
{
public:
    virtual void Init();
    virtual int  StudioDrawModel(int flags);
    virtual int  StudioDrawPlayer(int flags, entity_state_t *pplayer);

    virtual mstudioanim_t *StudioGetAnim(model_t *sub, mstudioseqdesc_t *seq);
    virtual void StudioSetUpTransform(int trivial_accept);
    virtual void StudioSetupBones();
    virtual void StudioCalcAttachments();
    virtual void StudioSaveBones();
    virtual void StudioMergeBones(model_t *sub);
    virtual float StudioEstimateInterpolant();
    virtual float StudioEstimateFrame(mstudioseqdesc_t *seq);
    virtual void StudioFxTransform(cl_entity_t *ent, float transform[3][4]);
    virtual void StudioSlerpBones(vec4_t q1[], float pos1[][3],
                                  vec4_t q2[], float pos2[][3], float s);
    virtual void StudioCalcBoneAdj(float dadt, float *adj,
                                   const byte *ctrl1, const byte *ctrl2, byte mouth);
    virtual void StudioCalcBoneQuaterion(int frame, float s, mstudiobone_t *bone,
                                         mstudioanim_t *anim, float *adj, float *q);
    virtual void StudioCalcBonePosition(int frame, float s, mstudiobone_t *bone,
                                        mstudioanim_t *anim, float *adj, float *pos);
    virtual void StudioCalcRotations(float pos[][3], vec4_t *q,
                                     mstudioseqdesc_t *seq,
                                     mstudioanim_t *anim, float f);
    virtual void StudioRenderModel(float *lightdir);
    virtual void StudioRenderFinal();
    virtual void StudioRenderFinal_Hardware();
    virtual void StudioPlayerBlend(mstudioseqdesc_t *seq, int *blend, float *pitch);
    virtual void StudioEstimateGait(entity_state_t *pplayer);
    virtual void StudioProcessGait(entity_state_t *pplayer);
    virtual void StudioSetShadowSprite(int idx);
    virtual void StudioDrawShadow(vec3_t origin, float scale);

public:
    double          m_clTime;          // время клиента
    double          m_clOldTime;
    int             m_fDoInterp;
    int             m_iShadowSprite;
    int             m_fGaitEstimation;
    int             m_nFrameCount;
    cvar_t         *m_pCvarHiModels;
    cvar_t         *m_pCvarDrawEntities;
    cvar_t         *m_pCvarShadows;
    cl_entity_t    *m_pCurrentEntity;  // ⭐ кого рисуем сейчас
    model_t        *m_pRenderModel;
    player_info_t  *m_pPlayerInfo;
    int             m_nPlayerIndex;    // ⭐ индекс игрока
    float           m_flGaitMovement;
    studiohdr_t    *m_pStudioHeader;   // ⭐ заголовок модели
    mstudiobodyparts_t *m_pBodyPart;
    mstudiomodel_t     *m_pSubModel;
    int             m_nTopColor;
    int             m_nBottomColor;
    int             m_nCachedBones;
    char            m_nCachedBoneNames[MAXSTUDIOBONES][32];   // ⭐ имена костей
    float           m_rgCachedBoneTransform[MAXSTUDIOBONES][3][4];
    float           m_rgCachedLightTransform[MAXSTUDIOBONES][3][4];
    float           m_vUp[3];
    float           m_vRight[3];
    float           m_vNormal[3];
    float           m_vRenderOrigin[3];
    float         (*m_pbonetransform)[MAXSTUDIOBONES][3][4];   // ⭐ МИРОВЫЕ МАТРИЦЫ КОСТЕЙ
    float         (*m_plighttransform)[MAXSTUDIOBONES][3][4];
    entity_state_t *m_pplayer;
};
```

**`MAXSTUDIOBONES = 128`** (`engine/studio.h:37`).

---

## 3. Кости: `m_pbonetransform` ⭐

`m_pbonetransform` — указатель на `float[128][3][4]`: для каждой кости
матрица 3×4 мировых координат. Позиция кости — **последний столбец**:

```c
vec3_t bone_pos(csr_t *r, i32 bone)
{
    float (*m)[3][4] = r->m_pbonetransform;
    if (!m || bone < 0 || bone >= 128) return zero;
    vec3_t p = { m[bone][0][3], m[bone][1][3], m[bone][2][3] };
    return p;
}
```

Это **точная позиция кости в мире**, посчитанная движком с учётом
анимации, интерполяции и приседа. Никакой ручной математики.

### Как получить доступ

```c
// 1. найти интерфейс
typedef struct { int version; void *fn[...]; } StudioInterface;
StudioInterface *si;
typedef int (*getstudio_fn)(StudioInterface **);
getstudio_fn f = (getstudio_fn)GetProcAddress(client, "HUD_GetStudioModelInterface");
f(&si);

// 2. из него — рендерер
typedef void *(*getclone_fn)(void);
CStudioModelRenderer *r = ((getclone_fn)si->fn_getrenderer)();
```

Практичнее — хук `StudioDrawPlayer`: рендерер доступен как `this`
(первый аргумент `__thiscall`, `ecx`).

---

## 4. Скелет по костям

Имена костей в `m_nCachedBoneNames[i]` (32 байта каждая). Для CS 1.6:

| индекс | имя | что |
|---:|---|---|
| 0 | `Bip01 Pelvis` | таз |
| 1 | `Bip01 Spine` | поясница |
| 2 | `Bip01 Spine1` | грудь |
| 3 | `Bip01 Neck` | шея |
| 4 | `Bip01 Head` | голова |
| 5-6 | `Bip01 L UpperArm` / `L Forearm` | левая рука |
| 7 | `Bip01 L Hand` | левая кисть |
| 8-9 | `Bip01 R UpperArm` / `R Forearm` | правая рука |
| 10 | `Bip01 R Hand` | правая кисть |
| 11-12 | `Bip01 L Thigh` / `L Calf` | левая нога |
| 13 | `Bip01 L Foot` | левая стопа |
| 14-15 | `Bip01 R Thigh` / `R Calf` | правая нога |
| 16 | `Bip01 R Foot` | правая стопа |

**Правильный скелет ESP** рисуется по этим позициям, а не по
пропорциональным точкам внутри bbox (как сделано в текущем SDK — это
приближение).

---

## 5. Чеймы: четыре способа

| # | способ | как | качество |
|---|---|---|---|
| 1 | `HUD_AddEntity` | правка `curstate.rendermode`/`renderamt` | грубо, но просто |
| 2 | `StudioDrawModel` хук | своя отрисовка вместо/вместе с оригиналом | **правильно** |
| 3 | `R_ParticleBox` | эффект по bbox | без правки модели |
| 4 | цвет модели | `m_nTopColor`/`m_nBottomColor` | глючит с моделями |

### Способ 2 — эталон

```c
i32 __fastcall hook_studio_draw_model(void *self, void *edx, i32 flags)
{
    cl_entity_t *ent = *(cl_entity_t **)((u8 *)self + OFF_CURRENT_ENTITY);
    if (!ent || !ent->player) return orig(self, edx, flags);

    if (ent->index != g_local &&
        is_enemy(ent) && g_cfg.visuals.chams)
    {
        glColor4ub(...);              // цвет чейма
        glDepthFunc(GL_ALWAYS);       // сквозь стены
        orig(self, edx, flags);       // рисуем модель

        glDepthFunc(GL_LEQUAL);       // вторым проходом — видимая часть
        glColor4ub(...);              // другой цвет
        orig(self, edx, flags);
    }
    else
    {
        orig(self, edx, flags);
    }
    return 1;
}
```

Два прохода: первый `GL_ALWAYS` (сквозь стены, цвет «невидимый»),
второй `GL_LEQUAL` (видимая часть, цвет «видимый»). Это классические
двухцветные чеймы.

---

## 6. Глоу (свечение)

Проще всего — через `HUD_AddEntity`:

```c
ent->curstate.rendermode = 0;   // kRenderNormal
ent->curstate.renderamt  = 60;  // полупрозрачность
ent->curstate.renderfx   = 20;  // kRenderFxGlowShell
```

`renderfx = 20` (`kRenderFxGlowShell`) — движок сам рисует оболочку.
`renderfx = 21` — `kRenderFxGlowShellPulse`.

Ровно это реализовано в `sh_add_entity_hook` в SDK.

---

## 7. Хитбоксы: где реально голова

Из `engine/studio.h` и `pm_shared`:

```c
// hull 0 (стоя)
player_mins = { -16, -16, -36 }
player_maxs = {  16,  16,  36 }
// hull 1 (присед)
player_mins = { -16, -16, -18 }
player_maxs = {  16,  16,  18 }
```

Глаза:
- стоя: `origin.z + 28`
- присед: `origin.z + 12`

Хитгруппы (из `studio.h`):

| группа | множитель урона |
|---|---:|
| `HITGROUP_HEAD` (1) | **4.0** |
| `HITGROUP_CHEST` (2) | 1.0 |
| `HITGROUP_STOMACH` (3) | 1.25 |
| `HITGROUP_LEFTARM` (4) | 1.0 |
| `HITGROUP_RIGHTARM` (5) | 1.0 |
| `HITGROUP_LEFTLEG` (6) | 0.75 |
| `HITGROUP_RIGHTLEG` (7) | 0.75 |

**Голова — ×4**, живот — ×1.25. Это и есть таблица приоритетов aimbot.

---

## 8. Что поправлено в SDK

1. Скелет ESP сейчас рисуется по **пропорциональным точкам bbox** —
   это приближение. Правильно — по `m_pbonetransform`.
2. Хитбоксы `g_hitbox_offsets = {28, 26, 18, 8, 2, -6}` — приближение
   от `origin.z`. Правильные — из `m_pbonetransform[4]` (голова),
   `[2]` (грудь), `[3]` (таз).
3. Множители урона `{1, 4, 1, 1.25, 1, 1, 0.75, 0.75}` — **совпали** с
   `sh_hitgroups` в `sh_aimbot.cpp`.

---

## 9. Следующее

- `10-anticheat.md` — что светит клиентский чит, что проверяет VAC
- переход скелета и хитбоксов на `m_pbonetransform`
