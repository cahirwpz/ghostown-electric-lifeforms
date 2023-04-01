#include <intro.h>
#include <custom.h>
#include <copper.h>
#include <blitter.h>
#include <bitmap.h>
#include <sprite.h>
#include <fx.h>
#include <strings.h>
#include <color.h>
#include <sync.h>
#include <system/memory.h>
#include <system/interrupt.h>

extern TrackT WeaveBarPulse;
extern TrackT WeaveStripePulse;

#include "data/bar.c"
#include "data/bar-dark.c"
#include "data/bar-bright.c"
#include "data/stripes.c"
#include "data/stripes-dark.c"
#include "data/stripes-bright.c"

#define bar_bplmod ((bar_width - WIDTH) / 8 - 2)

#define WIDTH (320 - 32)
#define HEIGHT 256
#define DEPTH 4
#define NSPRITES 8

#define BELOW 0
#define ABOVE BPLCON2_PF2P2

#define O0 0
#define O1 56
#define O2 112
#define O3 172
#define O4 224

#define STRIPES 5
#define BARS 4

typedef struct StateBar {
  /* two halves of the screen: 4 bitplane pointers and bplcon1 */
  CopListT *cp;
  CopInsT *bar[4];
  /* pointers for palette modification instructions */
  CopInsT *palette[4];
  short bar_y[4];
} StateBarT;

typedef struct StateFull {
  StateBarT bars;
  CopListT *cp;
  CopInsT *sprite;
  /* for each line five horizontal positions */
  u_short *stripes[HEIGHT];
} StateFullT;

static int active = 1;
static short sintab8[128 * 4];
static StateFullT *stateFull;
static StateBarT *stateBars;
static SpriteT stripe[NSPRITES];

/* These numbers must be even due to optimizations. */
static char StripePhase[STRIPES] = { 4, 24, 16, 2, 10 };
static char StripePhaseIncr[STRIPES] = { 10, -10, 4, -12, 8 };

static inline void CopSpriteSetHP(CopListT *cp, short n) {
  CopMove16(cp, spr[n * 2 + 0].pos, 0);
  CopMove16(cp, spr[n * 2 + 1].pos, 0);
}

static void MakeCopperListFull(StateFullT *state) {
  short b, y, i;
  CopListT *cp = state->cp;

  CopInit(cp);

  /* Setup initial bitplane pointers. */
  for (i = 0; i < DEPTH; i++)
    CopMove32(cp, bplpt[i], bar.planes[i]);
  CopMove16(cp, bplcon1, 0);

  /* Move back bitplane pointers to repeat the line. */
  CopMove16(cp, bpl1mod, -WIDTH / 8 - 2);
  CopMove16(cp, bpl2mod, -WIDTH / 8 - 2);

  /* Load default sprite settings */
  for (i = 0; i < NSPRITES; i++)
    CopMove32(cp, sprpt[i], stripe[i].sprdat);

  CopWait(cp, Y(-1), 0);

  state->sprite = cp->curr;
  for (i = 0; i < NSPRITES; i++)
    CopMove32(cp, sprpt[i], stripe[i].sprdat->data);

  for (y = 0, b = 0; y < HEIGHT; y++) {
    short vp = Y(y);
    short my = y & 63;

    CopWaitSafe(cp, vp, 0);

    if (my == 0) {
      state->bars.bar[b] = cp->curr;
      for (i = 0; i < DEPTH; i++)
        CopMove32(cp, bplpt[i], NULL);
      CopMove16(cp, bplcon1, 0);
    } else if (my == 8) {
      state->bars.palette[b] =
        CopLoadColorArray(cp, &bar_pal.colors[(y & 64) ? 16 : 0], 16, 0);
    } else if (my == 16) {
      /* Advance bitplane pointers to display consecutive lines. */
      CopMove16(cp, bpl1mod, bar_bplmod);
      CopMove16(cp, bpl2mod, bar_bplmod);
    } else if (my == 49) {
      /* Move back bitplane pointers to repeat the line. */
      CopMove16(cp, bpl1mod, -WIDTH / 8 - 2);
      CopMove16(cp, bpl2mod, -WIDTH / 8 - 2);
      b++;
    }

    {
      short p0, p1;

      if (y & 64) {
        p0 = BELOW, p1 = ABOVE;
      } else {
        p0 = ABOVE, p1 = BELOW;
      }

      CopWait(cp, vp, HP(O0) + 2);
      state->stripes[y] = (u_short *)cp->curr + 1;
      CopSpriteSetHP(cp, 0);
      CopMove16(cp, bplcon2, p0);
      CopWait(cp, vp, HP(O1) + 2);
      CopSpriteSetHP(cp, 1);
      CopMove16(cp, bplcon2, p1);
      CopWait(cp, vp, HP(O2) + 2);
      CopSpriteSetHP(cp, 2);
      CopMove16(cp, bplcon2, p0);
      CopWait(cp, vp, HP(O3) + 2);
      CopSpriteSetHP(cp, 3);
      CopMove16(cp, bplcon2, p1);
      CopWait(cp, vp, HP(O4) + 2);
      CopSpriteSetHP(cp, 0);
      CopMove16(cp, bplcon2, p0);
    }
  }

  CopEnd(cp);
}

static void MakeCopperListBars(StateBarT *bars) {
  short b, by, y, i;
  CopListT *cp = bars->cp;

  for (b = 0; b < BARS; b++) {
    bars->bar[b] = NULL;
    by = bars->bar_y[b];
    if (by >= -33)
      break;
  }

  CopInit(cp);

  /* Setup initial bitplane pointers. */
  for (i = 0; i < DEPTH; i++)
    CopMove32(cp, bplpt[i], bar.planes[i]);
  CopMove16(cp, bplcon1, 0);

  /* Move back bitplane pointers to repeat the line. */
  CopMove16(cp, bpl1mod, -WIDTH / 8 - 2);
  CopMove16(cp, bpl2mod, -WIDTH / 8 - 2);

  for (y = -1; y < HEIGHT && b < BARS; y++) {
    short vp = Y(y);

    CopWaitSafe(cp, vp, 0);

    if (y == by - 1) {
      bars->bar[b] = cp->curr;
      for (i = 0; i < DEPTH; i++)
        CopMove32(cp, bplpt[i], NULL);
      CopMove16(cp, bplcon1, 0);
      CopLoadColorArray(cp, &bar_pal.colors[(b & 1) ? 16 : 0], 16, 0);
    } else if (y == by) {
      /* Advance bitplane pointers to display consecutive lines. */
      CopMove16(cp, bpl1mod, bar_bplmod);
      CopMove16(cp, bpl2mod, bar_bplmod);
    } else if (y == by + 33) {
      /* Move back bitplane pointers to repeat the line. */
      CopMove16(cp, bpl1mod, -WIDTH / 8 - 2);
      CopMove16(cp, bpl2mod, -WIDTH / 8 - 2);
      by = bars->bar_y[++b];
    }
  }

  CopEnd(cp);
}

static void UpdateBarState(StateBarT *bars) {
  short w = (bar_width - WIDTH) / 2;
  short f = frameCount * 16;
  short shift, offset, i, j;

  for (i = 0; i < BARS; i++) {
    CopInsT *ins = bars->bar[i];
    short by = bars->bar_y[i];
    short bx = w + normfx(SIN(f) * w);

    if (ins) {
      /* fixes a glitch on the right side of the screen */
      if (bx >= bar_width - WIDTH - 1)
        bx = bar_width - WIDTH - 1;

      offset = (bx >> 3) & -2;
      shift = ~bx & 15;

      if (by < 0)
        offset -= by * bar_bytesPerRow;

      if (i & 1)
        offset += bar_bytesPerRow * 33;

      for (j = 0; j < DEPTH; j++, ins += 2)
        CopInsSet32(ins, bar.planes[j] + offset);
      CopInsSet16(ins, (shift << 4) | shift);
    }

    f += SIN_HALF_PI;
  }
}

static void UpdateSpriteState(StateFullT *state) {
  CopInsT *ins = state->sprite;
  int t = frameCount * 2;
  int fu = t & 63;
  int fd = ~t & 63;
  short i;

  for (i = 0; i < NSPRITES; i++, ins += 2)
    CopInsSet32(ins, stripe[i].sprdat->data + ((i & 2) ? fd : fu));
}

#define HPOFF(x) HP(x + 32)

static void UpdateStripeState(StateFullT *state) {
  static const char offset[STRIPES] = {
    HPOFF(O0), HPOFF(O1), HPOFF(O2), HPOFF(O3), HPOFF(O4) };
  u_char *phasep = StripePhase;
  char *incrp = StripePhaseIncr;
  const char *offsetp = offset;
  short i;

  for (i = 0; i < STRIPES * 8; i += 8) {
    u_int phase = (u_char)*phasep;
    u_short **stripesp = state->stripes;
    short *st = &sintab8[phase];
    short hp_off = *offsetp++;
    short n = HEIGHT / 8 - 1;

    (*phasep++) += (*incrp++);

    do {
      u_short *ins;
      short hp;

#define BODY()                  \
      ins = (*stripesp++) + i;  \
      hp = hp_off + (*st++);    \
      ins[0] = hp;              \
      ins[2] = hp + 8;

      BODY(); BODY(); BODY(); BODY();
      BODY(); BODY(); BODY(); BODY();
    } while (--n != -1);
  }
}

#define CP_FULL_SIZE (HEIGHT * 22 + 100)
#define CP_BARS_SIZE (500)

/* For both {Copy,Zero}SpriteTiles `t` controls which quarter of stripe,
 * i.e. tile has to be copied or cleared respectively. Note that all 8
 * stripes will be affected. */
#define TILEPTR(i, t)                                                          \
  (stripe[(i)].sprdat->data +                                                  \
   (((i) & 2) ? (t) * stripes_height : HEIGHT - (t) * stripes_height))

static void ZeroSpriteTiles(int t) {
  short i;

  for (i = 0; i < NSPRITES; i++) {
    SprWordT *dst = TILEPTR(i, t);
    bzero(dst, stripes_height * sizeof(SprWordT));
  }
}

static void CopySpriteTiles(int t) {
  short i, j;

  for (i = 0; i < NSPRITES; i++) {
    u_short *src = &_stripes_bpl[i];
    u_short *dst = (u_short *)TILEPTR(i, t);

    for (j = 0; j < stripes_height; j++) {
      *dst++ = *src;
      src += stripes_bytesPerRow / sizeof(u_short);
      *dst++ = *src;
      src += stripes_bytesPerRow / sizeof(u_short);
    }
  }
}

static void Load(void) {
  int i, j;

  for (i = 0, j = 0; i < 128; i++, j += 32)
    sintab8[i] = (sintab[j] + 512) >> 10;

  memcpy(&sintab8[128], &sintab8[0], 128 * sizeof(u_short));
  memcpy(&sintab8[256], &sintab8[0], 256 * sizeof(u_short));

  TrackInit(&WeaveBarPulse);
  TrackInit(&WeaveStripePulse);
}

static const PaletteT *barPalArray[] = {
  &bar_bright_pal,
  &bar_pal,
  &bar_dark_pal,
};

static const PaletteT *stripePalArray[] = {
  &stripes_bright_pal,
  &stripes_pal,
  &stripes_dark_pal,
};

static const short palIdxSeq[] = {
  -1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, -1
};

static short barPalIdx[4];
static short stripePalIdx[4];

static void LoadColorArray(const u_short *col, short ncols, u_int start) {
  short n = ncols - 1;
  volatile u_short *colreg = custom->color + start;

  do {
    *colreg++ = *col++;
  } while (--n != -1);
}

static int ForEachFrame(void) {
  short val;
  short i, m;

  UpdateFrameCount();

  if ((val = TrackValueGet(&WeaveStripePulse, frameFromStart))) {
    for (i = 3; i >= 0; i--, val >>= 1) {
      if (val & 1)
        stripePalIdx[i] = 1;
    }
  }

  for (i = 0, m = 0; i < 4; i++, m += 4) {
    short palIdx = palIdxSeq[stripePalIdx[i]];
    if (palIdx < 0)
      continue;
    stripePalIdx[i]++;
    LoadColorArray(&stripePalArray[palIdx]->colors[m], 4, 16 + m);
  }

  if ((val = TrackValueGet(&WeaveBarPulse, frameFromStart))) {
    for (i = 3; i >= 0; i--, val >>= 1) {
      if (val & 1)
        barPalIdx[i] = 1;
    }
  }

  for (i = 0; i < 4; i++) {
    short palIdx = palIdxSeq[barPalIdx[i]];
    if (palIdx < 0)
      continue;
    barPalIdx[i]++;
    {
      CopInsT *ins = stateFull[active ^ 1].bars.palette[i];
      const u_short *col = &barPalArray[palIdx]->colors[(i & 1) ? 16 : 0];
      for (m = 0; m < 16; m++) {
        CopInsSet16(ins++, *col++);
      }
    }
  }

  return 0;
}

INTSERVER(ForEachFrameInterrupt, 0, (IntFuncT)ForEachFrame, NULL);

static void Init(void) {
  short i;

  SetupDisplayWindow(MODE_LORES, X(16), Y(0), WIDTH, HEIGHT);
  SetupBitplaneFetch(MODE_LORES, X(0), WIDTH + 16);
  SetupMode(MODE_LORES, DEPTH);
  LoadPalette(&bar_pal, 0);
  LoadPalette(&stripes_pal, 16);

  /* Place sprites 0-3 above playfield, and 4-7 below playfield. */
  custom->bplcon2 = BPLCON2_PF2PRI | BPLCON2_PF2P1 | BPLCON2_PF1P1;

  for (i = 0; i < NSPRITES; i++) {
    SprDataT *sprdat =
      MemAlloc(SprDataSize(HEIGHT + stripes_height, 2), MEMF_CHIP | MEMF_CLEAR);
    MakeSprite(&sprdat, HEIGHT + stripes_height, false, &stripe[i]);
    EndSprite(&sprdat);
    SpriteUpdatePos(&stripe[i], X(0), Y(0));
  }

  stateFull = MemAlloc(sizeof(StateFullT) * 2, MEMF_PUBLIC|MEMF_CLEAR);
  stateFull[0].cp = NewCopList(CP_FULL_SIZE);
  stateFull[1].cp = NewCopList(CP_FULL_SIZE);
  MakeCopperListFull(&stateFull[0]);
  MakeCopperListFull(&stateFull[1]);

  stateBars = MemAlloc(sizeof(StateBarT) * 2, MEMF_PUBLIC|MEMF_CLEAR);
  stateBars[0].cp = NewCopList(CP_BARS_SIZE);
  stateBars[1].cp = NewCopList(CP_BARS_SIZE);
  MakeCopperListBars(&stateBars[0]);
  MakeCopperListBars(&stateBars[1]);

  UpdateBarState(&stateBars[0]);
  UpdateSpriteState(&stateFull[0]);

  CopListActivate(stateBars[0].cp);

#if 0
  Log("CopperListFull: %ld instructions left\n",
      CP_FULL_SIZE - (stateFull[0].cp->curr - stateFull[0].cp->entry));
  Log("CopperListBars: %ld instructions left\n",
      CP_BARS_SIZE - (stateBars[0].cp->curr - stateBars[0].cp->entry));
#endif

  EnableDMA(DMAF_RASTER);

  AddIntServer(INTB_VERTB, ForEachFrameInterrupt);
}

static void Kill(void) {
  short i;

  RemIntServer(INTB_VERTB, ForEachFrameInterrupt);

  ResetSprites();
  DisableDMA(DMAF_RASTER | DMAF_COPPER);

  DeleteCopList(stateFull[0].cp);
  DeleteCopList(stateFull[1].cp);
  MemFree(stateFull);

  DeleteCopList(stateBars[0].cp);
  DeleteCopList(stateBars[1].cp);
  MemFree(stateBars);

  for (i = 0; i < NSPRITES; i++)
    MemFree(stripe[i].sprdat);
}

PROFILE(UpdateStripeState);

#define BY0 (0 + 16)
#define BY1 (64 + 16)
#define BY2 (128 + 16)
#define BY3 (192 + 16)

static const short BarY[4] = { BY0, BY1, BY2, BY3 };

static void ControlBarsIn(StateBarT *bars) {
  short t = frameFromStart;
  short b = 3;

  bars->bar_y[0] = -100;
  bars->bar_y[1] = -100;
  bars->bar_y[2] = -100;
  bars->bar_y[3] = -100;

  for (b = 3; b >= 0; b--) {
    if (t < 16) {
      bars->bar_y[b] = normfx(SIN(t * 64) * BarY[b]);
      break;
    }

    t -= 16;
    bars->bar_y[b] = BarY[b];
  }
}

static void ControlBarsOut(StateBarT *bars) {
  short t = 64 - frameTillEnd;
  short b = 3;

  bars->bar_y[0] = BarY[0];
  bars->bar_y[1] = BarY[1];
  bars->bar_y[2] = BarY[2];
  bars->bar_y[3] = BarY[3];

  for (b = 3; b >= 0; b--) {
    if (t < 16) {
      bars->bar_y[b] = normfx(SIN(t * 64) * (HEIGHT - BarY[b])) + BarY[b];
      break;
    }

    t -= 16;
    bars->bar_y[b] = HEIGHT;
  }
}

static void ControlBars(StateBarT *bars) {
  if (frameFromStart < 64)
    ControlBarsIn(bars);
  if (frameTillEnd < 64)
    ControlBarsOut(bars);
}

static void ControlStripes(void) {
  if (frameFromStart <= 224) {
    static __code short phase = 0;

    short t = frameFromStart - 64;

    if (t >= 0 && phase == 0) {
      CopySpriteTiles(0);
      ZeroSpriteTiles(1);
      ZeroSpriteTiles(2);
      ZeroSpriteTiles(3);
      ZeroSpriteTiles(4);
      phase++;
    } else if (t >= 33 && phase == 1) {
      CopySpriteTiles(1);
      phase++;
    } else if (t >= 65 && phase == 2) {
      CopySpriteTiles(2);
      phase++;
    } else if (t >= 97 && phase == 3) {
      CopySpriteTiles(3);
      phase++;
    } else if (t >= 129 && phase == 4) {
      CopySpriteTiles(4);
      phase++;
    }
  } else if (frameTillEnd <= 224) {
    static __code short phase = 0;

    short t = 224 - frameTillEnd;

    if (t >= 1 && phase == 0) {
      ZeroSpriteTiles(0);
      phase++;
    } else if (t >= 33 && phase == 1) {
      ZeroSpriteTiles(1);
      phase++;
    } else if (t >= 65 && phase == 2) {
      ZeroSpriteTiles(2);
      phase++;
    } else if (t >= 97 && phase == 3) {
      ZeroSpriteTiles(3);
      phase++;
    } else if (t >= 129 && phase == 4) {
      ZeroSpriteTiles(4);
      phase++;
    }
  }
}

static void Render(void) {
  if (frameFromStart < 64 || frameTillEnd < 64) {
    StateBarT *bars = &stateBars[active];

    ResetSprites();
    ControlBars(bars);
    MakeCopperListBars(bars);
    UpdateBarState(bars);
    CopListRun(bars->cp);
  } else {
    StateFullT *state = &stateFull[active];

    EnableDMA(DMAF_SPRITE);
    ControlStripes();
    ProfilerStart(UpdateStripeState);
    UpdateBarState(&state->bars);
    UpdateSpriteState(state);
    UpdateStripeState(state);
    ProfilerStop(UpdateStripeState);
    CopListRun(state->cp);
  }

  WaitVBlank();
  active ^= 1;
}

EFFECT(Weave, Load, NULL, Init, Kill, Render);
